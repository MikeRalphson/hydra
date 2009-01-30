create or replace
procedure ibis.p_purge_stschlst as
begin
  delete from (select stsch_loc,stsch_product from agsh.stschlst 
  left join agsh.stloc on stloc_loc = stsch_loc where
  (length(trim(stloc_ccomp)) != 0) or (length(trim(stloc_customer)) != 0));
end;
