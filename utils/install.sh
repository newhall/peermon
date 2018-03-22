#!/bin/bash
# usage: ./install.sh file_with_machinenames username

# set to your peermon path
peermonpath=/home/newhall/peermon

if [ $# -ne 2 ] 
then
  echo "usage: ./install.sh  file_with_machinenames username"
else
  for i in `cat $1`
  do
    echo "installing peermon on $i:"
    ssh $2@$i $peermonpath/startcommand 
  done
fi
