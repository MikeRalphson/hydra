BEGIN {
  FS = "=|:|[ \t]+"
  user = "";
  password = "";
  lastupdate = "";
  flags = "";
}
function report() {
  if (user != "") {
    print "update ibis.unix_passwd set password='" password "',lastupdate=" lastupdate ",flags='" flags "' where username='" user "' and hostname='" ENVIRON["HOST"] "';";
    user = "";
  }
}
{ 
  #print $1 "," $2 "," $3 "," $4 "," $5;
  if ($1 != "") {
    report();
    user = $1;
  }
  if (match($2,"password")) {
    password = $5;
  }
  if (match($2,"lastupdate")) {
    lastupdate = $5;
  }
  if (match($2,"flags")) {
    flags = $5;
  }
}
END {
  report();
  print "commit;";
}
