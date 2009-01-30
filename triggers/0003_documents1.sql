-- Doesn't handle other-than lowercase aliases

create or replace
trigger DOCINDEX.T_DOCUMENTS1
 before insert on DOCINDEX.DOCUMENTS 
for each row 
when (new.server like 'ibi%')
begin
  if :new.server = 'ibis' then
    :new.server := 'phoenix';
  elsif :new.server = 'ibis-pre-prod' then
    :new.server := 'mork';
  elsif :new.server = 'ibis-train' then
    :new.server := 'mork';
  elsif :new.server = 'ibis-crp' then
    :new.server := 'mork';
  elsif :new.server = 'ibis-keyuser' then
    :new.server := 'mork';
  elsif :new.server = 'ibis-devone' then
    :new.server := 'holly';
  elsif :new.server = 'ibis-migrate' then
    :new.server := 'holly';
  elsif :new.server = 'ibis-damcrap' then
    :new.server := 'holly';
  end if;  
end;
