#! /bin/sh
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/sbin/peermon
NAME=peermon
SNAME=peermon
DESC="Peer Monitoring Daemon"
PIDFILE="/var/run/$NAME.pid"
OPTIONS=" -p 2288 "

export TMPDIR=/tmp
# Apparently people have trouble if this isn't explicitly set...

if [ ! -d /etc/peermon ] ; then
  echo "no /etc/peermon dir...exiting"
  exit 1
fi

if [ ! -f /etc/peermon/machines.txt ] ; then
  echo "no /etc/peermon/machines.txt file...exiting"
  exit 1
fi

test -f $DAEMON || exit 0

set -e

case "$1" in
  start)
	echo -n "Starting $DESC: "
        $DAEMON $OPTIONS
	echo "$NAME."
	;;

  stop)
	echo -n "Stopping $DESC: "
	pkill -SIGKILL $NAME
	echo "$NAME."
	;;

  restart | force-reload)
        echo -n "Restarting $DESC: "
        pkill -9 $NAME
        $DAEMON $OPTIONS
        echo "$NAME."
        ;;



  *)
	N=/etc/init.d/$SNAME
	echo "Usage: $N {start|stop}" >&2
	exit 1
	;;
esac

exit 0
