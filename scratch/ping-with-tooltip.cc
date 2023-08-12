#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/visualizer-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
  GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::VisualSimulatorImpl"));

  // Create nodes
  NodeContainer nodes;
  nodes.Create(2);

  // Install Internet stack
  InternetStackHelper stack;
  stack.Install(nodes);

  for (uint32_t i = 0; i < nodes.GetN(); ++i) {
    Ptr<Node> node = nodes.Get(i);
    Names::Add(std::to_string(i), node);
    // node->AggregateObject(Names::Find<Node>(std::to_string(i)));
  }

  // Assign IP addresses to nodes
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("1ms"));
  NetDeviceContainer ndc = p2p.Install(nodes);

  Ipv4AddressHelper addressHelper;
  addressHelper.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = addressHelper.Assign(ndc);

  // Create a Ping application on Node 0 (Source) to ping Node 1 (Destination)
  PingHelper pingApp(interfaces.GetAddress(1, 0));
  // pingApp.SetAttribute("Destination", AddressValue(InetSocketAddress(interfaces.GetAddress(1, 0), 0)));  // Set destination IP address
  ApplicationContainer apps = pingApp.Install(nodes.Get(0));  // Install on source node
  apps.Start(Seconds(0));
  apps.Stop(Seconds(2));

  // Enable ns-3 Visualizer

  // Create a NodeContainer for visualization
  // NodeContainer visualNodes;
  // visualNodes.Add(nodes);

  // Set tooltips for nodes

  // Run simulation
  Simulator::Stop(Seconds(2.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
