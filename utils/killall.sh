#!/bin/bash
# usage: ./killall.sh file_with_machine_names username 

if [ $# -ne 2 ] 
then
  echo "usage: ./killall.sh  file_with_machinenames username"
else
  for i in `cat $1`
  do
    echo "killing on $i:"
    ssh $2@$i pkill -9 peermon
  done
fi

