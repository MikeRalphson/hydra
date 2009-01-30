-- This trigger depends on the following explicit grants
-- GRANT CREATE ANY TRIGGER to ibis;
-- GRANT DELETE on utool.waopgrp to ibis;
-- GRANT SELECT on agsh.fgusers to ibis;
-- GRANT INSERT on utool.waopgrp to ibis;

create or replace
trigger ibis.t_fgusers1
 after insert or delete or update of fgu_dept,fgu_user on AGSH.FGUSERS 
BEGIN
  delete from utool.waopgrp where waopg_group like 'FG_%';
  insert into utool.waopgrp (WAOPG_GROUP,WAOPG_MEMTYP,WAOPG_MEMBER) (select 'FG_'||fgu_dept, 'O', fgu_user from agsh.fgusers);
END;
/
