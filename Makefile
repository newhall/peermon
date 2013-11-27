
#   PeerMon
#   Copyright 2011 Tia Newhall, Janis Libeks, Ross Greenwood,
#   		   Jeff Knerr, Steve Dini
# 
#   C++ Sockets on Unix and Windows  
#   Copyright (C) 2002
# 
#   This file is part of PeerMon.
# 
#   PeerMon is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by #   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
# 
#   PeerMon is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with PeerMon.  If not, see <http://www.gnu.org/licenses/>.
#  
#   PeerMon documentation: www.cs.swarthmore.edu/~newhall/peermon

CC = g++
CFLAGS = -g -Wall -ansi -pedantic 


LIBDIR = -L./
INCDIR = -I./

LIBS = $(LIBDIRS) -lm -pthread

TARGET = peermon

# set INSTALL_DIR to the directory into which you want to install peermon
# on your system.  for example: INSTALL_DIR = /usr/bin 
INSTALL_DIR = /usr/sbin

# set config directory to where you want the config files installed
PEERMON_CONFIG_DIR = /etc/peermon

# set etc init.d path
PEERMON_INITD_DIR = /etc/init.d

CP = cp

.PHONY: clean install

all: $(TARGET) 

$(TARGET): peermon.cpp PracticalSocket.o HashEntry.o  peermon.h 
	$(CC) $(CFLAGS) -o $(TARGET) peermon.cpp PracticalSocket.o HashEntry.o $(LIBS)

PracticalSocket.o: PracticalSocket.cpp PracticalSocket.h
	$(CC) $(CFLAGS) -c PracticalSocket.cpp $(LIBS)

HashEntry.o: HashEntry.cpp HashEntry.h
	$(CC) $(CFLAGS) -c HashEntry.cpp $(LIBS)

install: $(TARGET)
	mkdir -p $(INSTALL_DIR)
	mkdir -p $(PEERMON_CONFIG_DIR)
	mkdir -p $(PEERMON_INITD_DIR)
	echo "copying peermon, autoMPIgen.py, smarterSSH.py to" $(INSTALL_DIR)
	$(CP) peermon $(INSTALL_DIR)/.
	$(CP) smarterSSH.py $(INSTALL_DIR)/smarterSSH
	$(CP) autoMPIgen.py $(INSTALL_DIR)/autoMPIgen
	$(CP) peermonlib.py $(INSTALL_DIR)/.
	$(CP) utils/peerhealth.sh $(INSTALL_DIR)/peerhealth.sh
	$(CP) utils/peermon.sh $(PEERMON_INITD_DIR)/peermon.sh
	ln -f -s $(PEERMON_INITD_DIR)/peermon.sh /etc/rc2.d/S99-peermon
	$(CP) utils/machines.txt $(PEERMON_CONFIG_DIR)/machines.txt
	$(CP) utils/valid_ips.txt $(PEERMON_CONFIG_DIR)/valid_ips.txt
	chmod 755 $(INSTALL_DIR)/smarterSSH
	chmod 755 $(INSTALL_DIR)/autoMPIgen
	chmod 755 $(INSTALL_DIR)/peermonlib.py
	chmod 755 $(PEERMON_CONFIG_DIR)
	chmod 644 $(PEERMON_CONFIG_DIR)/machines.txt
	chmod 644 $(PEERMON_CONFIG_DIR)/valid_ips.txt

clean:
	$(RM) *.o core $(TARGET) 

