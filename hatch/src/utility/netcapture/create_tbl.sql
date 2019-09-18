-- netcapture packet table infomation
-- 2019/9/16 create by zzp

create table if not exists tbl_mac_packet(
			id bigint primary key,
			srcmac varchar,
			desmac varchar,
			type varchar,
			capturetime timestamp);
			
create table if not exists tbl_ip_packet(
			id bigint references tbl_mac_packet(id),
			protocol varchar, 
			desip varchar, 
			srcip varchar);
			
create table if not exists tbl_tcp_packet(
			id bigint references 	tbl_mac_packet(id),		 
			dport varchar,
			sport varchar);
