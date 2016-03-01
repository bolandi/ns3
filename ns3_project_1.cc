#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/athstats-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address-helper.h"
#include <iostream>

	
using namespace ns3;
//static bool g_verbose = true;
int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault( "ns3::RangePropagationLossModel::MaxRange", DoubleValue( 6.0 ) );
  
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  
  NodeContainer allNodes;
  allNodes.Create(4);  //Devices order in the container is as follow: my AP, my laptop, neighbor laptop, neighbor AP.

  
  NetDeviceContainer netDevsMe;
  NetDeviceContainer netDevsNeighbor;
  
  InternetStackHelper internetStack;
  internetStack.Install (allNodes);
  
  
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);  //we might need this for tapping
  YansWifiChannelHelper wifiChannel;
  	wifiChannel.SetPropagationDelay( "ns3::ConstantSpeedPropagationDelayModel" );
	wifiChannel.AddPropagationLoss(  "ns3::RangePropagationLossModel" );
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  
  Ssid ssid1 = Ssid ("me");
  Ssid ssid2 = Ssid ("neighbour");
  
  
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");  // need to understand what is this exactly
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid1),
                   "ActiveProbing", BooleanValue (false));
  NetDeviceContainer netDevsMeSta = wifi.Install (wifiPhy, wifiMac, allNodes.Get(1));
  
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid2),
                   "ActiveProbing", BooleanValue (false));
  NetDeviceContainer netDevsNeighborSta = wifi.Install (wifiPhy, wifiMac, allNodes.Get(2));
  
  
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid1));
  NetDeviceContainer myApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(0));
  
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid2));
  NetDeviceContainer neighborApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(3));
  
  
  netDevsMe.Add(netDevsMeSta);
  netDevsNeighbor.Add(netDevsNeighborSta);
  netDevsMe.Add(myApContainer);
  netDevsNeighbor.Add(neighborApContainer);
  
  wifiPhy.EnablePcap ("mydevices", netDevsMe);
  wifiPhy.EnablePcap ("neighbor", netDevsNeighbor);
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // my AP
  positionAlloc ->Add(Vector(5, 0, 0)); // my laptop
  positionAlloc ->Add(Vector(10, 0, 0)); // neighbor laptop
  positionAlloc ->Add(Vector(15, 0, 0)); // neighbor AP
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(allNodes);  //Need to set the propagation loss according to this
  
    //////////////////////

  Ipv4AddressHelper ipv4me;
  ipv4me.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer myifcont = ipv4me.Assign (netDevsMe);
  
  Ipv4AddressHelper ipv4neighbor;
  ipv4neighbor.SetBase ("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer neighborifcont = ipv4neighbor.Assign (netDevsNeighbor);
  
  uint16_t sinkPort = 8080;

  Address sinkAddress (InetSocketAddress (neighborifcont.GetAddress (1),sinkPort));
  
  ApplicationContainer sourceAp;  //Create Application
  
  OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkAddress);
  onOffHelper.SetAttribute ("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onOffHelper.SetAttribute ("DataRate",StringValue ("54Mbps"));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));
  
  sourceAp.Add(onOffHelper.Install (allNodes.Get(2)));
  sourceAp.Start (Seconds (1.0));
  sourceAp.Stop (Seconds (201.0));
  
  ApplicationContainer sinkapp;
  PacketSinkHelper sink ("ns3::UdpSocketFactory",InetSocketAddress (neighborifcont.GetAddress (1),sinkPort));
  sinkapp = sink.Install (allNodes.Get(3));
  sinkapp.Start (Seconds (0.0));
  sinkapp.Stop (Seconds (202.0));

  ///////////////////////////
  /*
    Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");


  Ipv4InterfaceContainer interfaces = address.Assign (netDevsNeighbor);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (allNodes.Get (3));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (allNodes.Get (2));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  */
  
  Simulator::Stop (Seconds (30.0));  
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}



