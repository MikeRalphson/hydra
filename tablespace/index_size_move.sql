-- routine to move indexes into tablespaces based on their sizes

DECLARE
  CURSOR c_index IS
    select sum(bytes)/1048576 as MB,case
    when sum(bytes)/1048576 <= 1 then 'SML'
    when sum(bytes)/1048576 <= 10 then 'MED'
    else 'LGE'
    end as siz, 
    segment_name,owner,tablespace_name from dba_extents where
    --segment_name like 'I_%' and 
    tablespace_name like 'STX_%' and
    segment_type = 'INDEX'
    group by owner,segment_name,tablespace_name order by MB;
  stmt VARCHAR2(2048);
  dest VARCHAR2(30);
BEGIN
  FOR i_loop IN c_index
  LOOP
    dest := 'STX_IDX_' || i_loop.siz;
    IF i_loop.owner = 'IBIS' THEN
      dest := 'IBIS_DATA_1';
    END IF;
    IF dest != i_loop.tablespace_name THEN
      stmt := 'alter index ' || i_loop.owner || '.' || i_loop.segment_name || 
      ' rebuild tablespace ' || dest;
      DBMS_OUTPUT.PUT_LINE(stmt);
      EXECUTE IMMEDIATE stmt;
    END IF;
  END LOOP;
END;
/
