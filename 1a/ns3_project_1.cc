
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/athstats-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include <iostream>
using namespace ns3;
//static bool g_verbose = true;
int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  
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
  //wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);  //we might need this for tapping
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  Ssid ssid1 = Ssid ("me");
  Ssid ssid2 = Ssid ("neighbour");
  
  
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");  // need to understand what is this exactly
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid1),
                   "ActiveProbing", BooleanValue (false));
  netDevsMe = wifi.Install (wifiPhy, wifiMac, allNodes.Get(1));
  
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid2),
                   "ActiveProbing", BooleanValue (false));
  netDevsNeighbor = wifi.Install (wifiPhy, wifiMac, allNodes.Get(2));
  
  
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid1));
  NetDeviceContainer myApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(0));
  
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid2));
  NetDeviceContainer neighborApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(3));
  
  netDevsMe.Add(myApContainer);
  netDevsNeighbor.Add(neighborApContainer);
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // my AP
  positionAlloc ->Add(Vector(500, 0, 0)); // my laptop
  positionAlloc ->Add(Vector(1000, 0, 0)); // neighbor laptop
  positionAlloc ->Add(Vector(1500, 0, 0)); // neighbor AP
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(allNodes);  //Need to set the propagation loss according to this
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer myifcont = ipv4.Assign (netDevsMe);
  
  ipv4.SetBase ("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer neighborifcont = ipv4.Assign (netDevsNeighbor);
  
  ApplicationContainer apps;  //Create Application
  
  OnOffHelper onoff ("ns3::UdpSocketFactory",InetSocketAddress ("10.1.1.1", 1025));
  onoff.SetAttribute ("OnTime", StringValue ("Constant:1.0"));
  onoff.SetAttribute ("OffTime", StringValue ("Constant:0.0"));
  onoff.SetConstantRate (DataRate ("11Mb/s"));
  apps = onoff.Install (allNodes.Get(3));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (201.0));
  
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
  InetSocketAddress ("10.1.1.1", 1025));
  apps = sink.Install (allNodes.Get(2));
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (202.0));
  
  Simulator::Stop (Seconds (202.0));  
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}