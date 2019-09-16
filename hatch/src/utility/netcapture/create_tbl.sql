-- netcapture packet table infomation
-- 2019/9/16 create by zzp
create table if not exists packet(
			varchar srcmac,
			varchar desmac,
			varchar protocol, 
			varchar desip, 
			varchar srcip, 
			varchar dport,
			varchar sport);
