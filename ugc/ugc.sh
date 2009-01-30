#!/bin/sh
export HOST=`hostname`
export DATE=`date +%Y-%m-%d`
BIN=`dirname $0`

outfile=${IBIS_CONTROL}/output/ugc_${HOST}.sql

echo Creating $outfile
echo Reading /etc/group ...
awk -f ${BIN}/ugc_group.awk < /etc/group > $outfile

echo Reading /etc/passwd ...
awk -f ${BIN}/ugc_passwd.awk < /etc/passwd >> $outfile

if test -r /etc/security/passwd
then
  echo Reading /etc/security/passwd ...
  awk -f ${BIN}/ugc_shadow.awk < /etc/security/passwd >> $outfile
else
  echo Skipping shadow password file
fi
