#!/bin/bash
# usage: ./isup.sh  file_with_machinenames
# also: change newhall to your user name

if [ $# -ne 2 ] 
then
  echo "usage: ./isup.sh  file_with_machinenames username"
else
  for i in `cat $1`
  do
    echo "checking on $i "
    ssh $2@$i ps -A | grep peermon
  done
fi

