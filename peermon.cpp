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

#include "PracticalSocket.h" // For UDPSocket and SocketException
#include <iostream>          // For cout and cerr
#include <cstdlib>           // For atoi()
#include <map>
#include <vector>
#include <algorithm>
#include <utility> // make_pair
#include <fstream>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <typeinfo>
#include "peermon.h"
#include "HashEntry.h"
#include <signal.h>
#include <sys/wait.h>
#include <vector>
#include <pwd.h>

using namespace std;

typedef HashEntry *HashEntryPtr;
typedef map<string,HashEntryPtr> HashMap;
typedef pair<int,string> SortPair;

struct thread_args{
  HashMap *hashtable;
};

static pthread_mutex_t hashtable_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t indegree_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool keep_alive;
static char self_IP[IP_LEN];
static int  port_num;
static int  client_port_num = DEFAULT_CLIENT_PORT;
static int send_port;  
static unsigned int self_indegree;
static int use_heuristic;
static int collector_only=0;
static char * config_file=0; 
static int sleep_secs=DEFAULT_SLEEP_AMT;
int IP_PREFIX_GIVEN=0;
int THRESHOLD = 32000;
static unsigned int current_time=0;
static char *ip_file=0;
vector<string>known_IPs; //vector for holding valid IP prefixes

uid_t PEERUID;
gid_t PEERGID;
unsigned int get_own_memory();
void parse_IP(const char * input_IP, char ret_IP[]);
string getStdoutFromCommand(string);
static void *handle_client_req(void *args);
void IPport_to_bin(const char *IPport, unsigned int *message);
bool IP_isValid(char *ip); //compare IP with known prefices
// set to 0 to turn off Hash table debugging
static const int PEERMON_DEBUG=0;
// set to 0 to turn off debug output
static const int DEBUG_PEERMON=0;
static const float VERSION=1.1;
static int user_mode = 0;

//*********************************************************************
// reads in the list of initial peermon machines from config file
// these are the first machines inserted into the hashtable
// and they act as the initial connect anchor to the network.
// at least one of the machines on this list should be running a daemon
// for the network to be functional
void readin(HashMap* hashtable){

  string line;
  if(config_file == 0) {
    config_file = const_cast<char *>("/etc/peermon/machines.txt");  
  } 
  if (DEBUG_PEERMON) {cout << "config file: " << config_file << "\n";}
  ifstream myfile(config_file);
  if (myfile.is_open())
  {
    while (! myfile.eof() )
    {
      getline (myfile,line);
      if(line[0]!=0){
        unsigned int temp[MSGUNITS];
        for (int j=0; j< MSGUNITS;j++){
           temp[j]=0;
        }
        IPport_to_bin(line.c_str(), temp);  //convert IP to binary
        HashEntry * dummyhash = 
          new HashEntry(0,MAX_TTL,current_time,line.c_str(),temp); 
        hashtable->insert(pair<string,HashEntryPtr>(string(line),dummyhash));        
      }
    }
    myfile.close();
  }

  else {
    cerr << "Unable to open file: " << config_file << "\n"; 
    perror("Unable to open file"); 
    peermon_exit("/etc/peermon/machines.txt not found, OR no \
        config file specified",1);
  }
}

//*********************************************************************
//Helper function to evaluate if a given HashEntry exists in a vector
//of HashEntries
bool isSubset(HashEntry* a, vector<HashEntry*> *vec){
  for (vector<HashEntry*>::iterator t = vec->begin();
                                    t!= vec->end();
                                    ++t){
    if (a->IPport == (*t)->IPport){
      return true;
    }
   }
  return false;
}
//*********************************************************************
// Heuristic 1: MAX_TTL-TTL+indeg
// "Contact New Nodes"
// The heuristic picks peers with the smallest value of MAX_TTL-TTL+indeg
// The heuristic ensures that nodes with a high TTL (nodes with new information)
//    and a low indegree (nodes that have not received recently) are selected.
//    The heuristic ensures that new peers are quickly integrated into the
//    system.
void heuristic_ContactNewNodes(HashEntry* a, vector<HashEntry*> *vec){
  unsigned int value;
  unsigned int current=0;
  
  //find entry to take out from the supplied vector
  for (unsigned int i=0; i < vec->size();i++){
    if ((*vec)[i]->message[TTL_FIELD] == 0){
      value = THRESHOLD;
      //(*vec)[i]->message[TTL_FIELD] = THRESHOLD; 
    }
    else {
      value = 
        MAX_TTL-(*vec)[i]->message[TTL_FIELD]+(*vec)[i]->message[INDEG_FIELD];
    }
    if (value > (unsigned int)(MAX_TTL-(*vec)[current]->message[TTL_FIELD]+
        (*vec)[current]->message[INDEG_FIELD])){
      current=i;
    }
  }

  //does the supplied Hash Table Entry have a heuristic value less than the 
  //largest we know of?
  if ((a->message[INDEG_FIELD]-a->message[TTL_FIELD]) >
     ((*vec)[current]->message[INDEG_FIELD]-(*vec)[current]->message[TTL_FIELD])
      && !isSubset(a, vec))
  {
    (*vec)[current]=a;
  }
}

//*********************************************************************
// Heuristic 2: TTL
// "Contact Forgotten Nodes"
// Selects the three peers with the lowest TTL (nodes the present node has not
//     heard from recently)
void heuristic_ContactForgottenNodes(HashEntry* a, vector<HashEntry*> *vec){
  unsigned int value;
  unsigned int current=0;

  //traverse the given vector
  for (unsigned int i=0;i<vec->size();i++){
    value = (*vec)[i]->message[TTL_FIELD];
    if (value > (unsigned int)((*vec)[current]->message[TTL_FIELD])){
      current=i;
    }
  }

  //Should we pick the supplied HashEntry
  if ((a->message[TTL_FIELD] > 
        (*vec)[current]->message[TTL_FIELD]) && !isSubset(a, vec))
  {
    (*vec)[current]=a;
  }
}

//*********************************************************************
// Heuristic 3: TOLS
// "Contact Old Friends"
// Ensures that a node cannot be permanently isolated. Selects Nodes with the 
//      highest TOLS to choose peers we have not sent data to recently.      
void heuristic_ContactOldNodes(HashEntry* a, vector<HashEntry*> *vec){
  unsigned  int value;
  unsigned int current=0;

  //traverse the given vector
  for (unsigned int i=0; i < vec->size(); i++){
    value = (*vec)[i]->TOLS;
    if (value < ((*vec)[current]->TOLS)){
      current=i;
    }
  }
  
  if ((a->TOLS < (*vec)[current]->TOLS) && !isSubset(a, vec)){
    (*vec)[current]=a;
  }
}

//*********************************************************************
//Update Peers
//Function that takes in a vector whose entries relate to the current "best"
//      nodes to send data to. It also takes in a reference to a HashEntry
//      and evaluates according to the current heuristic to see if the new
//      entry is "better" than one of the entries in the table. If "better",
//      the current "worst" in the vector of "best" nodes us replaced with
//      the new passed in entry
//The functionality for the actual comparisons is delegated to the 
//      heuristic_* functions.
//      a:
//      vec: the set of current peers to send to
void updatePeers(HashEntry* a, vector<HashEntry*> *vec){
  switch(use_heuristic){
    case 1:
      // use heuristic 1
      heuristic_ContactNewNodes(a, vec);
      break;
    case 2:
      // use all three, in order
      switch(current_time%3){
        case 0:
          heuristic_ContactNewNodes(a, vec);
          break;
        case 1:
          heuristic_ContactForgottenNodes(a, vec);
          break;
        case 2:
          heuristic_ContactOldNodes(a, vec);
          break;

      }
      break;
    case 3:
      heuristic_ContactForgottenNodes(a, vec);
      break;
  }
}

//*******************************************************************
//readPrefix()
//Opens the /etc/peermon/valid_ips.txt file and reads in the set of
//      IP prefices in the file which will later on be compared with
//      encountered IPs to check their validity. The known prefices
//      are stored in the global <known_IPs> vector.
//If the file is non-existant or is empty, IP_PREFIX_GIVEN is set to
//      zero indicating that we must accept any IP we encounter later
//      on.
void readPrefix(){
  string line;
  if(ip_file == 0) {
    ip_file = const_cast<char *>("/etc/peermon/valid_ips.txt");  
  } 
  
  ifstream myfile(ip_file);
  if (myfile.is_open()){
    IP_PREFIX_GIVEN=1;
    int ipcount=0; //how many IPs have we read 
    while (! myfile.eof() ){
      //populate buffer
      getline(myfile, line);
      if (line[0]!=0){
      known_IPs.push_back(line);
      ipcount++;
      }
    }
    if (ipcount==0){ //no prefix found in the ip_file
      IP_PREFIX_GIVEN=0;
    }
    myfile.close();
  }
  else {
    IP_PREFIX_GIVEN=0;
  }
}
//*******************************************************************
//Function that checks to see if a supplied IP is valid according to
//a set of prefices in a configuration file
bool IP_isValid(char *ip){
  char prefix[IP_LEN];
  int periods = 0; //counter for number of period in IP address
  if (IP_PREFIX_GIVEN){
    for (int i=0; i<IP_LEN;i++){
      if (ip[i]=='.'){periods++;}
      if (periods==3){
        prefix[i]='\0';
        break; //we have pulled the prefix from the IP
      } 
      prefix[i]=ip[i];
    }
    
    //now check prefix against entries in the prefices vector
    for (vector<string>::iterator t = known_IPs.begin();
                                  t!= known_IPs.end();
                                  ++t){
      if (prefix==*t){
        return true; //IP is valid
      }
    }
    return false; //IP is invalid
  }
  return true; //No ip file given or found, so all IPs are 'valid'
}

//********************************************************************
//Helper function to help convert IPport into its binary representation
 
void IPport_to_bin(const char *IPport, unsigned int *message){
      struct sockaddr_in sock; //hold the IP for conversion
      int i=0; //reset counter
      char IPbuf[IP_LEN+1];
      char portbuf[MAXPORTLEN];


      while (IPport[i]!=':' && (i <= IP_LEN)){
        IPbuf[i]=IPport[i];
        i++;
      }

      if(i > IP_LEN) {  // bad
        syslog(LOG_WARNING, "Failed to convert IP address %s bad IP:port format\n",IPbuf);
        message[IP_FIELD]=0;
        
      } else {
        IPbuf[i]='\0';
        if (!inet_pton(AF_INET, IPbuf, &(sock.sin_addr))){
          syslog(LOG_WARNING, "Failed to convert IP address %s \n",IPbuf);
          message[IP_FIELD]=0;
        }
        else{
          message[IP_FIELD] = sock.sin_addr.s_addr;
        }
        int j=0;
        i++; //skip one more character
        while (IPport[i]!=0){
          portbuf[j]=IPport[i];
          i++;
          j++;
        }
        portbuf[j] = '\0';
        message[PORT_FIELD] = atoi((const char *)portbuf);
      }
      if(DEBUG_PEERMON) {
         printf("adding peer %d:%d\n",message[IP_FIELD], message[PORT_FIELD]);
      } 
}


//********************************************************************
// collect data about this own node
// if THIS node is in collector mode (its a passive observer), the node is 
//      given 0 cores and a value of 1 for available RAM. These two values are
//      later used for discrimination between passive and active nodes.
// If not in collector-mode, values for load and available RAM and cores
//      are obtained from /proc
void get_own_data(unsigned int *message){
  string line;
  float load;

  IPport_to_bin(self_IP, message); //populate the IP and Port fields 

  if(collector_only) { 
    //default collector only configuration
    message[CORES_FIELD] = COLLECTOR_CORES;
    message[RAM_FIELD] = COLLECTOR_RAM;
    message[LOAD_FIELD] = int(1000*COLLECTOR_LOAD); //convert to a x1000 integer
    return;
  }

  ifstream myfile ("/proc/loadavg");
  if (myfile.is_open()) {
    getline(myfile,line);
    sscanf(line.c_str(),"%f",&load);
  }
  myfile.close();
  unsigned int mem = get_own_memory();
  string cores = 
    getStdoutFromCommand(string("cat /proc/cpuinfo | grep processor | wc -l")); 
  message[CORES_FIELD] = atoi((const char *)cores.c_str());
  message[RAM_FIELD] = (mem);
  message[LOAD_FIELD] = (1000*load); //convert to a x1000 integer
}

//*********************************************************************
// zero all the TOLS values for each of the HashTable Entries. Function only
// called when there is a danger of wrapping the value of current_time
void zero_TOLS(HashMap* hashtable){
  for(HashMap::const_iterator iter = hashtable->begin(); 
      iter != hashtable->end(); ++iter)
  {
      iter->second->TOLS=0; 
  }
}

//*********************************************************************
// update hashtable with memory and CPU data from this machine, and
// decrease TTL of all entries inside the hashtable, removing any if 
// necessary.
void update_hashtable(HashMap* hashtable){
  char data[DATA_LEN];
  unsigned int message[MSGUNITS];
 
  memset(data, 0, sizeof(data)); //initilaize buffer prior to use
  get_own_data(message); 
  if(DEBUG_PEERMON) { 
    printf(">>self stats: %s\n",data); 
    printf("%u %u %u %u %u %u %u  \n", message[0], message[1], message[2],
        message[3], message[4], message[5], message[6]);
  }
  // go through hashtable, decreasing TTL
  for(HashMap::const_iterator iter = hashtable->begin(); 
      iter != hashtable->end(); ++iter)
  {
    if (iter->second->message[TTL_FIELD] > 0) {
      iter->second->message[TTL_FIELD]--;
    }
  }

  //update self entry inside hashtable
  HashMap::iterator iter = hashtable->find(self_IP);
  if(iter != hashtable->end()){
    iter->second->setmsg(message);
    iter->second->message[TTL_FIELD] = MAX_TTL;
    iter->second->message[INDEG_FIELD] = self_indegree;
  }	
  else{
    // if own entry does not exist, create it
    HashEntry * dummyhash = new HashEntry(self_indegree, MAX_TTL,
                                 current_time, self_IP,message);
    if (DEBUG_PEERMON){
      dummyhash->print();
    }
    hashtable->insert(pair<string,HashEntryPtr>(string(self_IP),dummyhash));
  }
  pthread_mutex_lock(&indegree_mutex);
  self_indegree = 0;
  pthread_mutex_unlock(&indegree_mutex);
}

//*********************************************************************
// Run the command represented by cmd and capture the contents of
// stdout after executing the command.
string getStdoutFromCommand(string cmd){

  // setup
  string data;
  FILE *stream;
  char buffer[255];

  // do it
  stream = popen(cmd.c_str(), "r");
  while ( fgets(buffer, 255, stream) != NULL )
    data.append(buffer);
  pclose(stream);

  // exit
  return data.substr(0,data.length()-1);
}
//*********************************************************************
// allocates a new message buffer and/or a new array of message buffers
// depending on the contents of the array of buffers, and the value of 
// max_entries, next_entry
//
//   messages: the array of buffers
//   next_entry: the index into which the caller wants a buffer
//   max_entries: the current max size of the array of buffers
//                passed by reference
//   returns a pointer to the array of message buffers
//         the max_entries value may be updated to the new max size if a
//         new array of buffers neede to be allocated
//
unsigned int **alloc_new_message_buffer(unsigned int **messages, 
    unsigned int *msgsizes, 
    int next_entry, int *max_entries) 
{

          // do we need more space in the messages array? (and the sizes array too)
          if(next_entry == *max_entries) { 
            messages = (unsigned int **)realloc(messages, 
                        sizeof(unsigned int *)*((*max_entries)+20)); 
            msgsizes = (unsigned int *)realloc(msgsizes, 
                       sizeof(unsigned int)*((*max_entries)+20));
            if(!messages) {
              perror("realloc messages array");
              peermon_exit("Error reallocating messages array",1);
            }
            for(int i=*max_entries; i < *max_entries+20; i++) { 
              messages[i] = 0; 
              msgsizes[i] = 0;
            }
            *max_entries += 20;
          }
          // do we need to alloc space for a new message buffer?
          if(! messages[next_entry]) {
            messages[next_entry] = 
              (unsigned int *)calloc((MAX_UDP_MSG_LEN),sizeof(unsigned int));
            if (!messages[next_entry]) {
              perror("calloc message entry");
              peermon_exit("Error calloc'ing message array", 1);
            } 

          }
          return messages;
}
//*********************************************************************
// sender thread:
// responsible for sending out hashMap data to three peers
// The sender thread periodically wakes up after sleep_secs, increments the 
//      current time and packages all the entries in the HashMap with TTL>0
//      (those that are still alive) into UDP message-sized packets. And at the
//      same time, it updates the <peers> vector with the set of current "best"
//      peers to send data to.
// After the packaging and selection, the sender thread then sends out the data
//      to the nodes in the peers vector before going back to sleep again.
//                 
void *sender(void *args){

  unsigned int **messages;
  unsigned int *msgsizes;
  thread_args * received_args = (thread_args*)(args);
  HashMap * hashtable = received_args->hashtable;

  UDPSocket sock(send_port);                

  int fd=0;

  // alloc an array of message buffers, each udp send will send a single
  // buffer from this array that contains some number of hashMap entries
  // the buffer size is selected so that it fits into a single UDP packet
  // MAX_UDP_MSG_LEN should be <= single packet payload size
  // we start out with space for 100 buffers, only allocating each individual
  // buffer as we need it, and will allocate more total buffers if we need 
  messages = (unsigned int **)malloc(sizeof(unsigned int *)*100); 
  msgsizes = (unsigned int *)malloc(sizeof(unsigned int)*100); 
  //create a parallel array to hold the sizes of each buffer
  
  if(!messages || !msgsizes) {
    perror("malloc messages array");
    peermon_exit("malloc error in initial malloc of messages array", 1);
  }
  vector<HashEntry*> peers;
  int max_entries = 100;
  int next_entry = 0;
  
  for(int i=0; i < max_entries; i++) { 
    messages[i] = 0;
    msgsizes[i] = 0;
  }


  while(keep_alive) {
    // keep sending until the apocalypse/crash/restart, whichever comes first
    if (current_time > current_time + 1){
      //if current_time is larger than the largest unsigned long
      //iterate hash entries and zero all TOLS fields
      zero_TOLS(hashtable);
      current_time = 0;
    }
    current_time++;
    update_hashtable(hashtable);
    if(DEBUG_PEERMON) {printf("Hashtble size: %d\n",(int)(hashtable->size()));}

    // start display message
    if(DEBUG_PEERMON) {printf("Message:\n---\n");}

    // vector<char*> messages; // vector that keeps all messages to be sent
    HashMap::iterator iter = hashtable->begin();
    unsigned int* message = 0;
    next_entry = 0;
    unsigned int cur_entry=0;
    int mes_byte_counter=0;
    if(!messages[0]) {  // make sure to allocate the first buffer 
      messages = 
        alloc_new_message_buffer(messages, msgsizes, next_entry, &max_entries);
    }
    next_entry++;
    message=messages[0];  // get the start buffer
    for(int i=0; i < max_entries; i++) { //zero the sizes buffer
      msgsizes[i] = 0;
    }
    
    for(; iter != hashtable->end(); ++iter) {
      if (iter->second->message[TTL_FIELD]>0 
          && (0<=iter->second->message[CORES_FIELD])) 
      { 
        mes_byte_counter += MSGUNITS;
        if((msgsizes[cur_entry]+ (MSGUNITS*sizeof(unsigned int))) >=
            (unsigned int)MAX_UDP_MSG_LEN) 
        { // we need a new buffer
           messages =  alloc_new_message_buffer(messages, msgsizes, 
               next_entry, &max_entries); //this is more of a resize
           message = messages[next_entry];
           mes_byte_counter = MSGUNITS;
           next_entry++;
           cur_entry++;
        }
        memcpy((void *)message, (void *)iter->second->message, 
            sizeof(unsigned int)*MSGUNITS);
        //increment size of current buffer
        msgsizes[cur_entry] = 
          msgsizes[cur_entry]+(MSGUNITS*sizeof(unsigned int)); 
        if (PEERMON_DEBUG){
          printf("msgsizes[%d] is %d\n", cur_entry, msgsizes[cur_entry]);
        }
        message+=MSGUNITS;
      } 

      
      if( strcmp(iter->first.c_str(),self_IP)!=0){
        //push first 3 peers into the vector before starting any comparison
        if ((int)peers.size()<SEND_TO){ 
          peers.push_back(iter->second);
        }
        // compute heuristic for who to pick next
        else {
          updatePeers(iter->second, &peers);
        }
      }
     
    }

    // send to the first SEND_TO in the list
    for (int i=0; i<(int)peers.size(); i++) {
      string IPport = string(peers.at(i)->IPport);
      HashMap::iterator cc = hashtable->find(IPport);
      HashEntry* current = cc->second;
      unsigned int portNum = current->message[PORT_FIELD];
      char *IP = current->IP;
      current->TOLS = current_time;
      for (int j = 0; j < next_entry; j++) {
          unsigned int *m = messages[j];
          //number of bytes used in the current buffer
          int bytes = msgsizes[j]; 
          if (PEERMON_DEBUG){
            printf("Trying to send %d bytes \n", bytes);
            printf("IP_str value is: %s\n", IP);
          }

          try { 
            sock.sendTo(m,
                bytes,
                string(IP),
                portNum );
          } catch (SocketException E) {
            // we just don't send to this one      
            if(DEBUG_PEERMON){ 
              printf("SocketException, IP %s\n", IP);
            }
          }
      }
    }
    sleep(sleep_secs);
  }
  close(fd);
  syslog(LOG_INFO,"Sender thread dying");
  
  //clean up!
  for (int i=0; i < next_entry; i++){
    free(messages[i]);
  }
  free(messages);
  free(msgsizes);
  return NULL;
}

//*********************************************************************
// input_IP: string with first entry is IP:w
// ret_IP: string to fill with just the IP part from input_IP
void parse_IP(const char * input_IP, char ret_IP[]){
  if(DEBUG_PEERMON){ printf("input_IP:%s|\n",input_IP);}
  int IP1,IP2,IP3,IP4;
  sscanf(input_IP,"%d.%d.%d.%d",&IP1,&IP2,&IP3,&IP4);
  sprintf(ret_IP,"%d.%d.%d.%d",IP1,IP2,IP3,IP4);
}

//*********************************************************************
//Get available memory by running the "free" command and piping it into a grep
unsigned int get_own_memory(){
  string text = getStdoutFromCommand(
      string("free | grep 'Mem:' | cut -d: -f2 | awk '{ print $3}'"));
  unsigned int mem = atoi(text.c_str());
  return mem;

}

//*********************************************************************
// Right now this is IPv4 specific
void get_own_IP(char buf[]){
  // retrieve the IP of the first ethernet port for this machine

  // hacky way parsing ifconfig output 
  // (this is not portable to different linux distributions/versions):
  //string text = getStdoutFromCommand( string("ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'"));
  //parse_IP(text.c_str(), buf);
  
  // better way (thanks to Jeff who found this): 
  // https://www.includehelp.com/c-programs/get-ip-address-in-linux.aspx)
  // (1) create a socket
  // (2) use ioctl on socket to NW config information about eth0
  // (3) close socket
  // (4) convert sin_addr filed to IP string (using inet_ntoa)
  int fd;
  struct ifreq ifr;  /* sin_addr field stores IP address */

  /* create a udp socket (AF_INET is IPv4 */
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  ifr.ifr_addr.sa_family = AF_INET;

  /*eth0 - define the ifr_name - port name where network attached.*/
  memcpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

  /* get network interface information about eth0.*/
  ioctl(fd, SIOCGIFADDR, &ifr);
  /*closing fd*/
  close(fd); 
  
  /*Extract IP Address*/
  strcpy(buf, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

  if(DEBUG_PEERMON){ printf("self IP:%s\n",buf); }
}


//*********************************************************************
// client interface thread main loop
// Waits until it gets a request for data from a client and only then 
// does it call the handle_client_req function which is resposible
// for sending the data out to the client
void *client_interface(void *args){

  thread_args * received_args = (thread_args*)(args);
  HashMap * hashtable = received_args->hashtable;
  TCPServerSocket sock(client_port_num);                
  TCPSocket *s;
  char  *c_args[3];
  
  if(DEBUG_PEERMON){ printf("client thread started\n"); }


  while(keep_alive) {

    // wait for a connection
      s = sock.accept();
      if(DEBUG_PEERMON){ 
        printf("got a client connection: %s:%d \n",
          s->getLocalAddress().c_str(), s->getLocalPort());
      }
      c_args[0] = (char *) s; 
      c_args[1] = (char *)hashtable; 
      c_args[2] = 0;
      handle_client_req(c_args);
  }

  return 0;
}

//*********************************************************************
// handle a client connection...
// using the TCP socket opened in client_interface function, this function
//      sends out data to the requesting client.
// all the entries in the HashTable are packed into a TCP message which is
//      sent to the client. The only exception are entries which are in
//      collector mode. Entries referencing those nodes are not sent to the
//      client.
// The size of the outgoing TCP packet is first sent out before the packet
//      itself is sent out, so as to co-ordinate communication between 
//      peermon and the client.
// After sending out these two pieces of data, the socket is then closed
static void *handle_client_req(void *args){

  char **my_args = (char **)args;

  TCPSocket *client_sock = (TCPSocket *)my_args[0]; 
  HashMap * hashtable = (HashMap *)my_args[1];
  unsigned int *message;
  unsigned int *tracker;
  int next = 0;
  int count = 0;
  int all = 0;
  int some = 0;
  int size = MAX_UDP_MSG_LEN*sizeof(unsigned int);
 
  message = (unsigned int *)malloc(size+1);
  if(!message) {
    perror("malloc in handle_client_req\n");
    delete(client_sock); // close socket
    return (void *)(-1);
  }
  memset(message, 0, size);  // zero out the message buffer

  // create a message consisting of all hashtable contents
  tracker = message; //create a tracker in the message array
  for(HashMap::const_iterator iter = hashtable->begin(); 
      iter != hashtable->end(); ++iter)
  {
      //int buf[MAX_UDP_MSG_LEN];
      char ip[IP_LEN];

      if(next+MAX_UDP_MSG_LEN > size) {
        size = size*2;
        message = (unsigned int *)realloc(message, size+1);
        if(!message) {
          perror("realloc in handle_client_req\n");
          delete(client_sock); // close socket
          return (void *)(-1);
        }
      }
      HashEntry* ent = iter->second;
      all++;
      if (ent->message[TTL_FIELD]!=0){
        some++;
        parse_IP(iter->first.c_str(), ip);
        if (!(ent->message[CORES_FIELD]==COLLECTOR_CORES
            && ent->message[RAM_FIELD]==COLLECTOR_RAM)) {
          //for a machine running in collector mode, freeMEM=1 and cores=0
          memcpy((void *)tracker, (void *)ent->message, 
              sizeof(unsigned int)*MSGUNITS);
          tracker+=MSGUNITS;
          next += MSGUNITS*sizeof(unsigned int);
          count++;
        }
        else if(DEBUG_PEERMON){
          printf("Running in collector mode\n");
        }
      }
  } 

  if (DEBUG_PEERMON){
    printf("I know %d in total, %d with collector, %d i am sending\n", 
        all, some, count);
    printf("Sending to client Hashtable size %d\n",(int)(hashtable->size()));
    printf("sending message of size %d to client\n",*(&next));
  }

  // send message length and message to client
  client_sock->send((void *)(&next),sizeof(unsigned int));
  client_sock->send(message, next);
  
  // clean up
  free(message); // free buffer
  delete(client_sock); // close socket
  return 0;
}



//*********************************************************************
// listener thread main loop
// This function is responsible for receiving HashEntries from other peers and
//      updating local HashMap as necessary. 
// After decoding a single complete entry referencing a node, the decoded IP
//      is checked to see if it is 'valid' by comparing it with entries in
//      known_IPs. If the entry is valid, there is a further check to see if 
//      the entry already exists in our local HashMap. If not, a new entry is
//      created and insterted, else we check if the data is more recent than
//      the data we know of and make necessary updates in our HashMap
void *listener(void *args){

  thread_args * received_args = (thread_args*)(args);
  HashMap * hashtable = received_args->hashtable;
  UDPSocket sock(port_num); 
  unsigned int inBuffer[(MAX_UDP_MSG_LEN+1)];
  int recvMsgSize;                  // Size of received message
  string sourceAddress;             // Address of datagram source
  unsigned short sourcePort;        // Port of datagram source
  char str[INET_ADDRSTRLEN];
  struct sockaddr_in socket;

  // keep listening til apocalypse/crash/restart, whichever comes first
  while(keep_alive){
    // Block until receive message from a client
    memset(inBuffer, 0, sizeof(unsigned int)*(MAX_UDP_MSG_LEN+1));
    recvMsgSize = 
      sock.recvFrom(inBuffer, MAX_UDP_MSG_LEN, sourceAddress, sourcePort);

    pthread_mutex_lock(&indegree_mutex);
    self_indegree++;
    pthread_mutex_unlock(&indegree_mutex);
    char IP[IP_LEN];
    int msg_counter =0;
    unsigned int data[MSGUNITS];
    if (PEERMON_DEBUG){
      printf("Received %d bytes\n", recvMsgSize);
      printf("From %s:%d\n", sourceAddress.c_str(), sourcePort);
    }    
    
    while(1){
      if (msg_counter + MSGUNITS > MAX_UDP_MSG_LEN){
        break;
      }
      if (msg_counter*4 >= recvMsgSize){
        break;
      }

        for (int j=0; j < MSGUNITS;j++){
          data[j] = inBuffer[j+msg_counter];

         }
        if (PEERMON_DEBUG){
          printf("*************Receiving****************\n");
          for (int j=0;j<MSGUNITS;j++){
          printf("data[%d] is : %u\n", j, data[j]);
          }
        }
      

      socket.sin_addr.s_addr = data[0];
      if (!inet_ntop(AF_INET, &(socket.sin_addr), str, INET_ADDRSTRLEN)){
          perror("Failed to covert IP to printable format");
      }

      str[INET_ADDRSTRLEN]='\0';
      sprintf(IP, "%s:%u", str, data[1]);

      if (PEERMON_DEBUG){
        printf("Received IP is: %s\n", str);
        printf("**************Reception End***********\n");
      }
      msg_counter+=MSGUNITS;;

      
      if (IP_isValid(str)) { //is the IP that we received 'valid'
        if(strcmp(str, self_IP) != 0) {  // don't update my own information
          pthread_mutex_lock(&hashtable_mutex);
          // update entry in hashtable
          HashMap::iterator iter;
          iter = hashtable->find(IP);
          if( iter != hashtable->end() ){
            if(DEBUG_PEERMON){ 
              printf("found %s: ttl %d old %d ideg %d core %d ram %d load %d\n",
                  str, data[2], (unsigned int)iter->second->message[TTL_FIELD], 
                  data[3], data[4], data[5], data[6]);
            }
            // if entry exists in hashtable, update
            if ((unsigned int)iter->second->message[TTL_FIELD] <
                (unsigned int)data[TTL_FIELD])
            {
              iter->second->setmsg(data);
              iter->second->message[INDEG_FIELD] = data[INDEG_FIELD];
              iter->second->message[TTL_FIELD] = data[TTL_FIELD];
            }
          } 
          else{
            // if entry not in hashtable, insert
            if (data[0]!=0){
              if(DEBUG_PEERMON){ 
                printf("adding new hash entry for %s\n",IP);
                printf("%s: ttl %d in_deg %d cores %d ram %d load %d\n", 
                    str, data[2], data[3], data[4], data[5], data[6]);
              }
              HashEntry * dummyhash = new HashEntry(data[INDEG_FIELD], 
                  data[TTL_FIELD],current_time, IP,data);
              hashtable->insert(pair<string,HashEntryPtr>(
                    string(IP),dummyhash));
            }
          }
          pthread_mutex_unlock(&hashtable_mutex);
        } else { // an entry for me...ignore it 
          if(DEBUG_PEERMON){ 
            printf("Got entry for me its ttl %u\n",data[TTL_FIELD]);
          }

        }
      } // if IP_isValid
    }
  } 
  if(DEBUG_PEERMON){ 
    printf("Listener thread dying \n");
  }
  return NULL;
}

//*********************************************************************
//clean-up method for getting rid of the HashEntry instances which are 
//stored in the HashMap
void clean_hashtable(HashMap hashtable){
  for(HashMap::const_iterator iter = hashtable.begin(); 
      iter != hashtable.end(); ++iter)
  {
    delete iter->second;
  }
}

//*********************************************************************
void usage(void){
  fprintf(stderr,
      "Usage: peermon -p portnum [-h] [-c] [-f configfile]"
      " [-l portnum] [-n secs] [-u]\n"
      "   -p  portnum:  use portnum as the peermon listen port\n"
      "                     and portnum+1 for the peermon send port\n"
      "   -h:           print out this help message\n"
      "   -c:           run this peermon deamon in collector-only mode\n"
      "   -f conf_file: run w/conf_file instead of /etc/peermon/machines.txt\n"
      "   -i ip_file:   run w/ip_filr instead of /etc/peermon/valid_ips.txt\n"
      "   -l portnum:   use portnum for client interface (default 1981)\n"
      "   -u:           run at user-level (not as peermon user daemon)\n" 
      "   -n secs:      how often damon sends its info to peers (default 20)\n"
      "   -v:           show version number\n"
      "\n");
}

//*********************************************************************
// ac: argc value passed into main
// av: argv value passed into main
void process_args(int ac, char *av[]){
  int c, p=0;
  while(1){
    c=getopt(ac, av, ":p:i:chuvf:l:n:");   // "p:"  p option has an arg  "ch"  don't
    switch(c){
      case 'h': usage(); exit(0); break;
      case 'v': printf("Peermon Version: %.2f\n", VERSION); exit(0); break;
      case 'p': port_num=atoi(optarg); p = 1; break;
      case 'c': collector_only = 1; break;
      case 'u': user_mode = 1; break;
      case 'f': config_file=optarg; break;
      case 'l': client_port_num=atoi(optarg); break;
      case 'i': ip_file=optarg; break;
      case 'n': 
                sleep_secs=atoi(optarg); 
                if(sleep_secs <= 0){ 
                  sleep_secs = DEFAULT_SLEEP_AMT;
                }
                break;
      case ':': fprintf(stderr, "-%c missing arg\n", optopt); 
                usage(); peermon_exit("Exit from line 852",1); break;
      case '?': fprintf(stderr, "unknown arg %c\n", optopt); 
                usage(); peermon_exit("Exit from line 854", 1); break;
    }
    if(c==-1) break;
  }
  if(!p) { 
    fprintf(stderr,"Error: peermon must be run with command line option -p\n"); 
    usage(); 
    exit(1); 
  }
}

//*********************************************************
//the peermon daemon is initially run as root during startup
//the function drops priviledges so that the daemon can be run
//by the peermon user
void drop_privs(int real_uid, int real_gid) {

    int retvg=setegid(real_gid); // must drop group first
    int retvu=seteuid(real_uid);
    if (retvu == 0 && retvg == 0)
      return;
    if (retvu != 0) {
      syslog(LOG_WARNING, 
          "Error (%d) dropping uid privileges...exiting.", retvu);
      exit(retvu);
    }
    if (retvg != 0) {
      syslog(LOG_WARNING, 
          "Error (%d) dropping gid privileges...exiting.", retvg);
      exit(retvu);
    }
}

/**********************************************************************
Description: This function handles select signals that the daemon may
 receive.  This gives the daemon a chance to properly shut down in 
 emergency situations.  This function is installed as a signal handler 
 in the 'main()' function.
 
Params: The signal received
Returns: void 
*/
void signal_handler(int sig) {
 
  switch(sig) {
    case SIGHUP:
      syslog(LOG_WARNING, "Received SIGHUP signal.");
      break;
    case SIGTERM:
      syslog(LOG_WARNING, "Received SIGTERM signal.");
      break;
    case SIGKILL:
      syslog(LOG_WARNING, "Received SIGKILL signal...exiting.");
      exit(0);
      break;
    default:
      syslog(LOG_WARNING, "Unhandled signal (%d) %s", sig, strsignal(sig));
      break;
  }
}
//********************************************************************* 
// main program. spawns listener, sender, and client threads and 
// waits until they exit
int main(int argc, char *argv[]) {

  char result[IP_LEN];
  int ret;
  int daemonize_peermon;

  current_time = 0;
  use_heuristic = 2; //best heuristic to use

  process_args(argc, argv);
  send_port = port_num+1;

  daemonize_peermon = DAEMONIZE_PEERMON;
  if(user_mode) {
    daemonize_peermon = 0;
  }

  if(daemonize_peermon) { // daemonize this process
    // create an orphaned child that will become a child of init
    pid_t pid, sid; //variables for daemonizing
    struct passwd *ID;
    if (!(ID = getpwnam(DAEMON_NAME))){
      peermon_exit("Peermon user not found on system", 1);
    }
    PEERUID = ID->pw_uid;
    PEERGID = ID->pw_gid; //get uid and gid associated with the user DAEMON_NAME
    
    // Setup signal handling before we start
    signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    syslog(LOG_INFO, "%s daemon starting up", DAEMON_NAME);
    // setup syslog logging...
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

    /* change to peermon user/group -- should do this EARLY */
    if(!user_mode){ 
      drop_privs(PEERUID,PEERGID);
    }

    /* Fork off the parent process */       
    pid = fork();
    /* log the failure...and exit */
    if (pid < 0) { peermon_exit("pid < 0",EXIT_FAILURE); }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {exit(EXIT_SUCCESS); }


    /* child (daemon) continues... */

    /* change file mode mask */
    umask(022);

    /* open log files here?? Not sure about this, since we use syslog... */

    /* Create a new SID for the child process; log if it fails... */
    sid = setsid();
    if (sid < 0) { perror("sid");}
    
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    /* Change the current working dir; log if it fails... */
    if ((chdir("/")) < 0) { peermon_exit("/ does not exist.",EXIT_FAILURE); }
  }

  HashMap hashtable;
  // read in list of initial machines
  readin(&hashtable);
  readPrefix(); //get valid IP prefixes

  srand ( time(NULL) );
  get_own_IP(result);
  sprintf(self_IP,"%s:%d",result, port_num);
  if (DEBUG_PEERMON){printf("Self_IP is: %s\n", self_IP);}
  send_port = port_num+1;
  keep_alive = 1;
  thread_args * args = (thread_args*)malloc(sizeof(thread_args));
  args->hashtable = &hashtable;

  pthread_t listener_thr = 0;
  pthread_t sender_thr = 0;
  pthread_t client_thr = 0;

  // spawn off listener, sender and client interface threads
  ret = pthread_create(&listener_thr,0,listener,(void *)(args));
  if(ret) { perror("Error creating listener_thr"); exit(1); }
  ret = pthread_create(&sender_thr,0,sender,(void *)(args));
  if(ret) { perror("Error creating sender_thr"); exit(1); }
  ret = pthread_create(&client_thr,0,client_interface,(void *)(args));
  if(ret) { perror("Error creating client_thr"); exit(1); }

  //sleep forever, as the two spawned threads are in charge of doing 
  // all the hard work
  ret = pthread_join(listener_thr, NULL);
  if(!ret) { 
    if(DEBUG_PEERMON) {
      printf("listener thread join failed\n");
      perror("pthread_join");
    }
  }
  ret = pthread_join(sender_thr, NULL);
  if(!ret) { 
    if(DEBUG_PEERMON) {
      printf("sender thread join failed\n");
      perror("pthread_join");
    }
  }
  ret = pthread_join(client_thr, NULL);
  if(!ret) { 
    if(DEBUG_PEERMON) {
      printf("client thread join failed\n");
      perror("pthread_join");
    }
  }

  free(args);
  clean_hashtable(hashtable);
  hashtable.clear();
  if(DEBUG_PEERMON){ printf("Hashtable size %d\n",(int)(hashtable.size())); }
  if(DEBUG_PEERMON){ printf("done!\n");}
  syslog(LOG_INFO, "%s daemon exiting\n", DAEMON_NAME);
  return 0;
}
