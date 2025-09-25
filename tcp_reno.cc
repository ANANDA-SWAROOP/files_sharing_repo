/* tcp-reno-tahoe.cc
  hey erripuk-kalyan
  you owe me yet another time
  install gnuplot
  run this file using ns3
  this will give a .plt file with the name cwnd-reno.plt
  run gnuplot cwnd-reno.plt
  then that will give you cwnd-reno.png and then open it 
  f-you
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/gnuplot.h"
#include "ns3/tcp-congestion-ops.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace ns3;

static Gnuplot2dDataset cwndDataset;

// callback to trace cwnd
static void
CwndTracer (uint32_t oldCwnd, uint32_t newCwnd)
{
  double t = Simulator::Now ().GetSeconds ();
  cwndDataset.Add (t, static_cast<double> (newCwnd));
}

// helper to connect the trace (scheduled after apps start)
static void
TraceCwnd ()
{
  Config::ConnectWithoutContext (
    "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
    MakeCallback (&CwndTracer));
}

int
main (int argc, char *argv[])
{
  std::string tcpVariant = "Reno";  // "Reno" or "Tahoe"
  double simTime = 10.0;            // seconds
  std::string dataRate = "5Mbps";
  std::string delay = "2ms";
  uint16_t port = 50000;

  CommandLine cmd;
  cmd.AddValue ("tcpVariant", "TCP variant: Reno or Tahoe", tcpVariant);
  cmd.AddValue ("simTime", "Simulation time (s)", simTime);
  cmd.AddValue ("dataRate", "Link data rate", dataRate);
  cmd.AddValue ("delay", "Link delay", delay);
  cmd.Parse (argc, argv);

  // Select TCP variant
  if (tcpVariant == "Reno")
    {
      // In ns-3.42, Reno is exposed as TcpNewReno
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TypeId::LookupByName ("ns3::TcpNewReno")));
    }
  else if (tcpVariant == "Tahoe")
    {
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TypeId::LookupByName ("ns3::TcpTahoe")));
    }
  else
    {
      std::cout << "Unknown --tcpVariant (use Reno or Tahoe). Exiting.\n";
      return 1;
    }

  // --- Topology: 2 nodes with P2P ---
  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (delay));
  NetDeviceContainer devices = p2p.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // --- Applications ---
  // Receiver
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                               InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (1));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (simTime + 1.0));

  // Sender
  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (interfaces.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0)); // unlimited
  ApplicationContainer sourceApp = source.Install (nodes.Get (0));
  sourceApp.Start (Seconds (1.0));
  sourceApp.Stop (Seconds (simTime + 1.0));

  // --- Prepare dataset ---
  cwndDataset.SetTitle (tcpVariant + " cwnd");
  cwndDataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

  // schedule trace connection after sender starts
  Simulator::Schedule (Seconds (1.01), &TraceCwnd);

  // --- Run sim ---
  Simulator::Stop (Seconds (simTime + 1.5));
  Simulator::Run ();

  // --- Throughput ---
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApp.Get (0));
  uint64_t totalBytes = sink1->GetTotalRx ();
  double throughput_mbps = (double) totalBytes * 8.0 / simTime / 1e6;
  std::cout << "TCP variant: " << tcpVariant << "\n";
  std::cout << "Total bytes received: " << totalBytes << " bytes\n";
  std::cout << "Average throughput: " << throughput_mbps << " Mbps\n";

  // --- Gnuplot ---
  std::string pngFile = std::string ("cwnd-") + tcpVariant + ".png";
  std::string pltFile = std::string ("cwnd-") + tcpVariant + ".plt";
  Gnuplot gnuplotCwnd (pngFile);
  gnuplotCwnd.SetTitle ("Congestion Window vs Time (" + tcpVariant + ")");
  gnuplotCwnd.SetTerminal ("png");
  gnuplotCwnd.SetLegend ("Time (s)", "Cwnd (bytes)");
  gnuplotCwnd.AddDataset (cwndDataset);

  std::ofstream plotFile (pltFile.c_str ());
  gnuplotCwnd.GenerateOutput (plotFile);
  plotFile.close ();

  Simulator::Destroy ();
  return 0;
}
