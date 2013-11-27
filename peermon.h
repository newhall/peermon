//.h file for peermon.cpp
/*
 *  PeerMon
 *  Copyright 2011 Tia Newhall, Janis Libeks, Ross Greenwood, Jeff Knerr,
 *                 Steve Dini
 *
 *  C++ Sockets on Unix and Windows  
 *  Copyright (C) 2002
 *
 *  This file is part of PeerMon.
 *
 *  PeerMon is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  PeerMon is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with PeerMon.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  PeerMon documentation: www.cs.swarthmore.edu/~newhall/peermon
 */

// set to 1 to daemonize peermon...
static const int DAEMONIZE_PEERMON=1;

// default client interface port number
static const int DEFAULT_CLIENT_PORT=1981;

// default sleep seconds for Sender thread
static const int DEFAULT_SLEEP_AMT=5;

// max size of status msg to send to peers 
static const int PEERMON_MSG_SIZE = 100;  

static const int MAX_UDP_MSG_LEN = 512;  // max size of each UDP message    
// enough space for IP:portnum string "A.B.C.D:P"
static const int IP_LEN = 32;      
// enough space for data payload w/each node
static const int DATA_LEN = 64;    
// max for each node's data
static const int MAX_SINGLE_NODE_INFO = IP_LEN+DATA_LEN;  

static const int MAX_TTL=10;
static const int HIST_SIZE=10;
static const int SEND_TO=3;

// these are default values for any node running in collector mode
#define COLLECTOR_RAM 1
#define COLLECTOR_CORES 0
#define COLLECTOR_LOAD 0.0

// These are fields in the information sent for each machine
#define MSGUNITS 7 //message will be sent as a 7byte integer buffer
#define MAXPORTLEN 5
#define IP_FIELD 0
#define PORT_FIELD 1
#define TTL_FIELD 2
#define INDEG_FIELD 3
#define CORES_FIELD 4
#define RAM_FIELD 5
#define LOAD_FIELD 6

//DAEMON CONSTANTS
#define DAEMON_NAME "peermon"
#define PID_FILE "/var/run/peermon/peermon.pid"

#define peermon_exit(msg, val)\
  syslog(LOG_ERR, "PEERMON:%s\n", msg);\
  exit(val);
