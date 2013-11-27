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
//HashEntry.cpp
#include "HashEntry.h"
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;
    
void HashEntry::setmsg(unsigned int *msg){
      for (int i=0;i<UNITS; i++){
        message[i]=msg[i];
      }
}
    
void HashEntry::setIP(const char* ip){
      //helper function for transfering IP into the message structure

      struct sockaddr_in sock; //hold the IP for conversion
      int i=0; //reset counter
      //char IPbuf[IPLEN+1];
      char portbuf[PORTLEN];

      while (ip[i]!=':'){
        IP[i]=ip[i];
        i++;
      }
      IP[i]='\0';
     
      inet_pton(AF_INET, IP, &(sock.sin_addr));
      message[0] = sock.sin_addr.s_addr;

      
      
      int j=0;
      i++; //skip one more character
      while (ip[i]!=0){
        portbuf[j]=ip[i];
        i++;
        j++;
      }
      if (i>31 or j>31){
        portbuf[31]=0;
      }
      else {
      portbuf[j] = '\0';
      }

      message[1] = atoi((const char *)portbuf);


}
void HashEntry::print(){
  printf("IP is %s\n", IP);
  printf("TOLS is %u\n", TOLS);
  printf("IPport is %s\n", IPport);
  for (int i=0;i<8;i++){
    printf("message[%d] is %u\n", i, message[i]);
  }
}


HashEntry::HashEntry(unsigned int indeg, unsigned int ttl,
    unsigned int current_time, const char * ip,unsigned int * data)
{
      strncpy(IPport,ip,31);
      IPport[31]=0;
      TOLS = current_time;
      setmsg(data);
      setIP(ip);
      message[3] = indeg;
      message[2] = ttl;
}

HashEntry::~HashEntry(){
      //printf("@");
}	



