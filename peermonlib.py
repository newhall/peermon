#!/usr/bin/env python
"""
 *  PeerMon Connection Library
 *  Copyright 2011 Steve Dini, Tia Newhall
 * 
 *  This file is part of PeerMon
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
"""
"""======================================================================
                           About peermonlib

peermonlib is the interface which initiates contact with the client thread
running in peermon and collects the hashtable being held by THIS node. It
contains 2 classes, MachineEntry and PeerMonLib. Machine is the class which
defines the data held for each known node in the hashtable while PeerMonLib
creates a list of MachineEntry instances.

Usage
=====
To use this library, instantiate PeerMonLib and then use its nodes_list field
   lib = PeerMonLib()
   entries = lib.nodes_list
   (see smarterSSH.py for an example)
"""


import socket
import sys
from struct import unpack

# this needs to match the value in peermon.h
DEFAULT_CLIENT_PORT = 1981

MSGUNITS = 7 #length of message buffer
INTSIZE = 4 #size of an integer
MSGBYTES = MSGUNITS*INTSIZE #size of message buffer of ints

#The following represent the fields in the message buffer
IP_FIELD = 0 
PORT_FIELD = 1
TTL_FIELD = 2
INDEG_FIELD = 3
CORES_FIELD = 4
RAM_FIELD = 5
LOAD_FIELD = 6

class MachineEntry():
  def __init__(self, data):
    self.PORTNUM = 0
    self.HOSTNAME = ""
    self.IP = ""
    self.cores = 0
    self.ttl=0
    self.indeg = 0
    self.ram = 0
    self.load = 0.0
    self.score = 0.0
    self._populate(data) #actually populate these default fields

  def _populate(self, data):
    """
    This private method, takes apart the 28bytes (7 element integer array)
    and breaks the data down into what it was before being sent across the
    network. It then populates the respective class members with their data
    """
    for i in range(MSGUNITS):
      arg = data[(i*INTSIZE):((i+1)*INTSIZE)]
      if (i==IP_FIELD):
        self._handleIP(arg)
      elif (i==TTL_FIELD):
        self.ttl = self._digitize(arg)
      elif (i==INDEG_FIELD):
        self.indeg = self._digitize(arg)
      elif (i==PORT_FIELD):
        self.PORTNUM = self._digitize(arg)
      elif (i==CORES_FIELD):
        self.cores = self._digitize(arg)
      elif (i==RAM_FIELD):
        self.ram = self._digitize(arg)
      elif (i==LOAD_FIELD):
        self.load = self._digitize(arg)/1000.0 #load is passed around x1000
                                         #so as to pass it as an integer


  def _digitize(self, number):
    """
    Helper function to return the integer equivalent of a number
    sent over the network
    """
    return unpack("i", number)[0]

  def _handleIP(self,bin_IP):
    """
    Helper function to convert an IP address supplied in binary
    format to its equivalent string representation
    """
    self.IP = socket.inet_ntoa(bin_IP)
    try:
      self.HOSTNAME = socket.gethostbyaddr(self.IP)[0] #get hostname
    except:
      self.HOSTNAME = "Unknown Host"

#=======================================================================    
class PeermonLib():
  def __init__(self, port_num=DEFAULT_CLIENT_PORT):
    self.TCP_PORT = port_num
    self.nodes_list = []
    self._get_peermon_data()

  def _get_peermon_data(self):
    """
    This methods creates a TCP socket and creates the pipeline to receive
    data from the client thread of peermon. It receives an initial message
    representing the size of the incoming payload. It then uses this received size
    to actually receive the payload
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if (sock.connect_ex(("localhost", self.TCP_PORT))==-1):
      print "Failed to connect on port: %d"%self.TCP_PORT
      raise SystemExit

    try:    
      size = self._digitize(sock.recv(INTSIZE)) #get expected message size
      payload = sock.recv(size)
    except:
      print "Local PeerMon daemon not running. Program Exiting"
      raise SystemExit

    for i in range(size/MSGBYTES): #how many nodes are we dealing with
      node = MachineEntry(payload[(i*MSGBYTES):((i+1)*MSGBYTES)]) #pass info pertaining to a single node
      if not(node.HOSTNAME=="Unknown Host"): #append only valid hostnames
        self.nodes_list.append(node)
      

  def _digitize(self,number):
    """
    Helper function to return the integer equivalent of a number
    sent over the network
    """
    return unpack("i", number)[0]

#========================================================================
