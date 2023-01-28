/*
    This is a simple topology:
    With a load balancer in the middle
    Three sender and Three reciever
    the senders send messages with UDP protocol to LoadBalancer 
    and the LoadBalancer sends that message to a random reciever with TCP protocol

    * all the connections use 802.11 standard

    (sender)-*                          *-(receiver)
    (sender)-*    *-(LoadBalancer)-*    *-(receiver)
    (sender)-*                          *-(receiver)

*/
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "load-balancer.h"

using namespace ns3;

int main(int argc, char *argv[]) {

	// Create three sender and three receivers
	NodeContainer senderNodes;
	NodeContainer receiverNodes;
	senderNodes.Create(3);
	receiverNodes.Create(3);

	// Create load balancer node and add LB applicatoin
	NodeContainer loadBalancerNode;   
	Ptr<LoadBalancer> loadBalancerApplication = CreateObject<LoadBalancer>();
	loadBalancerNode.Get (0)->AddApplication (loadBalancerApplication);


	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy;
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");

  // Create mac for Station nodes
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));


  NetDeviceContainer staDeviceSender;
  NetDeviceContainer staDeviceReceiver;
  staDeviceSender = wifi.Install (phy, mac, senderNodes);
  staDeviceReceiver = wifi.Install (phy, mac, receiverNodes);

  // Create mac for Access Point node
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue(ssid));

  NetDeviceContainer apDeviceLoadBalancer;
  apDeviceLoadBalancer = wifi.Install (phy, mac, loadBalancerNode);



  // ..................... we can add mobility here !



  InternetStackHelper stack;
  stack.Install (loadBalancerNode);
  stack.Install (senderNodes);
  stack.Install (receiverNodes);


  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer loadBalancerInterface;
  loadBalancerInterface = address.Assign (apDeviceLoadBalancer);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer senderInterface;
  senderInterface = address.Assign (staDeviceSender);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer receiverInterface;
  receiverInterface = address.Assign (staDeviceReceiver);

	
	// Start the simulation
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
