/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* This is a simple topology:
* With a load balancer in the middle
* Three sender and Three reciever
* the senders send messages with UDP protocol to LoadBalancer 
* and the LoadBalancer sends that message to a random reciever with TCP protocol
* 
* Note that all the connections use 802.11 standard
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ssid.h"
#include "load-balancer.h"

// Default Network Topology
//
//  wifi 10.2.0       wifi 10.1.1.0       wifi 10.1.3.0
//
// * (sender)-*                                       *-(receiver)
// * (sender)-*   UDP   *-(LoadBalancer)-*   TCP      *-(receiver)
// * (sender)-*                                       *-(receiver)
//
//


using namespace ns3;

void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon)
{
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats ();
  Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier ());
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
    {
      Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
      std::cout << "Flow ID			: "<< stats->first << " ; " << fiveTuple.sourceAddress << " -----> " << fiveTuple.destinationAddress << std::endl;
      std::cout << "Tx Packets = " << stats->second.txPackets << std::endl;
      std::cout << "Rx Packets = " << stats->second.rxPackets << std::endl;
      std::cout << "Duration		: "<< (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) << std::endl;
      std::cout << "Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds () << " Seconds" << std::endl;
      std::cout << "Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024  << " Mbps" << std::endl;
      if (stats->first == 1)
        {
        }
      std::cout << "---------------------------------------------------------------------------" << std::endl;
    }
  flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
}



NS_LOG_COMPONENT_DEFINE ("Topology");

int main(int argc, char *argv[]) {

  LogComponentEnable ("Topology", LOG_LEVEL_ALL);
  LogComponentEnable ("LoadBalancer", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);


	// Create three sender and three receivers
	NodeContainer senderNodes;
	NodeContainer receiverNodes;
	NodeContainer loadBalancerNode;   
	senderNodes.Create (3);
	receiverNodes.Create (3);
  loadBalancerNode.Create (1);



	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy;
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");

  mac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer staDeviceSender;
  NetDeviceContainer staDeviceReceiver;
  NetDeviceContainer apDeviceLoadBalancer;

  staDeviceSender = wifi.Install (phy, mac, senderNodes);
  staDeviceReceiver = wifi.Install (phy, mac, receiverNodes);
  apDeviceLoadBalancer = wifi.Install (phy, mac, loadBalancerNode);



  // Now define the mobility of devices we assume all device are standstill

  MobilityHelper mobility;


  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (senderNodes);
  mobility.Install (receiverNodes);
  mobility.Install (loadBalancerNode);


  InternetStackHelper stack;
  stack.Install (loadBalancerNode);
  stack.Install (senderNodes);
  stack.Install (receiverNodes);


  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer loadBalancerInterface;
  Ipv4InterfaceContainer senderInterface, receiverInterface;
  loadBalancerInterface = address.Assign (apDeviceLoadBalancer);
  senderInterface = address.Assign (staDeviceSender);
  receiverInterface = address.Assign (staDeviceReceiver);

  uint16_t port = 8000;


  UdpEchoClientHelper echoClient (loadBalancerInterface.GetAddress (0), port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10.0));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
 
  ApplicationContainer clientApps = echoClient.Install (senderNodes);
  clientApps.Get (0)->SetStartTime (Seconds (0.0));
  clientApps.Get (0)->SetStopTime (Seconds (3.0));
  clientApps.Get (1)->SetStartTime (Seconds (4.0));
  clientApps.Get (1)->SetStopTime (Seconds (7.0));
  clientApps.Get (2)->SetStartTime (Seconds (8.0));
  clientApps.Get (2)->SetStopTime (Seconds (10.0));
  


  Ptr<LoadBalancer> loadBalancerApp = CreateObject<LoadBalancer> (port, receiverInterface);
  loadBalancerNode.Get (0)->AddApplication (loadBalancerApp);
  loadBalancerApp->SetStartTime (Seconds (0.0));
  loadBalancerApp->SetStopTime (Seconds (10.0));


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  Simulator::Stop (Seconds (10.0));

	// Start the simulation
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
