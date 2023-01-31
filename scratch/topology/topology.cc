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
#include "ns3/error-rate-model.h"
#include "load-balancer.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gnuplot.h"


// Network Topology
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
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
{
  double localThrou = 0;
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
      localThrou = stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds () - stats->second.timeFirstTxPacket.GetSeconds ()) / 1024 / 1024;
      if (stats->first == 1)
        {
          DataSet.Add ((double)Simulator::Now ().GetSeconds (),(double) localThrou);
        }
      std::cout << "---------------------------------------------------------------------------" << std::endl;
    }
  Simulator::Schedule (Seconds (10),&ThroughputMonitor, fmhelper, flowMon,DataSet);
  flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
}


NS_LOG_COMPONENT_DEFINE ("Topology");

int main(int argc, char *argv[]) {

  double error = 0.000001;  
  int bandWidth = 100;      // Mbps

  CommandLine cmd;
  cmd.AddValue ("bandWidth", "Band Width of the network", bandWidth);
  cmd.AddValue ("error", "Packet error rate", error);

  cmd.Parse (argc, argv);


  LogComponentEnable ("Topology", LOG_LEVEL_ALL);
  LogComponentEnable ("LoadBalancer", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);



	// Create three sender and three receivers
	NodeContainer senderNodes;
	NodeContainer receiverNodes;
	NodeContainer loadBalancerNode;   
	senderNodes.Create (3);
	receiverNodes.Create (3);
  loadBalancerNode.Create (1);


  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (error));


	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy;
	phy.SetChannel (channel.Create ());

  // phy.Set ("ChannelWidth", UintegerValue (bandWidth));

	WifiHelper wifi;
  wifi.SetStandard (WifiStandard::WIFI_STANDARD_80211a);            // set standard to 802.11
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");


  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  // mac.SetType ("ns3::AdhocWifiMac");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDeviceSender;
  NetDeviceContainer staDeviceReceiver;
  staDeviceSender = wifi.Install (phy, mac, senderNodes);
  staDeviceReceiver = wifi.Install (phy, mac, receiverNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDeviceLoadBalancer;
  apDeviceLoadBalancer = wifi.Install (phy, mac, loadBalancerNode);


  // staDeviceReceiver.Get (0)->getph

  // Create error model on receiver.

  // for (uint16_t i = 0; i < staDeviceReceiver.GetN (); i++)
  //   staDeviceReceiver.Get (i)->SetAttribute ("ReceiveErrorModel", PointerValue (em));



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

  // Senders
  UdpEchoClientHelper echoClient (loadBalancerInterface.GetAddress (0), port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10.0));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
 
  ApplicationContainer clientApps = echoClient.Install (senderNodes);
  clientApps.Get (0)->SetStartTime (Seconds (0.0));
  clientApps.Get (0)->SetStopTime (Seconds (5.0));
  clientApps.Get (1)->SetStartTime (Seconds (3.0));
  clientApps.Get (1)->SetStopTime (Seconds (8.0));
  clientApps.Get (2)->SetStartTime (Seconds (4.0));
  clientApps.Get (2)->SetStopTime (Seconds (10.0));
  

  // Load Balancer
  Ptr<LoadBalancer> loadBalancerApp = CreateObject<LoadBalancer> (port, receiverInterface);
  loadBalancerNode.Get (0)->AddApplication (loadBalancerApp);
  loadBalancerApp->SetStartTime (Seconds (0.0));
  loadBalancerApp->SetStopTime (Seconds (10.0));


  // Receivers
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (receiverNodes);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();



  Gnuplot2dDataset dataset;
  dataset.SetTitle ("Throupute");
  dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  // Flow monitor.
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll ();

  ThroughputMonitor (&flowHelper, flowMonitor, dataset);


  Simulator::Stop (Seconds (10.0));

	// Start the simulation
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
