alter table ibis.EXP_AUTO_PARAMS move lob(PARAM_SQL) store as (tablespace IBIS_DATA_1);

alter table ibis.EXP_EXTRACT_TYPE move lob(EXTRACT_SQL) store as (tablespace IBIS_DATA_1);

alter table ibis.IMPORT_STATUS move lob(DETAILS) store as (tablespace IBIS_DATA_1);
