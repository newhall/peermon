peermon
=======

A peer-to-peer LAN monitoring system

Peermon is a peer-to-peer network monitoring system for Linux LANs and 
clusters.  It is designed to run continuously on nodes and to scale to
large sized systems.  Peermon provides load and other information about
all nodes in the LAN.  Through its client interface, applications obtain
Peermon data that they can use it make LAN-wide decisions based on load.

Peermon includes two client applications: smarterSSH and autoMPIgen.
These applications select "good nodes" for ssh targets and fro mpi host
files based on Peermon data about CPU and memory load and number of cores
on each machine in the NW.   A simple client program is also included as
an example of how to write new client programs that use Peermon data.

We also describe how set-up dynamic dns mapping to use peermon data
to distribute ssh traffic over nodes based on Peermon's cluster-wide
load information.

More details about Peermon, about building and installing it, and about how
to use the applications included with it are available off the peermon
webpage:

http://www.cs.swarthmore.edu/~newhall/peermon
