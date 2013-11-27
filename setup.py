#! /usr/bin/python
"""
/*
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
 */
"""

import os

def install():
  """
  Function which determines if the user installing the software is Ok with
  the default installation directory or whether they want to suggest an
  alternative one for that purpose.
  If defaults are accepted, it proceeds and does the install, otherwise it
  invokes the make command passing in the suggested location as an argument
  """
  default = "/usr/sbin"

  print "Installing PeerMon 1.1"
  print "=========================================================="

  if ask_yn('Installing in ' + default + '. Ok with this destination?'):
    os.system('make install')
  else:
    install_dir = raw_input('Enter alternative installation directory: ')
    os.system('make INSTALL_DIR=' + install_dir + ' install')

def ask_yn(text):
  """
  Convenience function to ask a yes/no question and return True if the
  answer is 'y', 'yes', or variants, and False otherwise.
  """
  response = raw_input(text + ' [y/N] ')
  if response.lower() in ('y', 'ye', 'yes', 'yep'):
      return True
  return False

def main():
  install()

if __name__=="__main__":
  main()





