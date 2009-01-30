BEGIN {
  FS=":";
  OFS="";
  print "delete from ibis.unix_passwd where hostname = '" ENVIRON["HOST"] "';";
}
{
  print "insert into ibis.unix_passwd values ('" ENVIRON["HOST"] "','" $1 "',(select groupname from ibis.unix_group where hostname = '" ENVIRON["HOST"] "' and groupid = " $4 ")," $3 "," $4 ",'" $6 "','" $7 "','" $5 "',null,null,null);";
}
END {
  print "truncate table ibis.unix_user;";
  print "insert into ibis.unix_user select distinct(username),gecos from ibis.unix_passwd;";
  print "commit;";
}
