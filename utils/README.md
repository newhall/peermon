This file contains the descriptions about some of the files in this directory

 *client_template.py
   -This file represents a full functioning peermon client which just gets
    peermon data and prints it out to the screen. Change CLASSNAME and implement
    CLASSFUNCTION to perform the desired functionality using peermon data
 *init.d-peermon.sh
   -sample init.d script that can be renamed to /etc/init.d/peermon.sh and is
    used to start and stop the peermon daemon running on this machine. Saving it
    in that directory ensures that peermon is always started when the machine 
    has just been turned on.
 *peerhealth.sh
   -sample script that can be run via root's cron and is used check if peermon
    is running on a particular node. If not, the script automatically starts
    peermon.
 *machines.txt
   -sample configuration file to be saved as /etc/peermon/machines.txt and has 
    host address and portnumber entries for the 3 hosts to initially send 
    peermon data out to.
 *valid_ips.txt
   -sample valid IPs configuration file. The edited version of this file should
    be stored in /etc/peermon/valid_ips.txt

*install.sh, startcommand,killall.sh, isup.sh: some example scripts to run
  -example scripts to install (intall.sh) peermon at user level (just as 
   a regular user) and to kill (killall.sh) and check (isup.sh) if it is up 
   these scripts require a file with a list of machine names on which to
   start peermon, and the example startcommand uses command line options
   specifying specific machines.txt and valid_ips.txt files) 
