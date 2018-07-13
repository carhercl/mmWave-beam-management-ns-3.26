
#include "ns3/point-to-point-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/packet.h>
#include <ns3/tag.h>
/*#include <ns3/lte-helper.h>
#include <ns3/epc-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/lte-module.h>*/

//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * A script to simulate the DOWNLINK TCP data over mmWave links
 * with the mmWave devices and the LTE EPC.
 */
NS_LOG_COMPONENT_DEFINE ("mmWaveRlcSaturation");


int
main (int argc, char *argv[])
{

  // LogComponentEnable("TcpCongestionOps", LOG_LEVEL_INFO);
  // LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    LogComponentEnable ("MmWaveBeamManagement", LOG_LEVEL_INFO);
    //LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);


	uint16_t nodeNum = 1;
	double simStopTime = 73;
	bool harqEnabled = true;
	bool rlcAmEnabled = true;
	std::string protocol = "TcpCubic";
	//int bufferSize = 1000 *1000 * 3.5 * 0.4;
	//uint32_t bufferSize = 80000;
	uint32_t bufferSize = 5000000;
	uint32_t packetSize = 14000;
	uint32_t p2pDelay = 9;
	// This 3GPP channel model example only demonstrate the pathloss model. The fast fading model is still in developing.

	//The available channel scenarios are 'RMa', 'UMa', 'UMi-StreetCanyon', 'InH-OfficeMixed', 'InH-OfficeOpen', 'InH-ShoppingMall'
	std::string scenario = "UMa";
	std::string condition = "a";


	/* Read arguments from shell script */
	enum Architecture arch = Analog;
	std::string sArch = "Analog";
	MmWavePhyMacCommon::SsBurstPeriods ssburstset = MmWavePhyMacCommon::ms20;
	std::string stringSsBurstSet = "ms20";
	float speed = 1.0;
	unsigned los_profile = 0;	//select the LOS situation: 0=only LOS, 1=LOS-NLOS-LOS
	unsigned seed = 0;
	float distance = 20;

	CommandLine cmd;
//	cmd.AddValue("numEnb", "Number of eNBs", numEnb);
//	cmd.AddValue("numUe", "Number of UEs per eNB", numUe);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simStopTime);
//	cmd.AddValue("interPacketInterval", "Inter-packet interval [us])", interPacketInterval);
	cmd.AddValue("harq", "Enable Hybrid ARQ", harqEnabled);
	cmd.AddValue("rlcAm", "Enable RLC-AM", rlcAmEnabled);
	cmd.AddValue("protocol", "TCP protocol", protocol);
	cmd.AddValue("bufferSize", "buffer size", bufferSize);
	cmd.AddValue("packetSize", "packet size", packetSize);
	cmd.AddValue("p2pDelay","delay between server and PGW", p2pDelay);
	cmd.AddValue("arch","Beamforming architecture",sArch);
	cmd.AddValue("ssburstset", "SS burst set period", stringSsBurstSet);
	cmd.AddValue("speed","UE speed",speed);
	cmd.AddValue("obstacle","LOS condition profile",los_profile);
	cmd.AddValue("seed", "Simulation run seed", seed);
	cmd.AddValue("distance", "gNB-UE distance (radius in circular walk)", distance);

	cmd.Parse(argc, argv);

	//Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (65535));
	Config::SetDefault ("ns3::TcpSocketBase::MinRto", TimeValue (MilliSeconds (200)));
	Config::SetDefault ("ns3::Ipv4L3Protocol::FragmentExpirationTimeout", TimeValue (Seconds (1)));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
	Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));

	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (131072*400));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (131072*400));


	Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
	Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(true));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(true));
	//Config::SetDefault ("ns3::LteRlcAm::PollRetransmitTimer", TimeValue(MilliSeconds(8.0)));
	//Config::SetDefault ("ns3::LteRlcAm::ReorderingTimer", TimeValue(MilliSeconds(4.0)));
	//Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(4.0)));
	//Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MilliSeconds(8.0)));
	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (bufferSize));
    //Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (100*1000));

	//Config::SetDefault ("ns3::CoDelQueueDisc::Mode", StringValue ("QUEUE_DISC_MODE_PACKETS"));
    //Config::SetDefault ("ns3::CoDelQueueDisc::MaxPackets", UintegerValue (50000));
    //Config::SetDefault ("ns3::CoDelQueue::Interval", StringValue ("500ms"));
    //Config::SetDefault ("ns3::CoDelQueue::Target", StringValue ("50ms"));

    //Config::SetDefault ("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::FIXED_TTT));

//Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCubic::GetTypeId ()));

    if(protocol == "TcpNewReno")
    {
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
    }
    else if (protocol == "TcpVegas")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }
    else if (protocol == "TcpHighSpeed")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHighSpeed::GetTypeId ()));

    }
    else if (protocol == "TcpCubic")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpCubic::GetTypeId ()));

    }
    else if (protocol == "TcpIllinois")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpIllinois::GetTypeId ()));

    }
    else if (protocol == "TcpHybla")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHybla::GetTypeId ()));

    }
    else if (protocol == "TcpVeno")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVeno::GetTypeId ()));

    }
    else if (protocol == "TcpWestwood")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));

    }
    else if (protocol == "TcpYeah")
    {
    	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpYeah::GetTypeId ()));

    }
    else
    {
		std::cout<<protocol<<" Unknown protocol.\n";
		return 1;
    }



    // Beamforming architecture
	if(sArch == "Analog")
	{
		arch = Analog;
//		std::cout << "Analog architecture" << std::endl;
	}
	else if (sArch == "Analog_fast")
	{
		arch = Analog_fast;
//		std::cout << "Analog architecture with faster beam updates" << std::endl;
	}
	else if (sArch == "Digital")
	{
		arch = Digital;
//		std::cout << "Digital architecture" << std::endl;
	}
	else
	{
		NS_LOG_ERROR("Unsupported or unrecognized beamforming architecture. Choose Analog or Digital");
	}


	// SS Burst Set period
	if(stringSsBurstSet == "ms5")
	{
		ssburstset = MmWavePhyMacCommon::ms5;
	}
	else if (stringSsBurstSet == "ms10")
	{
		ssburstset = MmWavePhyMacCommon::ms10;
	}
	else if (stringSsBurstSet == "ms20")
	{
		ssburstset = MmWavePhyMacCommon::ms20;
	}
	else if (stringSsBurstSet == "ms40")
	{
		ssburstset = MmWavePhyMacCommon::ms40;
	}
	else if (stringSsBurstSet == "ms80")
	{
		ssburstset = MmWavePhyMacCommon::ms80;
	}
	else if (stringSsBurstSet == "ms160")
	{
		ssburstset = MmWavePhyMacCommon::ms160;
	}
	else
	{
		NS_LOG_ERROR("Unsupported or unrecognized SS periodicity. Choose among ms5, ms10, ms20, ms40, ms80 or ms160");
	}

	/*
	 *  The more statistically rigorous way to configure multiple independent
	 *  replications is to use a fixed seed and to advance the run number.
	 */
	RngSeedManager::SetSeed (1);
	RngSeedManager::SetRun(seed);


	Config::SetDefault ("ns3::TcpVegas::Alpha", UintegerValue (20));
	Config::SetDefault ("ns3::TcpVegas::Beta", UintegerValue (40));
	Config::SetDefault ("ns3::TcpVegas::Gamma", UintegerValue (2));


	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::ChannelCondition", StringValue(condition));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue(scenario));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::OptionalNlos", BooleanValue(false));
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Shadowing", BooleanValue(false)); // enable or disable the shadowing effect
	Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::InCar", BooleanValue(false)); // enable or disable the shadowing effect



	Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue (MilliSeconds (100))); // Set channel update period, 0 stands for no update.
	Config::SetDefault ("ns3::MmWave3gppChannel::CellScan", BooleanValue(false)); // Set true to use cell scanning method, false to use the default power method.
	Config::SetDefault ("ns3::MmWave3gppChannel::Blockage", BooleanValue(false)); // use blockage or not
	Config::SetDefault ("ns3::MmWave3gppChannel::PortraitMode", BooleanValue(true)); // use blockage model with UT in portrait mode
	Config::SetDefault ("ns3::MmWave3gppChannel::NumNonselfBlocking", IntegerValue(4)); // number of non-self blocking obstacles
	Config::SetDefault ("ns3::MmWave3gppChannel::BlockerSpeed", DoubleValue(1)); // speed of non-self blocking obstacles

	Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue(100));


	double hBS = 0; //base station antenna height in meters;
	double hUT = 0; //user antenna height in meters;
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
		hBS = 3;
		hUT = 1;
	}
	else
	{
		std::cout<<"Unknown scenario.\n";
		return 1;
	}


	Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();

	mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppBuildingsPropagationLossModel"));
	mmwaveHelper->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));

	// Set the transceiver architectures
  mmwaveHelper->SetEnbPhyArchitecture(arch);
  mmwaveHelper->SetUePhyArchitecture(arch);

  // Set the SS burst set pattern. SsBurstPeriod must be smaller than SetSsBurstSetPeriod
  mmwaveHelper->SetSsBurstSetPeriod(ssburstset);

	mmwaveHelper->Initialize();
	mmwaveHelper->SetHarqEnabled(true);

	Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	mmwaveHelper->SetEpcHelper (epcHelper);

	Ptr<Node> pgw = epcHelper->GetPgwNode ();


	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (nodeNum);
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ipv4InterfaceContainer internetIpIfaces;

	for (uint16_t i = 0; i < nodeNum; i++)
	{
		Ptr<Node> remoteHost = remoteHostContainer.Get (i);
		// Create the Internet
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
		p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
		p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (p2pDelay)));

		NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

		Ipv4AddressHelper ipv4h;
	    std::ostringstream subnet;
	    subnet<<i<<".1.0.0";
		ipv4h.SetBase (subnet.str ().c_str (), "255.255.0.0");
		internetIpIfaces = ipv4h.Assign (internetDevices);
		// interface 0 is localhost, 1 is the p2p device
		Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
		remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), 1);

	}

	/*Ptr < Building > building1;
	building1 = Create<Building> ();
	building1->SetBoundaries (Box (1100,1140.0,
								12, 20.0,
								0.0, 40));


	Ptr < Building > building2;
	building2 = Create<Building> ();
	building2->SetBoundaries (Box (620,700.0,
								0, 5.0,
								0.0, 40));

	Ptr < Building > building3;
	building3 = Create<Building> ();
	building3->SetBoundaries (Box (565,575.0,
								1.0, 5.0,
								0.0, 40));

	Ptr < Building > building4;
	building4 = Create<Building> ();
	building4->SetBoundaries (Box (1220,1260.0,
								11.0, 11.5,
								0.0, 40));

	Ptr < Building > building5;
	building5 = Create<Building> ();
	building5->SetBoundaries (Box (1330,1360.0,
								11.0, 11.5,
								0.0, 40));*/

	// Be careful not to locate the UE inside the building
	if (los_profile > 0)
	{

		if(speed < 5 )
		{

			Ptr < Building > building1;
			building1 = Create<Building> ();
			building1->SetBoundaries (Box (-78,-77.0,
										-49.8, -49.9,
										0.0, 40));

			Ptr < Building > building2;
			building2 = Create<Building> ();
			building2->SetBoundaries (Box (-76.5,-76.0,
										-49.8, -49.9,
										0.0, 40));

			Ptr < Building > building3;
			building3 = Create<Building> ();
			building3->SetBoundaries (Box (-74,-73.5,
										-49.8, -49.9,
										0.0, 40));

			Ptr < Building > building4;
			building4 = Create<Building> ();
			building4->SetBoundaries (Box (-72.5,-71.5,
										-49.8, -49.9,
										0.0, 40));

		}
		else
		{

			Ptr < Building > building1;
			building1 = Create<Building> ();
			building1->SetBoundaries (Box (-50,-35.0,
										-49, -49.5,
										0.0, 40));

			Ptr < Building > building2;
			building2 = Create<Building> ();
			building2->SetBoundaries (Box (-27,-20.0,
										-49, -49.5,
										0.0, 40));

			Ptr < Building > building3;
			building3 = Create<Building> ();
			building3->SetBoundaries (Box (10,17.0,
										-49, -49.5,
										0.0, 40));

			Ptr < Building > building4;
			building4 = Create<Building> ();
			building4->SetBoundaries (Box (32,47.0,
										-49, -49.5,
										0.0, 40));
		}

	}



	  NodeContainer ueNodes;
	  NodeContainer mmWaveEnbNodes;
	  mmWaveEnbNodes.Create(1);
	  ueNodes.Create(1);



	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add (Vector (0.0, 0.0, hBS));

	MobilityHelper enbmobility;
	enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbmobility.SetPositionAllocator(enbPositionAlloc);
	enbmobility.Install (mmWaveEnbNodes);
	BuildingsHelper::Install (mmWaveEnbNodes);

	MobilityHelper uemobility;
//  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
//  uePositionAlloc->Add (Vector (-80.0, -50.0, hUT));


//  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//  uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  uemobility.SetMobilityModel ("ns3::CircularWay");
  Config::SetDefault ("ns3::CircularWay::Radius",DoubleValue(distance));
  Config::SetDefault ("ns3::CircularWay::Speed",DoubleValue (speed));
  double theta = M_PI/2 - (speed/distance)/2;
  Config::SetDefault ("ns3::CircularWay::Theta",DoubleValue (theta));
//  uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install (ueNodes);
  ueNodes.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (distance, 0, hUT));
//  ueNodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (speed, 0, 0));

  BuildingsHelper::Install (ueNodes);
	// Install LTE Devices to the nodes

	  NetDeviceContainer mmWaveEnbDevs = mmwaveHelper->InstallEnbDevice (mmWaveEnbNodes);
	  NetDeviceContainer mcUeDevs;

	    mcUeDevs = mmwaveHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	// Assign IP address to UEs, and install applications
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mcUeDevs));

  // Attach list of NetDevices to each phy layer to enable scanning
  mmwaveHelper->AddMmWaveNetDevicesToPhy();

  // Initialize channels between eNBs and UEs
  //mmwaveHelper->InitiateChannel(mcUeDevs, mmWaveEnbDevs);
  mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs);  	//mmwaveHelper->EnableTraces ();

  mmwaveHelper->EnableTraces();

//  	uint16_t nFlows = 4;
//	ApplicationContainer sourceApps [nFlows];
//	ApplicationContainer sinkApps [nFlows];
//	uint16_t sinkPort = 20000;
//
//
//
//
//
//	for (uint16_t i = 0; i < ueNodes.GetN (); i++)
//	{
//		// Set the default gateway for the UE
//		Ptr<Node> ueNode = ueNodes.Get (i);
//		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
//		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
//
//		// Install and start applications on UEs and remote host
////		PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
////		sinkApps.Add (packetSinkHelper.Install (ueNodes.Get (i)));
//		for(unsigned int f = 0; f < nFlows; f++){
//			PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
//			sinkApps[f].Add (packetSinkHelper.Install (ueNodes.Get (i)));
//			sinkApps[f].Start(Seconds(0.));
//			sinkApps[f].Stop(Seconds(simStopTime));
//
//
//			BulkSendHelper ftp ("ns3::TcpSocketFactory",
//									 InetSocketAddress (ueIpIface.GetAddress (i), sinkPort));
//			sourceApps[f].Add (ftp.Install (remoteHostContainer.Get (i)));
//
//			std::ostringstream fileName;
//			fileName<<protocol+"-"+std::to_string(bufferSize)+"-"+std::to_string(packetSize)+"-"+std::to_string(p2pDelay)<<"-"<<i+1<<"-"<<f+1<<"-TCP-DATA.txt";
//
//			AsciiTraceHelper asciiTraceHelper;
//
//			Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
////			sinkApps.Get(i)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream));
//			sinkApps[f].Get(i)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream));
//
//			sourceApps[f].Get(i)->SetStartTime(Seconds (0.1+0.01*f));
//			Simulator::Schedule (Seconds (0.1001+0.01*f), &Traces, i, protocol+"-"+std::to_string(bufferSize)+"-"+std::to_string(packetSize)+"-"+std::to_string(p2pDelay)+"-"+std::to_string(f+1));
//			//sourceApps.Get(i)->SetStopTime (Seconds (10-1.5*i));
//			sourceApps[f].Get(i)->SetStopTime (Seconds (simStopTime));
//
//			sinkPort+=2;
//
//		}
//
//	}

	// Activate a data radio bearer
	enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_PREMIUM;
	EpsBearer bearer (q);
	mmwaveHelper->ActivateDataRadioBearer (mcUeDevs, bearer);

	//p2ph.EnablePcapAll("mmwave-sgi-capture");
	BuildingsHelper::MakeMobilityModelConsistent ();

	simStopTime = 2 * M_PI * distance / speed;
	std::cout << "Simulation Time = " << simStopTime << "s" << std::endl;
	Simulator::Stop (Seconds (simStopTime));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;

}
