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
#include "ns3/propagation-module.h"

using namespace ns3;
//static bool g_verbose = true;
int main (int argc, char *argv[])
{
  /*
  RtsCtsThreshold: If the size of the PSDU is bigger than this value, we use an RTS/CTS handshake before sending the data frame.
  Initial value: 65535
  Set with class: ns3::UintegerValue
  FragmentationThreshold: If the size of the PSDU is bigger than this value, we fragment it such that the size of the fragments are equal or smaller
  Initial value: 2346 
  Set with class: ns3::UintegerValue
  */  
  UintegerValue rtsCtsThreshold = 0;
  
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", rtsCtsThreshold);
//  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");  // need to understand what is this exactly
   wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
            "DataMode", StringValue("DsssRate11Mbps"),
            "ControlMode", StringValue("DsssRate11Mbps"));
  
  NodeContainer allNodes;
  allNodes.Create(4);  //Devices order in the container is as follow: my AP, my laptop, neighbor laptop, neighbor AP.
  
  NetDeviceContainer netDevsMe;
  NetDeviceContainer netDevsNeighbor;
  
  InternetStackHelper internetStack;
  internetStack.Install (allNodes);
  
  //create non QoS-enabled MAC layers for a ns3::WifiNetDevice
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  //wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);  //we might need this for tapping
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  Ssid ssid1 = Ssid ("me");
  Ssid ssid2 = Ssid ("neighbour");
  
  
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid1), "ActiveProbing", BooleanValue (false));
  netDevsMe = wifi.Install (wifiPhy, wifiMac, allNodes.Get(1));
  
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid2), "ActiveProbing", BooleanValue (false));
  netDevsNeighbor = wifi.Install (wifiPhy, wifiMac, allNodes.Get(2));
  
  
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid1));
  NetDeviceContainer myApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(0));
  
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid2));
  NetDeviceContainer neighborApContainer = wifi.Install (wifiPhy, wifiMac, allNodes.Get(3));
  
  netDevsMe.Add(myApContainer);
  netDevsNeighbor.Add(neighborApContainer);

  
  /**************************** must read **************************************
	https://www.nsnam.org/docs/release/3.11/manual/html/object-model.html
  ******************************************************************************/
  
   // Place nodes somehow, this is required by every wireless simulation. In this case consant position since we assume they do no move
   for (size_t i = 0; i < 4; i++)
   {
       allNodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
   }
  
   //https://www.nsnam.org/doxygen/classns3_1_1_matrix_propagation_loss_model.html#details
   //Create propagation loss matrix
   Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
   lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
   lossModel->SetLoss (allNodes.Get (0)->GetObject<MobilityModel>(), allNodes.Get (1)->GetObject<MobilityModel>(), 50); // set symmetric loss 0 <-> 1 to 50 dB
   lossModel->SetLoss (allNodes.Get (1)->GetObject<MobilityModel>(), allNodes.Get (2)->GetObject<MobilityModel>(), 50); // set symmetric loss 2 <-> 1 to 50 dB
   lossModel->SetLoss (allNodes.Get (2)->GetObject<MobilityModel>(), allNodes.Get (3)->GetObject<MobilityModel>(), 50); // set symmetric loss 2 <-> 3 to 50 dB
  
/*  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // my AP
  positionAlloc ->Add(Vector(500, 0, 0)); // my laptop
  positionAlloc ->Add(Vector(1000, 0, 0)); // neighbor laptop
  positionAlloc ->Add(Vector(1500, 0, 0)); // neighbor AP
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(allNodes);  //Need to set the propagation loss according to this
*/  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer myifcont_a = ipv4.Assign (netDevsMe);
  Ipv4InterfaceContainer myifcont_A = ipv4.Assign (myApContainer);
  
  ipv4.SetBase ("10.2.1.0", "255.255.255.0");
  Ipv4InterfaceContainer neighborifcont_b = ipv4.Assign (netDevsNeighbor);
  Ipv4InterfaceContainer neighborifcont_B = ipv4.Assign (neighborApContainer);
  
  
  ApplicationContainer apps;  //Create Application

  // [TODO] Is b address 10.2.1.1? should be taken from node itself
  Address b_address (InetSocketAddress ("10.2.1.1", 4493));
  PacketSinkHelper sink ("ns3::UdpSocketFactory", b_address);

  std::cout<<"so far so good\n";

/*  
  OnOffHelper onoff ("ns3::UdpSocketFactory",InetSocketAddress ("10.1.1.1", 1025));
  onoff.SetAttribute ("OnTime", StringValue ("Constant:1.0"));
  onoff.SetAttribute ("OffTime", StringValue ("Constant:0.0"));
  onoff.SetConstantRate (DataRate ("11Mb/s"));
  apps = onoff.Install (allNodes.Get(3));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (201.0));
  
  // apps = sink.Install (allNodes.Get(2));
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (202.0));
*/ 
 
 
  /*
  void ns3::YansWifiPhyHelper::EnablePcapInternal	(	std::string 	prefix,
Ptr< NetDevice > 	nd,
bool 	promiscuous,
bool 	explicitFilename 
)		
privatevirtual
Enable pcap output the indicated net device.

NetDevice-specific implementation mechanism for hooking the trace and writing to the trace file.

Parameters
prefix	Filename prefix to use for pcap files.
nd	Net device for which you want to enable tracing.
promiscuous	If true capture all possible packets available at the device.
explicitFilename	Treat the prefix as an explicit filename if true
*/

  //Enable capture
  wifiPhy.EnablePcap("a", netDevsMe); 
  wifiPhy.EnablePcap("A", myApContainer); 
  wifiPhy.EnablePcap("b", netDevsNeighbor); 
  wifiPhy.EnablePcap("B", neighborApContainer); 
  
  
  Simulator::Stop (Seconds (202.0));  
  
  std::cout<<"Starting simulation\n";
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout<<"simulation ran successfully\n";
  return 0;
}