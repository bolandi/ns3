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
#include <stdlib.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("2");

int main(int argc, char *argv[])

{

	// Enable RTS/CTS
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(0));

	// Create nodes
	NodeContainer node_A;
	node_A.Create(1);
	NodeContainer node_a;
	node_a.Create(1);
	NodeContainer node_b;
	node_b.Create(1);
	NodeContainer node_B;
	node_B.Create(1);
	NodeContainer node_C;
	node_C.Create(1);
	NodeContainer node_c;
	node_c.Create(1);

	// Position nodes (fixed position)
	Ptr <ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
	positionAlloc -> Add(Vector(0, 0, 0)); 			// Node a
	positionAlloc -> Add(Vector(5, 0, 0)); 			// Node A
	positionAlloc -> Add(Vector(0, 5, 0)); 			// Node b
	positionAlloc -> Add(Vector(5, 5, 0)); 			// Node B
	positionAlloc -> Add(Vector(0, 10, 0));			// Node c
	positionAlloc -> Add(Vector(5, 10, 0)); 		// Node C
	MobilityHelper	mobility;
	mobility.SetPositionAllocator(positionAlloc);
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	NodeContainer all;
	all.Add(node_a);
	all.Add(node_A);
	all.Add(node_b);
	all.Add(node_B);
	all.Add(node_c);
	all.Add(node_C);
	mobility.Install(all);

	// Set propagation loss according to the topology
	Config::SetDefault("ns3::RangePropagationLossModel::MaxRange", DoubleValue(8.0)); // 8 > sqrt(50)


	// Setup WifiHelper
	WifiHelper wifi = WifiHelper::Default();
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

	// Create non QoS-enabled MAC
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();

	// Setup channel and PHY
	YansWifiChannelHelper wifiChannel;
	wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel");
	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());

	// Create SSIDs
	Ssid ssid_me = Ssid("me");
	Ssid ssid_neighbour_b = Ssid("neighbour_b");
	Ssid ssid_neighbour_c = Ssid("neighbour_c");

	// Setup net devices
	wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid_me), "ActiveProbing", BooleanValue (false));
	NetDeviceContainer dev_a = wifi.Install(wifiPhy, wifiMac, node_a);
	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid_me));
	NetDeviceContainer dev_A = wifi.Install(wifiPhy, wifiMac, node_A);
	wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid_neighbour_b), "ActiveProbing", BooleanValue(false));
	NetDeviceContainer dev_b = wifi.Install(wifiPhy, wifiMac, node_b);
	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid_neighbour_b));
	NetDeviceContainer dev_B = wifi.Install(wifiPhy, wifiMac, node_B);
	wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid_neighbour_c), "ActiveProbing", BooleanValue(false));
	NetDeviceContainer dev_c = wifi.Install(wifiPhy, wifiMac, node_c);
	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid_neighbour_c));
	NetDeviceContainer dev_C = wifi.Install(wifiPhy, wifiMac, node_C);

	// Create internet protocol stack for nodes
	InternetStackHelper internetStack;
	internetStack.Install(node_A);
	internetStack.Install(node_a);
	internetStack.Install(node_b);
	internetStack.Install(node_B);
	internetStack.Install(node_c);
	internetStack.Install(node_C);

	// Assign ipv.4 addresses
	Ipv4AddressHelper ipv4_helper;
	ipv4_helper.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer if_container_a = ipv4_helper.Assign(dev_a);
	Ipv4InterfaceContainer if_container_A = ipv4_helper.Assign(dev_A);
	ipv4_helper.SetBase("10.2.1.0", "255.255.255.0");
	Ipv4InterfaceContainer if_container_b = ipv4_helper.Assign(dev_b);
	Ipv4InterfaceContainer if_container_B = ipv4_helper.Assign(dev_B);
	ipv4_helper.SetBase("10.3.1.0", "255.255.255.0");
	Ipv4InterfaceContainer if_container_c = ipv4_helper.Assign(dev_c);
	Ipv4InterfaceContainer if_container_C = ipv4_helper.Assign(dev_C);


	// Setup packet capture for all nodes
	wifiPhy.EnablePcap("2_A", dev_A, true);
	wifiPhy.EnablePcap("2_a", dev_a, true);
	wifiPhy.EnablePcap("2_B", dev_B, true);
	wifiPhy.EnablePcap("2_b", dev_b, true);
	wifiPhy.EnablePcap("2_C", dev_C, true);
	wifiPhy.EnablePcap("2_c", dev_c, true);

	// Verify toplogy
//	NS_LOG_DEBUG ("Node A ip : " << );

	// Setup applications
	uint16_t sink_port = 8080;
	ApplicationContainer application_b;
	Address sink_address_b (InetSocketAddress(if_container_b.GetAddress(0), sink_port));
	// Server application for consuming UDP traffic
	PacketSinkHelper sink_b ("ns3::UdpSocketFactory", sink_address_b);
	application_b = sink_b.Install(node_b.Get(0));
	application_b.Start(Seconds(0.1));
	application_b.Stop(Seconds(200.0));

	ApplicationContainer application_B;
	OnOffHelper on_off_B ("ns3::UdpSocketFactory", sink_address_b);
	// Constant bit rate for traffic generator at 11 mbps
	on_off_B.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	on_off_B.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	on_off_B.SetAttribute("DataRate", StringValue("11Mbps"));
	on_off_B.SetAttribute ("PacketSize", UintegerValue (1024));
	application_B.Add(on_off_B.Install(node_B.Get(0)));
	application_B.Start(Seconds(0.2));
	application_B.Stop(Seconds(200.0));

	ApplicationContainer application_a;
	Address sink_address_a (InetSocketAddress(if_container_a.GetAddress(0), sink_port));
	// Server application for consuming UDP traffic
	PacketSinkHelper sink_a ("ns3::UdpSocketFactory", sink_address_a);
	application_a = sink_a.Install(node_a.Get(0));
	application_a.Start(Seconds(0.1));
	application_a.Stop(Seconds(200.0));

	ApplicationContainer application_A;
	OnOffHelper on_off_A ("ns3::UdpSocketFactory", sink_address_a);
	// Constant bit rate for traffic generator at 11 mbps
	on_off_A.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	on_off_A.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	on_off_A.SetAttribute("DataRate", StringValue("11Mbps"));
	on_off_A.SetAttribute ("PacketSize", UintegerValue (1024));
	application_A.Add(on_off_A.Install(node_A.Get(0)));
	application_A.Start(Seconds(0.2));
	application_A.Stop(Seconds(200.0));

	ApplicationContainer application_c;
	Address sink_address_c (InetSocketAddress(if_container_c.GetAddress(0), sink_port));
	// Server application for consuming UDP traffic
	PacketSinkHelper sink_c ("ns3::UdpSocketFactory", sink_address_c);
	application_c = sink_c.Install(node_c.Get(0));
	application_c.Start(Seconds(0.1));
	application_c.Stop(Seconds(200.0));

	ApplicationContainer application_C;
	OnOffHelper on_off_C ("ns3::UdpSocketFactory", sink_address_c);
	// Constant bit rate for traffic generator at 11 mbps
	on_off_C.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
	on_off_C.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
	on_off_C.SetAttribute("DataRate", StringValue("11Mbps"));
	on_off_C.SetAttribute ("PacketSize", UintegerValue (1024));
	application_C.Add(on_off_C.Install(node_C.Get(0)));
	application_C.Start(Seconds(0.2));
	application_C.Stop(Seconds(200.0));

	// Setup simulator
	Simulator::Stop(Seconds(200.0));
	Simulator::Run();
	Simulator::Destroy();

	return 0;
}
