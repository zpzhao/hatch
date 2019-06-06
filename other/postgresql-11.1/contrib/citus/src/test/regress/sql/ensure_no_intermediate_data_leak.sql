
------
-- THIS TEST SHOULD IDEALLY BE EXECUTED AT THE END OF 
-- THE REGRESSION TEST SUITE  TO MAKE SURE THAT WE 
-- CLEAR ALL INTERMEDIATE RESULTS ON BOTH THE COORDINATOR 
-- AND ON THE WORKERS. HOWEVER, WE HAVE SOME ISSUES AROUND
-- WINDOWS SUPPORT, FAILURES IN TASK-TRACKER EXECUTOR
-- SO WE DISABLE THIS TEST ON WINDOWS
------

SELECT pg_ls_dir('base/pgsql_job_cache') WHERE citus_version() NOT ILIKE '%windows%';
SELECT run_command_on_workers($$SELECT pg_ls_dir('base/pgsql_job_cache') WHERE citus_version() NOT ILIKE '%windows%'$$);
