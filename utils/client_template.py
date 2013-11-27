#!/usr/bin/env python
"""
 *  PeerMon
 *  Copyright 2012 Tia Newhall, Janis Libeks, Ross Greenwood, Jeff Knerr,
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

This is the base code on which to expand any client 

To use, copy this to a new file and:
 * Replace CLASSNAME with the name of your class
 * Replace FUNCTIONNAME to your method function name and add 
   in the desired functionality
 * and replace FUNCTION to the name of your new method function
 * and re-write main to add correct control flow for your client program 

This template has an example of support for command line arguments
similar to those used by 
""" 

from sys import argv,exit
import random
import os
import socket
from struct import unpack
from peermonlib import PeermonLib #import parser for PeerMon raw output
import argparse #to handle argument parsing

DELTA=1000 
class CLASSNAME():
  def __init__(self, args, data):

    self.results = args.length #number of results to print
    self.verbose = args.verbose #how much info to print
    self.IPs = args.IP #display hostnames or IPs?
    self.rand = args.random #randomize results? 
    self.port_num = args.port #get port_number
    self.data = data #the processes info
    self.flags = args.flags
  
  def _sort(self):
    """
    Function to sort the contents of data depending on the passed
    argument
    """
    try:
      if (('c' in self.flags) and ('m' in self.flags)):
        self.data.sort(key=self._CMsort) #both CPU and MEM as sorting keys
      elif ('c' in self.flags): #sort using only CPU
        self.data.sort(key=self._CPUsort)
      elif ('m' in self.flags):#sort using only mem
        self.data.sort(key=self._MEMsort)
    except: #no c or m arguments passed
      self.data.sort(key=self._CMsort)

  def printout(self):
    if len(self.data) < self.results:
      self.results = len(self.data)
    
    random.shuffle(self.data) #randomize results
    if not self.rand:
      self._sort()

    #print out the results
    if (self.verbose):
      print "IP/Hostname\t\t\tCPU load\tFree Memory\t   CPU Cores"
      print "==========================================================================="
      for i in range(self.results):
        if self.IPs:
          print "%-30s\t%.3f\t\t%10d\t\t%d"%(
              self.data[i].IP,
              self.data[i].load,
              self.data[i].ram,
              self.data[i].cores)

        else:
          print "%-30s\t%.3f\t\t%10d\t\t%d"%(
              self.data[i].HOSTNAME,
              self.data[i].load,
              self.data[i].ram,
              self.data[i].cores)

    else: #print condensed information
      for i in range(self.results):
        if self.IPs:
          print self.data[i].IP

        else:
          print self.data[i].HOSTNAME

  def CLASSFUNCTION(self):
    """
    Perform the functionality associated with this peermon tool
    """
    #INSERT DESIRED FUNCTIONALITY HERE

  def _CPUsort(self,node):
    #should we divide by number of CPUs?
    val = int(DELTA*float(node.load))
    return val/1000.0

  def _MEMsort(self,node):
    #truncate anything beyond 1000's k
    return -DELTA*(node.ram/DELTA) #neg sign to rearrange the order

  def _CMsort(self,node):
    #use (FreeMem/(1+CPUload)) as the comparator relationship
    e1 = DELTA*(node.ram/DELTA)
    e2 = int(DELTA*float(node.load))/1000.0
    return -(e1/(1+e2))   #add 1 in denominator to avoid
                          #division by zero

#======================================================================
def main():
  descriptiontext = "Client_Template. Version 2.0"
  argp = argparse.ArgumentParser(description=descriptiontext)
  argp.add_argument('-p', '--port', action='store', type=int, 
                    dest='port', default=1981, 
                    help="Specify the port number to use with smarterSSH.")
  argp.add_argument('-c', '--cpu', action='append_const', dest='flags',
                    const='c', help="Sort results by CPU loads")
  argp.add_argument('-m', '--mem', action='append_const', dest='flags',
                    const='m', help="Sort results by available memory")
  argp.add_argument('-n', action='store', type=int, dest='length',
                    default=1, help="specify number of machines to print")
  argp.add_argument('-r', action='store_true', default=False, dest='random',
                    help="Randomly sort the results")
  argp.add_argument('-v', '--verbose', action='store_true', default=False,
                    dest='verbose', help="Fully print all the statistics")
  argp.add_argument('-I', '--IP', action='store_true', default=False, dest='IP',
                    help="Print machines by IP instead of by hostname")
  argp.add_argument('-i', '--info', action='append_const', dest='flags',
                    const='i', help="Display results and exit")

  args = argp.parse_args()
  library = PeermonLib() #create instance of PeerMon data parser
  session = CLASSNAME(args, library.nodes_list)
  
  if args.flags==None or not('i' in args.flags):
    session.FUNCTION()
  elif (args.verbose and not('i' in args.flags)):
    session.printout()
    session.FUNCTION()
  else:
    session.printout()

if __name__=="__main__":
  main()



