#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LEOSatelliteRouting");

/** Prints the IP addresses of the nodes */
void printNodeDetails(NodeContainer nodes) {
  Ptr<OutputStreamWrapper> outputStreamWrapper = Create<OutputStreamWrapper>(&std::cout);
  for (uint32_t nodeIndex = 0; nodeIndex < nodes.GetN(); nodeIndex++) {
    Ptr<Node> node = nodes.Get(nodeIndex);
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    std::string nodeName = Names::FindName(node);
    std::cout << "Node: " << nodeIndex;
    if (nodeName.size() > 0) std::cout << " (" << nodeName << ")";
    std::cout << std::endl;
    for (uint32_t interfaceIndex = 0; interfaceIndex < ipv4->GetNInterfaces(); interfaceIndex++) {
      Ipv4InterfaceAddress interfaceAddress = ipv4->GetAddress(interfaceIndex, 0);
      std::cout << "  " << interfaceIndex << ": " << interfaceAddress.GetLocal() << std::endl;
    }
    ipv4->GetRoutingProtocol()->PrintRoutingTable(outputStreamWrapper);
    std::cout << std::endl;
  }
}

int main(int argc, char* argv[]) {
  /** Simulation configs */
  bool printNodeInfo = false;

  /** Simulation constants */
  const int numberOfSatellites = 24;
  const int numberOfConnections = 30;

  /** Parsing the command line arguments */
  CommandLine cmd(__FILE__);
  cmd.AddValue("printNodeInfo", "prints the Interfaces and routing table of all nodes", printNodeInfo);
  cmd.Parse(argc, argv);

  /** Creating the satellite constellation and the connection between the satellites */
  NodeContainer satellites;
  satellites.Create(numberOfSatellites);
  std::tuple<int, int> satelliteConstellation[numberOfConnections] = {
      {2, 0},   {0, 3},   {3, 1},   {1, 4},   {2, 5},   {3, 6},   {4, 7},   {8, 5},   {5, 9},   {9, 6},
      {6, 10},  {10, 7},  {7, 11},  {8, 12},  {9, 13},  {10, 14}, {11, 15}, {12, 16}, {16, 13}, {13, 17},
      {17, 14}, {14, 18}, {18, 15}, {16, 19}, {17, 20}, {18, 21}, {19, 22}, {22, 20}, {20, 23}, {23, 21}};

  /** Creating the source and destination nodes */
  NodeContainer devices;
  devices.Create(2);
  Ptr<Node> sourceDevice = devices.Get(0);
  Names::Add("Source Device", sourceDevice);
  Ptr<Node> destinationDevice = devices.Get(1);
  Names::Add("Destination Device", destinationDevice);

  /** Creating point to point channel connection between the satellites */
  int a, b;
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("1ms"));
  NetDeviceContainer netDeviceContainers[numberOfConnections];
  for (uint32_t index = 0; index < numberOfConnections; index++) {
    std::tie(a, b) = satelliteConstellation[index];
    NodeContainer nodeContainer = NodeContainer(satellites.Get(a), satellites.Get(b));
    netDeviceContainers[index] = p2p.Install(nodeContainer);
  }

  /** Creating internet connection for the devices */
  NodeContainer sourceWithSatellite = NodeContainer(satellites.Get(0), sourceDevice);
  NodeContainer destinationWithSatellite = NodeContainer(satellites.Get(23), destinationDevice);
  NetDeviceContainer sourceNetDevice = p2p.Install(sourceWithSatellite);
  NetDeviceContainer destinationNetDevice = p2p.Install(destinationWithSatellite);

  /** Setting up the routing table */
  RipHelper ripHelper;
  ripHelper.ExcludeInterface(satellites.Get(0), 3);
  ripHelper.ExcludeInterface(satellites.Get(23), 3);
  Ipv4ListRoutingHelper listRoutingHelper;
  listRoutingHelper.Add(ripHelper, 0);

  /** Setting up the internet stack */
  InternetStackHelper internet;
  internet.SetIpv6StackInstall(false);
  internet.SetRoutingHelper(listRoutingHelper);
  internet.Install(satellites);

  InternetStackHelper deviceInternet;
  deviceInternet.SetIpv6StackInstall(false);
  deviceInternet.Install(devices);

  /** Assigning IP addresses to all the nodes */
  Ipv4AddressHelper addressHelper;
  Ipv4Address address;
  Ipv4InterfaceContainer interfaceContainers[numberOfConnections];
  for (uint32_t index = 0; index < numberOfConnections; index++) {
    std::string baseIp = "10.0." + std::to_string(index + 1) + ".0";
    address.Set(baseIp.c_str());
    addressHelper.SetBase(address, "255.255.255.0");
    NetDeviceContainer netDeviceContainer = netDeviceContainers[index];
    interfaceContainers[index] = addressHelper.Assign(netDeviceContainer);
  }

  /** Assigning IP address for the devices */
  addressHelper.SetBase("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer sourceDeviceInterface = addressHelper.Assign(sourceNetDevice);
  addressHelper.SetBase("10.0.31.0", "255.255.255.0");
  Ipv4InterfaceContainer destinationDeviceInterface = addressHelper.Assign(destinationNetDevice);

  /** Setting up static routing for the devices */
  Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(sourceDevice->GetObject<Ipv4>()->GetRoutingProtocol())->SetDefaultRoute("10.0.0.1", 1);
  Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(destinationDevice->GetObject<Ipv4>()->GetRoutingProtocol())
      ->SetDefaultRoute("10.0.31.1", 1);

  /** Printing the topology details */
  if (printNodeInfo) {
    Simulator::Schedule(Seconds(10), &printNodeDetails, satellites);
    Simulator::Schedule(Seconds(10), &printNodeDetails, devices);
  }

  /** Setting up the ping application */
  PingHelper ping(Ipv4Address("10.0.31.2"));
  ping.SetAttribute("Interval", StringValue("1s"));
  ping.SetAttribute("Size", StringValue("1024"));

  ApplicationContainer apps = ping.Install(sourceDevice);
  apps.Start(Seconds(0));
  apps.Stop(Seconds(100));

  /** Running the simulation */
  Simulator::Stop(Seconds(100));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}