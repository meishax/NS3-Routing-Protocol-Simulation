/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

/*
  * Copyright (c) 2021 Meijia Xu
  * 徐美佳 2120200437 南开大学 NS3网络仿真及协议分析 
*/

#include <fstream>
#include <iostream>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/dsdv-helper.h"
#include "ns3/dsr-helper.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/aodv-helper.h"
#include "ns3/point-to-point-module.h" 
#include "ns3/wifi-module.h"  
#include "ns3/v4ping-helper.h"
#include "ns3/flow-monitor-module.h"

 
using namespace ns3;
using namespace dsr;
 
NS_LOG_COMPONENT_DEFINE ("NS3ManetRoutingCompare-Xu");
 
class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName);

  std::string CommandSetup (int argc, char **argv);
 
private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);  //设置包接受
  void ReceivePacket (Ptr<Socket> socket);  //接受包
  void CheckThroughput ();  //检查吞吐量
 
  uint32_t port;               //端口，这里与aodv示例一样设置为9
  uint32_t bytesTotal;  //所有节点接收的byte数
  uint32_t packetsReceived; //所有节点接收的包数
 
  std::string m_CSVfileName; //保存csv文件名称
  int m_nSinks;  //接收器节点的数量（receiver）
  std::string m_protocolName; //协议名称，四个协议名称
  double m_txp; //能量
  bool m_traceMobility; //是否 psap trace
  uint32_t m_protocol;  //协议代号
};
 
RoutingExperiment::RoutingExperiment ()
  : port (9),  //port，这里与aodv示例一样设置为9
    bytesTotal (0),  //初始化总byte为0
    packetsReceived (0), //初始化总收包数为0
    m_CSVfileName ("manet-routing.output.csv"),  //输出csv文件
    m_traceMobility (true), //psap trace 设置false
    m_protocol (2) // 默认aodv
 {
 }
 

//打印接收包信息
static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;
 
  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();
 
  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}
 
//接收包 
void RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize (); //byte加一下
      packetsReceived += 1; //收包数+1
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}
 
void RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000; //计算kbs
  bytesTotal = 0;
 
  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);
 
  out << (Simulator::Now ()).GetSeconds () << "," //当前秒数
      << kbs << "," //kbs数
      << packetsReceived << "," //收包数
      << m_nSinks << ","  //receiver
      << m_protocolName << ","  //协议名
      << m_txp << ""  //能量
      << std::endl;
 
   out.close ();
   packetsReceived = 0;
   Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}
 
//设置包接收
Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));
 
  return sink;//返回receiver
}

//cmd参数
std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
 }
 
 int main (int argc, char *argv[])
 {
  RoutingExperiment experiment; //运行实验
  std::string CSVfileName = experiment.CommandSetup (argc,argv);
 
   //清空最后一个输出文件并写入列标题 
   std::ofstream out (CSVfileName.c_str ());//输出至csv
   out << "SimulationSecond," <<
   "ReceiveRate," <<
   "PacketsReceived," <<
   "NumberOfSinks," <<
   "RoutingProtocol," <<
   "TransmissionPower" <<
   std::endl;
   out.close ();
 
   int nSinks = 10;
   double txp = 7.5;
 
   experiment.Run (nSinks, txp, CSVfileName);
 }
 
void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName)
{
  Packet::EnablePrinting ();
  m_nSinks = nSinks;  
  m_txp = txp;
  m_CSVfileName = CSVfileName;
 
  int nWifis = 50;  //节点总数
 
  double TotalTime = 200.0;
  std::string rate ("2048bps");  //网络速度2048bps
  std::string phyMode ("DsssRate11Mbps");  //物理模式DsssRate11Mbps
  std::string tr_name ("manet-routing-compare"); //生成的trace name
  int nodeSpeed = 20; //in m/s 节点速度
  int nodePause = 0; //in s  节点停顿
  m_protocolName = "protocol";

  uint32_t SentPackets = 0;
	uint32_t ReceivedPackets = 0;
	uint32_t LostPackets = 0;
	bool TraceMetric = true;在这里插入代码片
 
  //设置包大小与数据速率
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));
 
  //设置Non-unicastMode为单播模式
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
 
  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);
 
  //用helper设置wifi物理模式和信道
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
 
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());
 
  //添加mac和禁用速率控制
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode",StringValue (phyMode),
                                 "ControlMode",StringValue (phyMode));
 
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
 
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);
 
  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; //用于在不同场景下获得一致的移动性
 
  //范围
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
 
  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);
 
  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                   "Speed", StringValue (ssSpeed.str ()),
                                   "Pause", StringValue (ssPause.str ()),
                                   "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);
  NS_UNUSED (streamIndex); // 禁用streamIndex
 
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;
 

  //协议选择
  switch (m_protocol)
  {
    case 1:
      list.Add (olsr, 100);
      m_protocolName = "OLSR";
      break;
    case 2:
      list.Add (aodv, 100);
      m_protocolName = "AODV";
      break;
    case 3:
      list.Add (dsdv, 100);
      m_protocolName = "DSDV";
      break;
    case 4:
      m_protocolName = "DSR";
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << m_protocol);
  }
 
  if (m_protocol < 4)
    {
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    }
   else if (m_protocol == 4)
    {
      internet.Install (adhocNodes);
      dsrMain.Install (dsr, adhocNodes);
    }
 
  NS_LOG_INFO ("assigning ip address");
  

  //IP地址
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);
 
  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
 
  for (int i = 0; i < nSinks; i++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), adhocNodes.Get (i));
 
      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
      onoff1.SetAttribute ("Remote", remoteAddress);
 
      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + nSinks));
      temp.Start (Seconds (var->GetValue (100.0,101.0)));
      temp.Stop (Seconds (TotalTime));
    }
 
  std::stringstream ss;
  ss << nWifis;
  std::string nodes = ss.str ();
 
  std::stringstream ss2;
  ss2 << nodeSpeed;
  std::string sNodeSpeed = ss2.str ();
 
  std::stringstream ss3;
  ss3 << nodePause;
  std::string sNodePause = ss3.str ();
 
  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();
  //trace 结构
  //NS_LOG_INFO ("Configure Tracing.");
  //tr_name = tr_name + "_" + m_protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" + sNodePause + "pause_" + sRate + "rate";
 
  //AsciiTraceHelper ascii;
  //Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (tr_name + ".tr").c_str());
  //wifiPhy.EnableAsciiAll (osw);
  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));
 
  //Ptr<FlowMonitor> flowmon;
  //FlowMonitorHelper flowmonHelper;
  //flowmon = flowmonHelper.InstallAll ();
 
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  
  NS_LOG_INFO ("Run Simulation.");
 
  CheckThroughput ();

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();
 

   //flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);

  if (TraceMetric)
	{
		int j=0;
		float AvgThroughput = 0;
		Time Jitter;
		Time Delay;

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
		{
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        NS_LOG_UNCOND("----Flow ID:" <<iter->first);
        NS_LOG_UNCOND("Src Addr" <<t.sourceAddress << "Dst Addr "<< t.destinationAddress);
        NS_LOG_UNCOND("Sent Packets=" <<iter->second.txPackets);
        NS_LOG_UNCOND("Received Packets =" <<iter->second.rxPackets);
        NS_LOG_UNCOND("Lost Packets =" <<iter->second.txPackets-iter->second.rxPackets);
        NS_LOG_UNCOND("Packet delivery ratio =" <<iter->second.rxPackets*100/iter->second.txPackets << "%");
        NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%");
        NS_LOG_UNCOND("Delay =" <<iter->second.delaySum);
        NS_LOG_UNCOND("Jitter =" <<iter->second.jitterSum);
        NS_LOG_UNCOND("Throughput =" <<iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");

        SentPackets = SentPackets +(iter->second.txPackets);
        ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
        LostPackets = LostPackets + (iter->second.txPackets-iter->second.rxPackets);
        AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024);
        Delay = Delay + (iter->second.delaySum);
        Jitter = Jitter + (iter->second.jitterSum);

        j = j + 1;

		}

		AvgThroughput = AvgThroughput/j;
		NS_LOG_UNCOND("--------Total Results of the simulation----------"<<std::endl);
		NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
		NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
		NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
		NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets*100)/SentPackets)<< "%");
		NS_LOG_UNCOND("Average Throughput =" << AvgThroughput<< "Kbps");
		NS_LOG_UNCOND("End to End Delay =" << Delay);
		NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
		NS_LOG_UNCOND("Total Flod id " << j);
		monitor->SerializeToXmlFile("manet-routing.xml", true, true);
	}
 
  Simulator::Destroy ();
}
 
