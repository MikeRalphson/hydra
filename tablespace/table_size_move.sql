-- move tables to appropriate tablespaces based on table size
-- this script ignores our own 'side-table' schemas

DECLARE
  CURSOR c_index IS
    select sum(bytes)/1048576 as MB,case
      when sum(bytes)/1048576 < 5 then 'SML'
      when sum(bytes)/1048576 < 10 then 'MED'
      when sum(bytes)/1048576 < 25 then 'LGE'
      when sum(bytes)/1048576 < 50 then 'XLA'
      else 'XXL'
    end as dest, 
    segment_name,owner,tablespace_name from dba_extents where
    tablespace_name like 'STX_DATA%' 
    and segment_type = 'TABLE'
    and owner != 'IBIS'
    and owner != 'EXTRANET'
    and owner != 'REFERRAL'
    group by owner,segment_name,tablespace_name order by MB desc;
  stmt VARCHAR2(2048);
BEGIN
  FOR i_loop IN c_index
  LOOP
    IF SUBSTR(i_loop.tablespace_name,-3) != i_loop.dest THEN 
      stmt := 'alter table ' || i_loop.owner || '.' || i_loop.segment_name || 
      ' move tablespace STX_DATA_' || i_loop.dest;
      DBMS_OUTPUT.PUT_LINE(stmt);
      EXECUTE IMMEDIATE stmt;
    END IF;
  END LOOP;
END;
/
