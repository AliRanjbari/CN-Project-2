/*
    This is a simple topology:
    With a load balancer in the middle
    Three sender and Three reciever
    the senders send messages with UDP protocol to LoadBalancer 
    and the LoadBalancer sends that message to a random reciever with TCP protocol

    * all the connections use 802.11 standard

    (sender)---->|            |----->(receiver)
    (sender)---->|LoadBalancer|----->(receiver)
    (sender)---->|____________|----->(receiver)

*/

#include "ns3/core-module.h"

using namespace ns3;


int
main(int argc, char* argv[]) {

    NS_LOG_UNCOND ("A simple topology\n");
    CommandLine cmd;
    cmd.Parse (argc, argv);
}
