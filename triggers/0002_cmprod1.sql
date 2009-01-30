-- Requires the following grants

-- GRANT CREATE ANY TRIGGER to ibis;

create or replace
trigger ibis.t_cmprod1
 before insert or update of cmp_ccat,cmp_projcode on AGSH.CMPROD 
 for each row
 when ((new.cmp_projcode is null) or
 (trim(new.cmp_projcode) != translate(new.cmp_ccat,'ABCDEFG S','ABCDDDDDS')))
begin
  :new.cmp_projcode := translate(:new.cmp_ccat,'ABCDEFG S','ABCDDDDDS');
end;
