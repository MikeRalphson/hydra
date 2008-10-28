#!/bin/sh
if [ $# -ge 1 ] ; then
  ipfile=`basename $1`
  rm -f $ipfile
  wget $1
  url=`grep \.ram $ipfile | grep metaFile | cut -d\" -f2`
  echo $url
  
  ramfile=`basename $url`
  rm -f $ramfile
  wget $url
  prefix=`basename $url .ram`
  url=`cat $ramfile`
  out="$2"
  if [ "$out" = "" ] ; then
    out=`date +%Y%m%d`_${prefix}.mp3
  fi

  echo $url
  mplayer -vo null -vc null -ao pcm:fast $url
  lame -V 8 audiodump.wav $out
else
  echo Usage: iplayer2mp3.sh {iplayer-console-url} '[outfile.mp3]'
fi
