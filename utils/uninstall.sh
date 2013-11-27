#! /bin/bash
 #  PeerMon
 #  Copyright 2012 Tia Newhall, Janis Libeks, Ross Greenwood, Jeff Knerr,
 #                 Steve Dini
 #
 #  C++ Sockets on Unix and Windows
 #  Copyright (C) 2002
 #
 #  This file is part of PeerMon.
 #
 #  PeerMon is free software: you can redistribute it and/or modify
 #  it under the terms of the GNU General Public License as published by
 #  the Free Software Foundation, either version 3 of the License, or
 #  (at your option) any later version.
 #
 #  PeerMon is distributed in the hope that it will be useful,
 #  but WITHOUT ANY WARRANTY; without even the implied warranty of
 #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #  GNU General Public License for more details.
 #
 #  You should have received a copy of the GNU General Public License
 #  along with PeerMon.  If not, see <http://www.gnu.org/licenses/>.
 #
 #  PeerMon documentation: www.cs.swarthmore.edu/~newhall/peermon

#change INSTALL_DIR to peermon install directory
INSTALL_DIR="/usr/sbin"

echo "starting peermon unistall"
# 1.stop any running instances of peermon before attempting unistall
/etc/init.d/peermon.sh stop

# 2.start deleting the peermon core
for fname in peermon smarterSSH smarterSSH.py peermonlib.py autoMPIgen autoMPIgen.py
do
  rm -f $INSTALL_DIR/$fname
done

# 3.remove the init.d scripts
rm -f /etc/init.d/peermon.sh
rm -f /etc/rc2.d/s99-peermon


# 5.delete config files
rm -f /etc/peermon/machines.txt

# 6.clean-up old stuff
#rm -f /usr/swat/bin/smartersshTop10*


echo "all components of peermon unistalled"

