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
//HashTable.h

#ifndef _HASHTABLE_H 
#define _HASHTABLE_H
#define UNITS 7
#define PORTLEN 5
#define IPLEN 32
#define DATALEN 64

class HashEntry{
  public:
    unsigned int TOLS;   //the time-stamp of last time we sent HashMap
                              //data to this peer
    char IP[IPLEN]; //just the IP address
    char IPport[IPLEN];//IP and port in the form IP:port
    unsigned int message[UNITS]; //[IP|Port|TTL|Indegree|Cores|RAM|load]
                                 //IP:int representation of the IP
                                 //port:int representation of the port number
                                 //TTL:how old this data is. 0 is old and 
                                 //    MAX_TTL is most recent
                                 //Indegree:how recently have we received data
                                 //    from other peers (a count of number of
                                 //    receives between our sending data)
                                 //Cores: number of cores
                                 //RAM: amount of free ram
                                 //load: CPU load
                                 //    load is x1000, to allow its use an 
                                 //    integer.
                                 //    divide by 1000 on reception

    //methods
    HashEntry(unsigned int indeg, unsigned int ttl,
        unsigned int current_time, const char* ip, unsigned int *data);
    ~HashEntry();
    void setmsg(unsigned int *msg);
    void setIP(const char *ip);
    void print();
};
    
#endif
