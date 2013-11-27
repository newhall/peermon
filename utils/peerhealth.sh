#! /bin/bash
#This file is part of peermon
#This script checks for if the peermon daemon is running and if not it restarts
#it. The script should be run from root's cron

if ! `ps -A | grep -q peermon`
then
  /etc/init.d/peermon.sh start
fi

