 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *        	 	  Sourjya Dutta <sdutta@nyu.edu>
 *        	 	  Russell Ford <russell.ford@nyu.edu>
 *        		  Menglei Zhang <menglei@nyu.edu>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-helper.h"
#include <ns3/buildings-helper.h>
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"


#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("mmWaveBeamSweepingExample");


// parameters that can be set from the command line
static ns3::GlobalValue g_scenario("scenario",
    "The scenario for the simulation. Choose among 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', 'InH-OfficeOpen', 'InH-ShoppingMall'",
    ns3::StringValue("UMi-StreetCanyon"), ns3::MakeStringChecker());
static ns3::GlobalValue g_enableBuildings("enableBuildings", "If true, use MmWave3gppBuildingsPropagationLossModel, else use MmWave3gppPropagationLossModel",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());
static ns3::GlobalValue g_losCondition("losCondition",
    "The LOS condition for the simulation, if MmWave3gppPropagationLossModel is used. Choose 'l' for LOS only, 'n' for NLOS only, 'a' for the probabilistic model",
    ns3::StringValue("a"), ns3::MakeStringChecker());
static ns3::GlobalValue g_optionNlos("optionNlos", "If true, use the optional NLOS pathloss equation from 3GPP TR 38.900",
    ns3::BooleanValue(true), ns3::MakeBooleanChecker());


class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
	static int send_num = 1;
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
	m_socket->Send (packet);
	NS_LOG_DEBUG ("Sending:    "<<send_num++ << "\t" << Simulator::Now ().GetSeconds ());

  	if (++m_packetsSent < m_nPackets)
	{
	    ScheduleTx ();
	}
}

static void Rx (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize()<< std::endl;
}


void
MyApp::ScheduleTx (void)
{
	if (m_running)
	{
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
	}
}


static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}



int 
main (int argc, char *argv[])
{

//  CommandLine cmd;
//  cmd.Parse (argc, argv);

  // parse the command line options
    BooleanValue booleanValue;
    StringValue stringValue;
    GlobalValue::GetValueByName("scenario", stringValue); // set the scenario
    std::string scenario = stringValue.Get();
    GlobalValue::GetValueByName("enableBuildings", booleanValue); // use buildings or not
    bool enableBuildings = booleanValue.Get();
    GlobalValue::GetValueByName("losCondition", stringValue); // set the losCondition
    std::string condition = stringValue.Get();
    GlobalValue::GetValueByName("optionNlos", booleanValue); // use optional NLOS equation
    bool optionNlos = booleanValue.Get();

    // attributes that can be set for this channel model
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue(condition));
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue(scenario));
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue(optionNlos));
    Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(true)); // enable or disable the shadowing effect
    Config::SetDefault ("ns3::MmWave3gppBuildingsPropagationLossModel::UpdateCondition", BooleanValue(true)); // enable or disable the LOS/NLOS update when the UE moves
    Config::SetDefault ("ns3::AntennaArrayModel::AntennaHorizontalSpacing", DoubleValue(0.5));
    Config::SetDefault ("ns3::AntennaArrayModel::AntennaVerticalSpacing", DoubleValue(0.5));
    Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue(MilliSeconds(100))); // interval after which the channel for a moving user is updated,
                                                                                               // with spatial consistency procedure. If 0, spatial consistency is not used
    Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue(true)); // Set true to use cell scanning method, false to use the default power method.
    Config::SetDefault ("ns3::MmWave3gppChannel::Blockage", BooleanValue(true)); // use blockage or not
    Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue(true)); // use blockage model with UT in portrait mode
    Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(4)); // number of non-self blocking obstacles
    Config::SetDefault ("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue(28e9)); // check MmWavePhyMacCommon for other PHY layer parameters


      //LogComponentEnable ("MmWaveSpectrumPhy", LOG_LEVEL_INFO);
      //LogComponentEnable ("MmWaveUePhy", LOG_LEVEL_INFO);
      LogComponentEnable ("MmWaveBeamManagement", LOG_LEVEL_INFO);
      //LogComponentEnable ("MmWaveEnbPhy", LOG_LEVEL_INFO);
      //LogComponentEnable ("MmWavePhy", LOG_LEVEL_INFO);

      //LogComponentEnable ("MmWaveUeMac", LOG_LEVEL_INFO);
      //LogComponentEnable ("MmWaveEnbMac", LOG_LEVEL_INFO);
      //LogComponentEnable ("MmWaveRrMacScheduler", LOG_LEVEL_INFO);

      //LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
      //LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
      //LogComponentEnable("PropagationLossModel",LOG_LEVEL_ALL);
      //LogComponentEnable("mmWaveInterference",LOG_LEVEL_ALL);
      //LogComponentEnable("MmWaveBeamforming",LOG_LEVEL_ALL);
      //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);


    // set mobile device and base station antenna heights in meters, according to the chosen scenario
  	double hBS; //base station antenna height in meters;
  	double hUT; //user antenna height in meters;
    if(scenario.compare("RMa")==0)
    {
    	hBS = 35;
    	hUT = 1.5;
    }
    else if(scenario.compare("UMa")==0)
    {
    	hBS = 25;
    	hUT = 1.5;
    }
    else if (scenario.compare("UMi-StreetCanyon")==0)
    {
    	hBS = 10;
    	hUT = 1.5;
    }
    else if (scenario.compare("InH-OfficeMixed")==0 || scenario.compare("InH-OfficeOpen")==0 || scenario.compare("InH-ShoppingMall")==0)
    {
  	//The fast fading model does not support 'InH-ShoppingMall' since it is listed in table 7.5-6
    	hBS = 3;
    	hUT = 1;
    }
    else
    {
  	  std::cout<<"Unknown scenario.\n";
  	  return 1;
    }


  Ptr<MmWaveHelper> ptr_mmWave = CreateObject<MmWaveHelper> ();

  /*
   *  Choose the desired channel model
   */

  // Beamforming channel
//  ptr_mmWave->SetAttribute ("PathlossModel", StringValue ("ns3::BuildingsObstaclePropagationLossModel"));

  // 3GPP channel
  ptr_mmWave->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));

  // Determine propagation loss model (with or without buildings)
  if(enableBuildings)
  {
	  ptr_mmWave->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppBuildingsPropagationLossModel"));
  }
  else
  {
	  ptr_mmWave->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));
  }


  // Carlos modification
  ptr_mmWave->SetEnbPhyArchitecture(Analog);
  ptr_mmWave->SetUePhyArchitecture(Analog);
  // End of Carlos modification

  /* A configuration example.
   * ptr_mmWave->GetPhyMacConfigurable ()->SetAttribute("SymbolPerSlot", UintegerValue(30)); */

  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (1);
  ueNodes.Create (1);

//  Ptr < Building > building;
//  building = Create<Building> ();
//  building->SetBoundaries (Box (20.0, 30.0,
//                                10.0, 20.0,
//                                0.0, 10.0));
//  building->SetBuildingType (Building::Residential);
//  building->SetExtWallsType (Building::ConcreteWithWindows);
//  building->SetNFloors (1);
//  building->SetNRoomsX (1);
//  building->SetNRoomsY (1);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0.0, 0.0, hBS));

  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);

  MobilityHelper uemobility;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  uePositionAlloc->Add (Vector (0.0, -20.0, hUT));


//  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install (ueNodes);
//  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (60, -20, hUT));
  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (10, 10, 0));

  BuildingsHelper::Install (ueNodes);

  NetDeviceContainer enbNetDev = ptr_mmWave->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueNetDev = ptr_mmWave->InstallUeDevice (ueNodes);

  // Attach list of NetDevices to each phy layer to enable scanning
  ptr_mmWave->AddMmWaveNetDevicesToPhy();

  // Initialize channels between eNBs and UEs
  ptr_mmWave->InitiateChannel(ueNetDev, enbNetDev);
//  ptr_mmWave->AttachToClosestEnb (ueNetDev, enbNetDev);	// If I disable this, MIBs are not received

  ptr_mmWave->EnableTraces();

//  // Activate a data radio bearer
//	enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
//	EpsBearer bearer (q);
//	ptr_mmWave->ActivateDataRadioBearer (ueNetDev, bearer);
  BuildingsHelper::MakeMobilityModelConsistent ();

	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (1024 * 1024));
	double stopTime = 9.9;
	double simStopTime = 20.00;
	Ipv4Address remoteHostAddr;

	Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	ptr_mmWave->SetEpcHelper (epcHelper);
	ptr_mmWave->Initialize();

	Ptr<Node> pgw = epcHelper->GetPgwNode ();

	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);
	Ptr<Node> remoteHost = remoteHostContainer.Get (0);
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);

	// Create the Internet
	PointToPointHelper p2ph;
	p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
	p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
	p2ph.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (1)));
	NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
	// interface 0 is localhost, 1 is the p2p device
	remoteHostAddr = internetIpIfaces.GetAddress (1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	// Install the IP stack on the UEs
	// Assign IP address to UEs, and install applications
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueNetDev));

	// Set the default gateway for the UE
	Ptr<Node> ueNode = ueNodes.Get (0);
	Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

	// Install and start applications on UEs and remote host
	uint16_t sinkPort = 20000;

	Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (0), sinkPort));
	PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
	ApplicationContainer sinkApps = packetSinkHelper.Install (ueNodes.Get (0));
	sinkApps.Start (Seconds (0.));
	sinkApps.Stop (Seconds (simStopTime));

	Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHostContainer.Get (0), TcpSocketFactory::GetTypeId ());

	Ptr<MyApp> app = CreateObject<MyApp> ();
	app->Setup (ns3TcpSocket, sinkAddress, 1000, 500, DataRate ("30Mb/s"));
	remoteHostContainer.Get (0)->AddApplication (app);

	AsciiTraceHelper asciiTraceHelper;
	Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-window.txt");
	ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, stream1));

	Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream ("mmWave-tcp-data.txt");
	sinkApps.Get(0)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream2));


	app->SetStartTime (Seconds (0.1));
	app->SetStopTime (Seconds (stopTime));


	p2ph.EnablePcapAll("mmwave-sgi-capture");

	Simulator::Stop (Seconds (simStopTime));
	Simulator::Run ();

	Simulator::Destroy ();

	return 0;
}
