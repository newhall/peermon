#!/usr/bin/env python
"""
 *  autoMPIgen
 *  Copyright 2010 Tia Newhall, Janis Libeks, Ross Greenwood, Jeff Knerr,
 *                 Steve Dini
 *
 *  C++ Sockets on Unix and Windows  
 *  Copyright (C) 2002
 *
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
""" 

import sys
import random
import os
import socket
from struct import unpack
from peermonlib import PeermonLib #import peermon library
import argparse #to handle argument parsing

# these are some config parameters for smarterSSH that allow it to
# ignore small differences in size when ordering results
# change these to config for your system
MEM_DELTA=100000     # 100 MB differences are not significant for ordering hosts
CPU_DELTA= 100       # 10ths of a unit of CPU load is not significant

class MPIgen():
  def __init__(self, args, data):
    self.results = args.length #number of results to print
    self.verbose = args.verbose #how much info to print
    self.IPs = args.IP #display hostnames or IPs?
    self.rand = args.random #randomize results? 
    self.port_num = args.port #get port_number
    self.data = data #the processes info
    self.in_cpus = args.cpu
    self.output = args.hostfile
    self.inc_cpu_count = args.inc_cpu


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

    random.shuffle(self.data)
    if not self.rand:
      self._sort()

    #print out the results
    if (self.verbose):
      print "IP/Hostname\t\t\tCPU load\tFree Memory\t   CPU Cores"
      print "====================================================================================="
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

  def generate(self):
    """
    Method which actually does the generation of the hostfiles
    """
    random.shuffle(self.data)
    if not self.rand:
      self._sort()

    outfile = self.output
    stdout = sys.stdout
    sys.stdout = outfile #create redirection
    displayed = 0 #number of displayed results
    for host in self.data:
      if not (host.ram==0):
        if (self.inc_cpu_count): #include CPU counts
          if self.IPs:
            print "%s slots=%d"%(host.IP, host.cores)
          else:
            print "%s slots=%d"%(host.HOSTNAME, host.cores)
        else:
          if self.IPs:
            print host.IP
          else:
            print host.HOSTNAME

        if (self.in_cpus):
          displayed += host.cores
        else:
          displayed += 1

      if (displayed >= self.results):
        break

    outfile.close()
    sys.stdout = stdout #restore stdout

  def _CPUsort(self,node):
    #should we divide by number of CPUs?
    if(node.cores >= 1):
      key_val = node.load/node.cores
    else:
      key_val = node.load
    key_val = key_val*CPU_DELTA
    key_val = int(key_val/(1+ (CPU_DELTA/10.0)))
    #print "%30s %-8d " %(node.HOSTNAME, key_val)
    return key_val

  def _MEMsort(self,node):
    #truncate anything beyond MEM_DELTA precision
    val = node.ram + (MEM_DELTA*5)/10   # round up
    key_val =  int(val/MEM_DELTA)
    if(key_val == 0):   # we just need a non-zero value here to negate later
      key_val = 0.01
    key_val = key_val*MEM_DELTA

    #print "%30s %-8d" %(node.HOSTNAME, key_val)
    return -key_val #neg sign to rearrange the order

  def _CMsort(self,node):
    #use (FreeMem/(1+CPUload)) as the comparator relationship
    e1 = self._MEMsort(node)
    e2 = self._CPUsort(node)
    key_val = int((e1)/(1+e2))  # add 1 to avoid division by 0
    #print "%30s %-8d" %(node.HOSTNAME, key_val)
    return key_val


#======================================================================
def main():
  descriptiontext = "autoMPIgen. Version 2.0"
  argp = argparse.ArgumentParser(description=descriptiontext)
  argp.add_argument('hostfile', type=argparse.FileType('w'),
                     help="Name of the output file for autoMPIgen")
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
                    help="return any random n nodes: no ordering by usage")
  argp.add_argument('-v', '--verbose', action='store_true', default=False,
                    dest='verbose', help="Fully print all the statistics")
  argp.add_argument('-I', '--IP', action='store_true', default=False, dest='IP',
                    help="Print machines by IP instead of by hostname")
  argp.add_argument('-i', '--info', action='append_const', dest='flags',
                    const='i', help="Display results and exit")
  argp.add_argument('-q', '--quantify', action='store_true', default=False,
                    dest='inc_cpu', help="Include number of CPUs in hostfile")
  argp.add_argument('-x', action='store_true', default=False, dest='cpu',
                    help='Execute the -n flag as number of CPUs not nodes\
                         (only valid when both -q and -n are used)')


  args = argp.parse_args()
  library = PeermonLib() #create instance of PeerMon data parser
  session = MPIgen(args, library.nodes_list)

  if args.flags==None or not('i' in args.flags):
    session.generate()
  elif (args.verbose and not('i' in args.flags)):
    session.printout()
    session.generate()
  else:
    session.printout()

if __name__=="__main__":
  main()


