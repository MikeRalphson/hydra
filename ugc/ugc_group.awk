BEGIN {
  FS=":";
  OFS="";
  print "update ibis.unix_host set updated = '" ENVIRON["DATE"] "' where hostname = '" ENVIRON["HOST"] "';";
  print "delete from ibis.unix_group where hostname = '" ENVIRON["HOST"] "';";
}
{
  print "insert into ibis.unix_group values ('" $1 "','" ENVIRON["HOST"] "','" $3 "');";
}
