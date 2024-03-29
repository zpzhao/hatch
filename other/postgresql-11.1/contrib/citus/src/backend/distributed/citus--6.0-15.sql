/* citus.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION citus" to load this file. \quit

CREATE SCHEMA citus;

-- Ensure CREATE EXTENSION is not run against an old citus data
-- directory, we're not compatible (due to the builtin functions/tables)
DO $$
BEGIN
   IF EXISTS(SELECT * FROM pg_proc WHERE proname = 'worker_apply_shard_ddl_command') THEN
      RAISE 'cannot install citus extension in Citus 4 data directory';
   END IF;
END;
$$;

/*****************************************************************************
 * Citus data types
 *****************************************************************************/
CREATE TYPE citus.distribution_type AS ENUM (
   'hash',
   'range',
   'append'
);


/*****************************************************************************
 * Citus tables & corresponding indexes
 *****************************************************************************/
CREATE TABLE citus.pg_dist_partition(
    logicalrelid Oid NOT NULL,    /* type changed to regclass as of version 6.0-1 */
    partmethod "char" NOT NULL,
    partkey text NOT NULL
);
/* SELECT granted to PUBLIC in upgrade script */
CREATE UNIQUE INDEX pg_dist_partition_logical_relid_index
ON citus.pg_dist_partition using btree(logicalrelid);
ALTER TABLE citus.pg_dist_partition SET SCHEMA pg_catalog;

CREATE TABLE citus.pg_dist_shard(
    logicalrelid oid NOT NULL,    /* type changed to regclass as of version 6.0-1 */
    shardid int8 NOT NULL,
    shardstorage "char" NOT NULL,
    shardalias text,
    shardminvalue text,
    shardmaxvalue text
);
/* SELECT granted to PUBLIC in upgrade script */
CREATE UNIQUE INDEX pg_dist_shard_shardid_index
ON citus.pg_dist_shard using btree(shardid);
CREATE INDEX pg_dist_shard_logical_relid_index
ON citus.pg_dist_shard using btree(logicalrelid);
ALTER TABLE citus.pg_dist_shard SET SCHEMA pg_catalog;

CREATE TABLE citus.pg_dist_shard_placement(
    shardid int8 NOT NULL,
    shardstate int4 NOT NULL,
    shardlength int8 NOT NULL,
    nodename text NOT NULL,
    nodeport int8 NOT NULL
) WITH oids;
/* SELECT granted to PUBLIC in upgrade script */
CREATE UNIQUE INDEX pg_dist_shard_placement_oid_index
ON citus.pg_dist_shard_placement using btree(oid);
CREATE INDEX pg_dist_shard_placement_shardid_index
ON citus.pg_dist_shard_placement using btree(shardid);
CREATE INDEX pg_dist_shard_placement_nodeid_index
ON citus.pg_dist_shard_placement using btree(nodename, nodeport);
ALTER TABLE citus.pg_dist_shard_placement SET SCHEMA pg_catalog;


/*****************************************************************************
 * Citus sequences
 *****************************************************************************/

/*
 * Unternal sequence to generate 64-bit shard ids. These identifiers are then
 * used to identify shards in the distributed database.
 */
CREATE SEQUENCE citus.pg_dist_shardid_seq
    MINVALUE 102008
    NO CYCLE;
ALTER SEQUENCE  citus.pg_dist_shardid_seq SET SCHEMA pg_catalog;

/*
 * internal sequence to generate 32-bit jobIds. These identifiers are then
 * used to identify jobs in the distributed database; and they wrap at 32-bits
 * to allow for slave nodes to independently execute their distributed jobs.
 */
CREATE SEQUENCE citus.pg_dist_jobid_seq
    MINVALUE 2 /* first jobId reserved for clean up jobs */
    MAXVALUE 4294967296;
ALTER SEQUENCE  citus.pg_dist_jobid_seq SET SCHEMA pg_catalog;


/*****************************************************************************
 * Citus functions
 *****************************************************************************/

/* For backward compatibility and ease of use create functions et al. in pg_catalog */
SET search_path = 'pg_catalog';

/* master_* functions */

CREATE FUNCTION master_get_table_metadata(relation_name text, OUT logical_relid oid,
                                          OUT part_storage_type "char",
                                          OUT part_method "char", OUT part_key text,
                                          OUT part_replica_count integer,
                                          OUT part_max_size bigint,
                                          OUT part_placement_policy integer)
    RETURNS record
    LANGUAGE C STABLE STRICT
    AS 'MODULE_PATHNAME', $$master_get_table_metadata$$;
COMMENT ON FUNCTION master_get_table_metadata(relation_name text)
    IS 'fetch metadata values for the table';

CREATE FUNCTION master_get_table_ddl_events(text)
    RETURNS SETOF text
    LANGUAGE C STRICT ROWS 100
    AS 'MODULE_PATHNAME', $$master_get_table_ddl_events$$;
COMMENT ON FUNCTION master_get_table_ddl_events(text)
    IS 'fetch set of ddl statements for the table';

CREATE FUNCTION master_get_new_shardid()
    RETURNS bigint
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_get_new_shardid$$;
COMMENT ON FUNCTION master_get_new_shardid()
    IS 'fetch unique shardId';

CREATE FUNCTION master_create_empty_shard(text)
    RETURNS bigint
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_create_empty_shard$$;
COMMENT ON FUNCTION master_create_empty_shard(text)
    IS 'create an empty shard and shard placements for the table';

CREATE FUNCTION master_append_table_to_shard(bigint, text, text, integer)
    RETURNS real
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_append_table_to_shard$$;
COMMENT ON FUNCTION master_append_table_to_shard(bigint, text, text, integer)
    IS 'append given table to all shard placements and update metadata';

CREATE FUNCTION master_drop_all_shards(logicalrelid regclass,
                                       schema_name text,
                                       table_name text)
    RETURNS integer
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_drop_all_shards$$;
COMMENT ON FUNCTION master_drop_all_shards(regclass, text, text)
    IS 'drop all shards in a relation and update metadata';

CREATE FUNCTION master_apply_delete_command(text)
    RETURNS integer
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_apply_delete_command$$;
COMMENT ON FUNCTION master_apply_delete_command(text)
    IS 'drop shards matching delete criteria and update metadata';

CREATE FUNCTION master_get_active_worker_nodes(OUT node_name text, OUT node_port bigint)
    RETURNS SETOF record
    LANGUAGE C STRICT ROWS 100
    AS 'MODULE_PATHNAME', $$master_get_active_worker_nodes$$;
COMMENT ON FUNCTION master_get_active_worker_nodes()
    IS 'fetch set of active worker nodes';

CREATE FUNCTION master_create_distributed_table(table_name regclass,
                                                distribution_column text,
                                                distribution_method citus.distribution_type)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_create_distributed_table$$;
COMMENT ON FUNCTION master_create_distributed_table(table_name regclass,
                                                    distribution_column text,
                                                    distribution_method citus.distribution_type)
    IS 'define the table distribution functions';

-- define shard creation function for hash-partitioned tables
CREATE FUNCTION master_create_worker_shards(table_name text, shard_count integer,
                                            replication_factor integer DEFAULT 2)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

/* task_tracker_* functions */

CREATE FUNCTION task_tracker_assign_task(bigint, integer, text)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$task_tracker_assign_task$$;
COMMENT ON FUNCTION task_tracker_assign_task(bigint, integer, text)
    IS 'assign a task to execute';

CREATE FUNCTION task_tracker_task_status(bigint, integer)
    RETURNS integer
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$task_tracker_task_status$$;
COMMENT ON FUNCTION task_tracker_task_status(bigint, integer)
    IS 'check an assigned task''s execution status';

CREATE FUNCTION task_tracker_cleanup_job(bigint)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$task_tracker_cleanup_job$$;
COMMENT ON FUNCTION task_tracker_cleanup_job(bigint)
    IS 'clean up all tasks associated with a job';


/* worker_* functions */

CREATE FUNCTION worker_fetch_partition_file(bigint, integer, integer, integer, text,
                                            integer)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_partition_file$$;
COMMENT ON FUNCTION worker_fetch_partition_file(bigint, integer, integer, integer, text,
                                                integer)
    IS 'fetch partition file from remote node';

CREATE FUNCTION worker_fetch_query_results_file(bigint, integer, integer, text, integer)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_query_results_file$$;
COMMENT ON FUNCTION worker_fetch_query_results_file(bigint, integer, integer, text,
                                                    integer)
    IS 'fetch query results file from remote node';

CREATE FUNCTION worker_fetch_foreign_file(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_foreign_file$$;
COMMENT ON FUNCTION worker_fetch_foreign_file(text, bigint, text[], integer[])
    IS 'fetch foreign file from remote node and apply file';

CREATE FUNCTION worker_fetch_regular_table(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_regular_table$$;
COMMENT ON FUNCTION worker_fetch_regular_table(text, bigint, text[], integer[])
    IS 'fetch PostgreSQL table from remote node';

CREATE FUNCTION worker_range_partition_table(bigint, integer, text, text, oid, anyarray)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_range_partition_table$$;
COMMENT ON FUNCTION worker_range_partition_table(bigint, integer, text, text, oid,
                                                 anyarray)
    IS 'range partition query results';

CREATE FUNCTION worker_hash_partition_table(bigint, integer, text, text, oid, integer)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_hash_partition_table$$;
COMMENT ON FUNCTION worker_hash_partition_table(bigint, integer, text, text, oid,
                                                integer)
    IS 'hash partition query results';

CREATE FUNCTION worker_merge_files_into_table(bigint, integer, text[], text[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_merge_files_into_table$$;
COMMENT ON FUNCTION worker_merge_files_into_table(bigint, integer, text[], text[])
    IS 'merge files into a table';

CREATE FUNCTION worker_merge_files_and_run_query(bigint, integer, text, text)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_merge_files_and_run_query$$;
COMMENT ON FUNCTION worker_merge_files_and_run_query(bigint, integer, text, text)
    IS 'merge files and run a reduce query on merged files';

CREATE FUNCTION worker_cleanup_job_schema_cache()
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_cleanup_job_schema_cache$$;
COMMENT ON FUNCTION worker_cleanup_job_schema_cache()
    IS 'cleanup all job schemas in current database';

CREATE FUNCTION worker_foreign_file_path(text)
    RETURNS text
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_foreign_file_path$$;
COMMENT ON FUNCTION worker_foreign_file_path(text)
    IS 'get a foreign table''s local file path';

CREATE FUNCTION worker_find_block_local_path(bigint, text[])
    RETURNS text
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_find_block_local_path$$;
COMMENT ON FUNCTION worker_find_block_local_path(bigint, text[])
    IS 'find an HDFS block''s local file path';

CREATE FUNCTION worker_apply_shard_ddl_command(bigint, text)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_apply_shard_ddl_command$$;
COMMENT ON FUNCTION worker_apply_shard_ddl_command(bigint, text)
    IS 'extend ddl command with shardId and apply on database';

CREATE FUNCTION worker_append_table_to_shard(text, text, text, integer)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_append_table_to_shard$$;
COMMENT ON FUNCTION worker_append_table_to_shard(text, text, text, integer)
    IS 'append a regular table''s contents to the shard';


/* trigger functions */

CREATE OR REPLACE FUNCTION citus_drop_trigger()
    RETURNS event_trigger
    LANGUAGE plpgsql
    SET search_path = pg_catalog
    /* declared as SECURITY DEFINER in upgrade script */
    AS $cdbdt$
DECLARE v_obj record;
BEGIN
    FOR v_obj IN SELECT * FROM pg_event_trigger_dropped_objects() LOOP
        IF v_obj.object_type NOT IN ('table', 'foreign table') THEN
           CONTINUE;
        END IF;

        -- nothing to do if not a distributed table
        IF NOT EXISTS(SELECT * FROM pg_dist_partition WHERE logicalrelid = v_obj.objid) THEN
            CONTINUE;
        END IF;

        -- ensure all shards are dropped
        PERFORM master_drop_all_shards(v_obj.objid, v_obj.schema_name, v_obj.object_name);

        -- delete partition entry
        DELETE FROM pg_dist_partition WHERE logicalrelid = v_obj.objid;

    END LOOP;
END;
$cdbdt$;
COMMENT ON FUNCTION citus_drop_trigger()
    IS 'perform checks and actions at the end of DROP actions';

CREATE FUNCTION master_dist_partition_cache_invalidate()
    RETURNS trigger
    LANGUAGE C
    AS 'MODULE_PATHNAME', $$master_dist_partition_cache_invalidate$$;
COMMENT ON FUNCTION master_dist_partition_cache_invalidate()
    IS 'register relcache invalidation for changed rows';

CREATE FUNCTION master_dist_shard_cache_invalidate()
    RETURNS trigger
    LANGUAGE C
    AS 'MODULE_PATHNAME', $$master_dist_shard_cache_invalidate$$;
COMMENT ON FUNCTION master_dist_shard_cache_invalidate()
    IS 'register relcache invalidation for changed rows';


/* internal functions, not user accessible */

CREATE FUNCTION citus_extradata_container(INTERNAL)
    RETURNS void
    LANGUAGE C
    AS 'MODULE_PATHNAME', $$citus_extradata_container$$;
COMMENT ON FUNCTION pg_catalog.citus_extradata_container(INTERNAL)
    IS 'placeholder function to store additional data in postgres node trees';


/*****************************************************************************
 * Citus triggers
 *****************************************************************************/

CREATE EVENT TRIGGER citus_cascade_to_partition
    ON SQL_DROP
    EXECUTE PROCEDURE citus_drop_trigger();

CREATE TRIGGER dist_partition_cache_invalidate
    AFTER INSERT OR UPDATE OR DELETE
    ON pg_catalog.pg_dist_partition
    FOR EACH ROW EXECUTE PROCEDURE master_dist_partition_cache_invalidate();

CREATE TRIGGER dist_shard_cache_invalidate
    AFTER INSERT OR UPDATE OR DELETE
    ON pg_catalog.pg_dist_shard
    FOR EACH ROW EXECUTE PROCEDURE master_dist_shard_cache_invalidate();


/*****************************************************************************
 * Citus aggregates
 *****************************************************************************/
CREATE AGGREGATE array_cat_agg(anyarray) (SFUNC = array_cat, STYPE = anyarray);
COMMENT ON AGGREGATE array_cat_agg(anyarray)
    IS 'concatenate input arrays into a single array';


/*
 * Creates a temporary table exactly like the specified target table along with
 * a trigger to redirect any INSERTed rows from the proxy to the underlying
 * table. Users may optionally provide a sequence which will be incremented
 * after each row that has been successfully proxied (useful for counting rows
 * processed). Returns the name of the proxy table that was created.
 */
CREATE FUNCTION create_insert_proxy_for_table(target_table regclass,
                                              sequence regclass DEFAULT NULL)
RETURNS text
AS $create_insert_proxy_for_table$
    DECLARE
        temp_table_name text;
        attr_names text[];
        attr_list text;
        param_list text;
        using_list text;
        insert_command text;
        -- templates to create dynamic functions, tables, and triggers
        func_tmpl CONSTANT text :=    $$CREATE FUNCTION pg_temp.copy_to_insert()
                                        RETURNS trigger
                                        AS $copy_to_insert$
                                        BEGIN
                                            EXECUTE %L USING %s;
                                            PERFORM nextval(%L);
                                            RETURN NULL;
                                        END;
                                        $copy_to_insert$ LANGUAGE plpgsql;$$;
        table_tmpl CONSTANT text :=   $$CREATE TEMPORARY TABLE %I
                                        (LIKE %s INCLUDING DEFAULTS)$$;
        trigger_tmpl CONSTANT text := $$CREATE TRIGGER copy_to_insert
                                        BEFORE INSERT ON %s FOR EACH ROW
                                        EXECUTE PROCEDURE pg_temp.copy_to_insert()$$;
    BEGIN
        -- create name of temporary table using unqualified input table name
        SELECT format('%s_insert_proxy', relname)
        INTO   STRICT temp_table_name
        FROM   pg_class
        WHERE  oid = target_table;

        -- get list of all attributes in table, we'll need shortly
        SELECT array_agg(attname)
        INTO   STRICT attr_names
        FROM   pg_attribute
        WHERE  attrelid = target_table AND
               attnum > 0 AND
               NOT attisdropped;

        -- build fully specified column list and USING clause from attr. names
        SELECT string_agg(quote_ident(attr_name), ','),
               string_agg(format('NEW.%I', attr_name), ',')
        INTO   STRICT attr_list,
                      using_list
        FROM   unnest(attr_names) AS attr_name;

        -- build ($1, $2, $3)-style VALUE list to bind parameters
        SELECT string_agg('$' || param_num, ',')
        INTO   STRICT param_list
        FROM   generate_series(1, array_length(attr_names, 1)) AS param_num;

        -- use the above lists to generate appropriate INSERT command
        insert_command = format('INSERT INTO %s (%s) VALUES (%s)', target_table,
                                attr_list, param_list);

        -- use the command to make one-off trigger targeting specified table
        EXECUTE format(func_tmpl, insert_command, using_list, sequence);

        -- create a temporary table exactly like the target table...
        EXECUTE format(table_tmpl, temp_table_name, target_table);

        -- ... and install the trigger on that temporary table
        EXECUTE format(trigger_tmpl, quote_ident(temp_table_name)::regclass);

        RETURN temp_table_name;
    END;
$create_insert_proxy_for_table$ LANGUAGE plpgsql SET search_path = 'pg_catalog';

COMMENT ON FUNCTION create_insert_proxy_for_table(regclass, regclass)
        IS 'create a proxy table that redirects INSERTed rows to a target table';

-- define shard repair function
CREATE FUNCTION master_copy_shard_placement(shard_id bigint,
                                            source_node_name text,
                                            source_node_port integer,
                                            target_node_name text,
                                            target_node_port integer)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT;

RESET search_path;
/* citus--5.0--5.0-1.sql */

ALTER FUNCTION pg_catalog.citus_drop_trigger() SECURITY DEFINER;

GRANT SELECT ON pg_catalog.pg_dist_partition TO public;
GRANT SELECT ON pg_catalog.pg_dist_shard TO public;
GRANT SELECT ON pg_catalog.pg_dist_shard_placement TO public;
/* citus--5.0-1--5.0-2.sql */

CREATE FUNCTION master_update_shard_statistics(shard_id bigint)
    RETURNS bigint
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_update_shard_statistics$$;
COMMENT ON FUNCTION master_update_shard_statistics(bigint)
    IS 'updates shard statistics and returns the updated shard size';
/* citus--5.0-2--5.1-1.sql */

/* empty, but required to update the extension version */
CREATE FUNCTION pg_catalog.master_modify_multiple_shards(text)
    RETURNS integer
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_modify_multiple_shards$$;
COMMENT ON FUNCTION master_modify_multiple_shards(text)
    IS 'push delete and update queries to shards';DROP FUNCTION IF EXISTS public.master_update_shard_statistics(shard_id bigint);

CREATE OR REPLACE FUNCTION pg_catalog.master_update_shard_statistics(shard_id bigint)
    RETURNS bigint
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_update_shard_statistics$$;
COMMENT ON FUNCTION master_update_shard_statistics(bigint)
    IS 'updates shard statistics and returns the updated shard size';
DROP FUNCTION IF EXISTS pg_catalog.worker_apply_shard_ddl_command(bigint, text);

CREATE OR REPLACE FUNCTION pg_catalog.worker_apply_shard_ddl_command(bigint, text, text)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_apply_shard_ddl_command$$;
COMMENT ON FUNCTION worker_apply_shard_ddl_command(bigint, text, text)
    IS 'extend ddl command with shardId and apply on database';
DROP FUNCTION IF EXISTS pg_catalog.worker_fetch_foreign_file(text, bigint, text[], integer[]);

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_foreign_file(text, text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_foreign_file$$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_foreign_file(text, text, bigint, text[], integer[])
    IS 'fetch foreign file from remote node and apply file';

DROP FUNCTION IF EXISTS pg_catalog.worker_fetch_regular_table(text, bigint, text[], integer[]);

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_regular_table(text, text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_regular_table$$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_regular_table(text, text, bigint, text[], integer[])
    IS 'fetch PostgreSQL table from remote node';
CREATE OR REPLACE FUNCTION pg_catalog.worker_apply_shard_ddl_command(bigint, text)
    RETURNS void
    LANGUAGE sql
AS $worker_apply_shard_ddl_command$
    SELECT pg_catalog.worker_apply_shard_ddl_command($1, 'public', $2);
$worker_apply_shard_ddl_command$;
COMMENT ON FUNCTION worker_apply_shard_ddl_command(bigint, text)
    IS 'extend ddl command with shardId and apply on database';

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_foreign_file(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE sql
AS $worker_fetch_foreign_file$
    SELECT pg_catalog.worker_fetch_foreign_file('public', $1, $2, $3, $4);
$worker_fetch_foreign_file$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_foreign_file(text, bigint, text[], integer[])
    IS 'fetch foreign file from remote node and apply file';

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_regular_table(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE sql
AS $worker_fetch_regular_table$
    SELECT pg_catalog.worker_fetch_regular_table('public', $1, $2, $3, $4);
$worker_fetch_regular_table$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_regular_table(text, bigint, text[], integer[])
    IS 'fetch PostgreSQL table from remote node';
DROP FUNCTION IF EXISTS pg_catalog.worker_fetch_foreign_file(text, text, bigint, text[], integer[]);

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_foreign_file(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_foreign_file$$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_foreign_file(text, bigint, text[], integer[])
    IS 'fetch foreign file from remote node and apply file';

DROP FUNCTION IF EXISTS pg_catalog.worker_fetch_regular_table(text, text, bigint, text[], integer[]);

CREATE OR REPLACE FUNCTION pg_catalog.worker_fetch_regular_table(text, bigint, text[], integer[])
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_fetch_regular_table$$;
COMMENT ON FUNCTION pg_catalog.worker_fetch_regular_table(text, bigint, text[], integer[])
    IS 'fetch PostgreSQL table from remote node';
CREATE FUNCTION pg_catalog.master_drop_sequences(sequence_names text[],
												 node_name text,
												 node_port bigint)
	RETURNS bool
	LANGUAGE C STRICT
	AS 'MODULE_PATHNAME', $$master_drop_sequences$$;
COMMENT ON FUNCTION pg_catalog.master_drop_sequences(text[], text, bigint)
	IS 'drop specified sequences from a node';

REVOKE ALL ON FUNCTION pg_catalog.master_drop_sequences(text[], text, bigint) FROM PUBLIC;

CREATE OR REPLACE FUNCTION pg_catalog.citus_drop_trigger()
	RETURNS event_trigger
	LANGUAGE plpgsql
	SECURITY DEFINER
	SET search_path = pg_catalog
	AS $cdbdt$
DECLARE
	v_obj record;
	sequence_names text[] := '{}';
	node_names text[] := '{}';
	node_ports bigint[] := '{}';
	node_name text;
	node_port bigint;
BEGIN
	-- collect set of dropped sequences to drop on workers later
	SELECT array_agg(object_identity) INTO sequence_names
	FROM pg_event_trigger_dropped_objects()
	WHERE object_type = 'sequence';

	-- Must accumulate set of affected nodes before deleting placements, as
	-- master_drop_all_shards will erase their rows, making it impossible for
	-- us to know where to drop sequences (which must be dropped after shards,
	-- since they have default value expressions which depend on sequences).
	SELECT array_agg(sp.nodename), array_agg(sp.nodeport)
	INTO node_names, node_ports
	FROM pg_event_trigger_dropped_objects() AS dobj,
		 pg_dist_shard AS s,
		 pg_dist_shard_placement AS sp
	WHERE dobj.object_type IN ('table', 'foreign table')
	  AND dobj.objid = s.logicalrelid
	  AND s.shardid = sp.shardid;

	FOR v_obj IN SELECT * FROM pg_event_trigger_dropped_objects() LOOP
		IF v_obj.object_type NOT IN ('table', 'foreign table') THEN
		   CONTINUE;
		END IF;

		-- nothing to do if not a distributed table
		IF NOT EXISTS(SELECT * FROM pg_dist_partition WHERE logicalrelid = v_obj.objid) THEN
			CONTINUE;
		END IF;

		-- ensure all shards are dropped
		PERFORM master_drop_all_shards(v_obj.objid, v_obj.schema_name, v_obj.object_name);

		-- delete partition entry
		DELETE FROM pg_dist_partition WHERE logicalrelid = v_obj.objid;
	END LOOP;

	IF cardinality(sequence_names) = 0 THEN
		RETURN;
	END IF;

	FOR node_name, node_port IN
	SELECT DISTINCT name, port
	FROM unnest(node_names, node_ports) AS nodes(name, port)
	LOOP
		PERFORM master_drop_sequences(sequence_names, node_name, node_port);
	END LOOP;
END;
$cdbdt$;
/* citus--5.1-8--5.2-1.sql */

/* empty, but required to update the extension version */
/* citus--5.2-1--5.2-2.sql */

CREATE OR REPLACE FUNCTION pg_catalog.citus_truncate_trigger()
	RETURNS trigger
	LANGUAGE plpgsql
	SET search_path = 'pg_catalog'
	AS $cdbtt$
DECLARE
	partitionType char;
	commandText text;
BEGIN
	SELECT partmethod INTO partitionType
	FROM pg_dist_partition WHERE logicalrelid = TG_RELID;
	IF NOT FOUND THEN
		RETURN NEW;
	END IF;
	
	IF (partitionType = 'a') THEN
		PERFORM master_drop_all_shards(TG_RELID, TG_TABLE_SCHEMA, TG_TABLE_NAME);
	ELSE
		SELECT format('TRUNCATE TABLE %I.%I CASCADE', TG_TABLE_SCHEMA, TG_TABLE_NAME)
		INTO commandText;
		PERFORM master_modify_multiple_shards(commandText);
	END IF;

	RETURN NEW;
END;
$cdbtt$;
/* citus--5.2-2--5.2-3.sql */
CREATE OR REPLACE FUNCTION master_expire_table_cache(table_name regclass)
	RETURNS VOID
	LANGUAGE C STRICT
	AS 'MODULE_PATHNAME', $$master_expire_table_cache$$;
/* citus--5.2-3--5.2-4.sql */

ALTER TABLE pg_dist_partition ADD COLUMN colocationid BIGINT DEFAULT 0 NOT NULL;

CREATE INDEX pg_dist_partition_colocationid_index
ON pg_dist_partition using btree(colocationid);

/* citus--5.2-4--6.0-1.sql */

/* change logicalrelid type to regclass to allow implicit casts to text */
ALTER TABLE pg_catalog.pg_dist_partition ALTER COLUMN logicalrelid TYPE regclass;
ALTER TABLE pg_catalog.pg_dist_shard ALTER COLUMN logicalrelid TYPE regclass;
/* citus--6.0-1--6.0-2.sql */

CREATE FUNCTION pg_catalog.shard_name(object_name regclass, shard_id bigint)
    RETURNS text
    LANGUAGE C STABLE
    AS 'MODULE_PATHNAME', $$shard_name$$;
COMMENT ON FUNCTION pg_catalog.shard_name(object_name regclass, shard_id bigint)
    IS 'returns shard-extended version of object name';

/* citus--6.0-2--6.0-3.sql */

ALTER TABLE pg_catalog.pg_dist_partition
ADD COLUMN repmodel "char" DEFAULT 'c' NOT NULL;
SET search_path = 'pg_catalog';

CREATE SEQUENCE citus.pg_dist_groupid_seq
	MINVALUE 1
	MAXVALUE 4294967296;

CREATE SEQUENCE citus.pg_dist_node_nodeid_seq
	MINVALUE 1
	MAXVALUE 4294967296;

ALTER SEQUENCE citus.pg_dist_groupid_seq SET SCHEMA pg_catalog;
ALTER SEQUENCE citus.pg_dist_node_nodeid_seq SET SCHEMA pg_catalog;

/* add pg_dist_node */
CREATE TABLE citus.pg_dist_node(
	nodeid int NOT NULL DEFAULT nextval('pg_dist_groupid_seq') PRIMARY KEY,
	groupid int NOT NULL DEFAULT nextval('pg_dist_node_nodeid_seq'),
	nodename text NOT NULL,
	nodeport int NOT NULL DEFAULT 5432,
	noderack text NOT NULL DEFAULT 'default',
	UNIQUE (nodename, nodeport)
);

ALTER TABLE citus.pg_dist_node SET SCHEMA pg_catalog;

CREATE FUNCTION master_dist_node_cache_invalidate()
	RETURNS trigger
	LANGUAGE C
	AS 'MODULE_PATHNAME', $$master_dist_node_cache_invalidate$$;
COMMENT ON FUNCTION master_dist_node_cache_invalidate()
	IS 'invalidate internal cache of nodes when pg_dist_nodes changes';
CREATE TRIGGER dist_node_cache_invalidate
    AFTER INSERT OR UPDATE OR DELETE
    ON pg_catalog.pg_dist_node
    FOR EACH ROW EXECUTE PROCEDURE master_dist_node_cache_invalidate();

CREATE FUNCTION master_add_node(nodename text,
								nodeport integer)
	RETURNS record
	LANGUAGE C STRICT
	AS 'MODULE_PATHNAME', $$master_add_node$$;
COMMENT ON FUNCTION master_add_node(nodename text,
									nodeport integer)
	IS 'add node to the cluster';

CREATE FUNCTION master_remove_node(nodename text, nodeport integer)
	RETURNS void
	LANGUAGE C STRICT
	AS 'MODULE_PATHNAME', $$master_remove_node$$;
COMMENT ON FUNCTION master_remove_node(nodename text, nodeport integer)
	IS 'remove node from the cluster';

/* this only needs to run once, now. */
CREATE FUNCTION master_initialize_node_metadata()
    RETURNS BOOL
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_initialize_node_metadata$$;

SELECT master_initialize_node_metadata();

RESET search_path;
/*
 * Replace oid column in pg_dist_shard_placement with a sequence column.
 */
CREATE SEQUENCE citus.pg_dist_shard_placement_placementid_seq
    NO CYCLE;
ALTER SEQUENCE citus.pg_dist_shard_placement_placementid_seq
    SET SCHEMA pg_catalog;
ALTER TABLE pg_catalog.pg_dist_shard_placement
    ADD COLUMN placementid bigint;
-- keep existing oids, and update sequence to match max.
UPDATE pg_catalog.pg_dist_shard_placement SET placementid = oid;
ALTER TABLE pg_catalog.pg_dist_shard_placement
    ALTER COLUMN placementid SET DEFAULT nextval('pg_catalog.pg_dist_shard_placement_placementid_seq'),
    ALTER COLUMN placementid SET NOT NULL,
    SET WITHOUT OIDS;
CREATE UNIQUE INDEX pg_dist_shard_placement_placementid_index
ON pg_catalog.pg_dist_shard_placement using btree(placementid);
SELECT setval('pg_catalog.pg_dist_shard_placement_placementid_seq', max(placementid))
FROM pg_catalog.pg_dist_shard_placement;

CREATE FUNCTION master_get_new_placementid()
    RETURNS bigint
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_get_new_placementid$$;
COMMENT ON FUNCTION master_get_new_placementid()
    IS 'fetch unique placementid';

CREATE FUNCTION worker_drop_distributed_table(logicalrelid Oid)
    RETURNS VOID
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_drop_distributed_table$$;
    
COMMENT ON FUNCTION worker_drop_distributed_table(logicalrelid Oid)
    IS 'drop the clustered table and its reference from metadata tables';
    
CREATE FUNCTION column_name_to_column(table_name regclass, column_name text)
    RETURNS text
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$column_name_to_column$$;
COMMENT ON FUNCTION column_name_to_column(table_name regclass, column_name text)
    IS 'convert a column name to its textual Var representation';
/* citus--6.0-6--6.0-7.sql */

CREATE FUNCTION pg_catalog.get_colocated_table_array(regclass)
    RETURNS regclass[]
    AS 'citus'
    LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION pg_catalog.master_move_shard_placement(shard_id bigint,
													   source_node_name text,
													   source_node_port integer,
													   target_node_name text,
													   target_node_port integer)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$master_move_shard_placement$$;
COMMENT ON FUNCTION pg_catalog.master_move_shard_placement(shard_id bigint,
													   source_node_name text,
													   source_node_port integer,
													   target_node_name text,
													   target_node_port integer)
    IS 'move shard from remote node';
/*
 * Drop shardalias from pg_dist_shard
 */
ALTER TABLE pg_dist_shard DROP shardalias;
/* citus--6.0-8--6.0-9.sql */

CREATE TABLE citus.pg_dist_local_group(
    groupid int NOT NULL PRIMARY KEY)
;

/* insert the default value for being the coordinator node */
INSERT INTO citus.pg_dist_local_group VALUES (0);

ALTER TABLE citus.pg_dist_local_group SET SCHEMA pg_catalog;
GRANT SELECT ON pg_catalog.pg_dist_local_group TO public;

ALTER TABLE pg_catalog.pg_dist_node ADD COLUMN hasmetadata bool NOT NULL DEFAULT false;
/* citus--6.0-9--6.0-10.sql */

CREATE TABLE citus.pg_dist_transaction (
    groupid int NOT NULL,
    gid text NOT NULL
);

CREATE INDEX pg_dist_transaction_group_index
ON citus.pg_dist_transaction using btree(groupid);

ALTER TABLE citus.pg_dist_transaction SET SCHEMA pg_catalog;
ALTER TABLE pg_catalog.pg_dist_transaction
ADD CONSTRAINT pg_dist_transaction_unique_constraint UNIQUE (groupid, gid);

GRANT SELECT ON pg_catalog.pg_dist_transaction TO public;

CREATE FUNCTION recover_prepared_transactions()
    RETURNS int
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$recover_prepared_transactions$$;

COMMENT ON FUNCTION recover_prepared_transactions()
    IS 'recover prepared transactions started by this node';

/* citus--6.0-10--6.0-11.sql */

SET search_path = 'pg_catalog';

CREATE SEQUENCE citus.pg_dist_colocationid_seq
	MINVALUE 1
	MAXVALUE 4294967296;

ALTER SEQUENCE citus.pg_dist_colocationid_seq SET SCHEMA pg_catalog;

/* add pg_dist_colocation */
CREATE TABLE citus.pg_dist_colocation(
	colocationid int NOT NULL PRIMARY KEY,
	shardcount int NOT NULL,
	replicationfactor int NOT NULL,
	distributioncolumntype oid NOT NULL
);

ALTER TABLE citus.pg_dist_colocation SET SCHEMA pg_catalog;

CREATE INDEX pg_dist_colocation_configuration_index
ON pg_dist_colocation USING btree(shardcount, replicationfactor, distributioncolumntype);

CREATE FUNCTION create_distributed_table(table_name regclass,
										 distribution_column text,
										 distribution_type citus.distribution_type DEFAULT 'hash')
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$create_distributed_table$$;
COMMENT ON FUNCTION create_distributed_table(table_name regclass,
											 distribution_column text,
											 distribution_type citus.distribution_type)
    IS 'creates a distributed table';


CREATE OR REPLACE FUNCTION pg_catalog.citus_drop_trigger()
    RETURNS event_trigger
    LANGUAGE plpgsql
    SECURITY DEFINER
    SET search_path = pg_catalog
    AS $cdbdt$
DECLARE
    v_obj record;
    sequence_names text[] := '{}';
    node_names text[] := '{}';
    node_ports bigint[] := '{}';
    node_name text;
    node_port bigint;
    table_colocation_id integer;
BEGIN
    -- collect set of dropped sequences to drop on workers later
    SELECT array_agg(object_identity) INTO sequence_names
    FROM pg_event_trigger_dropped_objects()
    WHERE object_type = 'sequence';

    -- Must accumulate set of affected nodes before deleting placements, as
    -- master_drop_all_shards will erase their rows, making it impossible for
    -- us to know where to drop sequences (which must be dropped after shards,
    -- since they have default value expressions which depend on sequences).
    SELECT array_agg(sp.nodename), array_agg(sp.nodeport)
    INTO node_names, node_ports
    FROM pg_event_trigger_dropped_objects() AS dobj,
         pg_dist_shard AS s,
         pg_dist_shard_placement AS sp
    WHERE dobj.object_type IN ('table', 'foreign table')
      AND dobj.objid = s.logicalrelid
      AND s.shardid = sp.shardid;

    FOR v_obj IN SELECT * FROM pg_event_trigger_dropped_objects() LOOP
        IF v_obj.object_type NOT IN ('table', 'foreign table') THEN
           CONTINUE;
        END IF;

        -- nothing to do if not a distributed table
        IF NOT EXISTS(SELECT * FROM pg_dist_partition WHERE logicalrelid = v_obj.objid) THEN
            CONTINUE;
        END IF;

        -- ensure all shards are dropped
        PERFORM master_drop_all_shards(v_obj.objid, v_obj.schema_name, v_obj.object_name);

        -- get colocation group
        SELECT colocationid INTO table_colocation_id FROM pg_dist_partition WHERE logicalrelid = v_obj.objid;

        -- delete partition entry
        DELETE FROM pg_dist_partition WHERE logicalrelid = v_obj.objid;

        -- drop colocation group if all referencing tables are dropped
        IF NOT EXISTS(SELECT * FROM pg_dist_partition WHERE colocationId = table_colocation_id) THEN
            DELETE FROM pg_dist_colocation WHERE colocationId = table_colocation_id;
        END IF;
    END LOOP;

    IF cardinality(sequence_names) = 0 THEN
        RETURN;
    END IF;

    FOR node_name, node_port IN
    SELECT DISTINCT name, port
    FROM unnest(node_names, node_ports) AS nodes(name, port)
    LOOP
        PERFORM master_drop_sequences(sequence_names, node_name, node_port);
    END LOOP;
END;
$cdbdt$;

COMMENT ON FUNCTION citus_drop_trigger()
    IS 'perform checks and actions at the end of DROP actions';

ALTER TABLE pg_dist_partition ALTER COLUMN colocationid TYPE integer;

RESET search_path;
/* citus--6.0-11--6.0-12.sql */

SET search_path = 'pg_catalog';

CREATE FUNCTION create_reference_table(table_name regclass)
	RETURNS void
	LANGUAGE C STRICT
	AS 'MODULE_PATHNAME', $$create_reference_table$$;
COMMENT ON FUNCTION create_reference_table(table_name regclass)
	IS 'create a distributed reference table';

RESET search_path;
/* citus--6.0-12--6.0-13.sql */

CREATE FUNCTION pg_catalog.worker_apply_inter_shard_ddl_command(referencing_shard bigint,
																referencing_schema_name text,
																referenced_shard bigint,
																referenced_schema_name text,
																command text)
    RETURNS void
    LANGUAGE C STRICT
    AS 'MODULE_PATHNAME', $$worker_apply_inter_shard_ddl_command$$;
COMMENT ON FUNCTION pg_catalog.worker_apply_inter_shard_ddl_command(referencing_shard bigint,
																	referencing_schema_name text,
																	referenced_shard bigint,
																	referenced_schema_name text,
																	command text)
    IS 'executes inter shard ddl command';
/* citus--6.0-13--6.0-14.sql */

DO $ff$
BEGIN
	-- fix functions created in wrong namespace
	ALTER FUNCTION public.recover_prepared_transactions()
	SET SCHEMA pg_catalog;

	ALTER FUNCTION public.column_name_to_column(table_name regclass, column_name text)
	SET SCHEMA pg_catalog;

	ALTER FUNCTION public.worker_drop_distributed_table(logicalrelid Oid)
	SET SCHEMA pg_catalog;

	ALTER FUNCTION public.master_get_new_placementid()
	SET SCHEMA pg_catalog;

	ALTER FUNCTION public.master_expire_table_cache(table_name regclass)
	SET SCHEMA pg_catalog;

-- some installations don't need this corrective, so just skip...
EXCEPTION WHEN undefined_function THEN
	-- do nothing
END
$ff$;
/* citus--6.0-14--6.0-15.sql */


CREATE FUNCTION pg_catalog.master_dist_placement_cache_invalidate()
    RETURNS trigger
    LANGUAGE C
    AS 'MODULE_PATHNAME', $$master_dist_placement_cache_invalidate$$;
COMMENT ON FUNCTION master_dist_placement_cache_invalidate()
    IS 'register relcache invalidation for changed placements';

CREATE TRIGGER dist_placement_cache_invalidate
    AFTER INSERT OR UPDATE OR DELETE
    ON pg_catalog.pg_dist_shard_placement
    FOR EACH ROW EXECUTE PROCEDURE master_dist_placement_cache_invalidate();
