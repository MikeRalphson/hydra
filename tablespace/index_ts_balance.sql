-- Script to balance the sizes of index tablespaces
-- it does this by moving indexes to the smallest tablespace
-- it expects them to start off in the data tablespaces

DECLARE
  CURSOR c_index IS
    select owner,index_name,tablespace_name from dba_indexes 
    where index_name like 'I%' and tablespace_name like '%DAT%'
    --and owner = 'MULTI' 
    order by owner,index_name;
  n_id NUMBER;
  s_smallest VARCHAR2(64);
  stmt VARCHAR2(2048);
BEGIN
  FOR i_loop IN c_index
  LOOP
    select id,tablespace_name into n_id,s_smallest from (
      select row_number() over (order by used_space) as id, 
      TABLESPACE_NAME from DBA_TABLESPACE_USAGE_METRICS 
      where tablespace_name like '%INDEX%'
    ) where id <= 1;
    stmt := 'alter index ' || i_loop.owner || '.' || i_loop.index_name || 
    ' rebuild tablespace ' || s_smallest;
    DBMS_OUTPUT.PUT_LINE(stmt);
    EXECUTE IMMEDIATE stmt;
  END LOOP;
END;
/
