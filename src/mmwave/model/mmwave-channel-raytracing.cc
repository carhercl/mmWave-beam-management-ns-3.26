 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
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
 *  
 *   Author: Marco Mezzavilla < mezzavilla@nyu.edu>
 *        	 Sourjya Dutta <sdutta@nyu.edu>
 *        	 Russell Ford <russell.ford@nyu.edu>
 *        	 Menglei Zhang <menglei@nyu.edu>
 *
 *   Modified by: Carlos Herranz <carhercl@iteam.upv.es>
 */


#include "mmwave-channel-raytracing.h"
#include <ns3/log.h>
#include <ns3/math.h>
#include <ns3/simulator.h>
#include <ns3/mmwave-phy.h>
#include <ns3/mmwave-net-device.h>
#include <ns3/node.h>
#include <ns3/mmwave-ue-net-device.h>
#include <ns3/mmwave-enb-net-device.h>
#include <ns3/antenna-array-model.h>
#include <ns3/mmwave-ue-phy.h>
#include <ns3/mmwave-enb-phy.h>
#include <ns3/double.h>
#include <algorithm>
#include <fstream>
#include "mmwave-spectrum-value-helper.h"


namespace ns3{

NS_LOG_COMPONENT_DEFINE ("MmWaveChannelRaytracing");

NS_OBJECT_ENSURE_REGISTERED (MmWaveChannelRaytracing);


doubleVector_t g_path; //number of multipath
double2DVector_t g_delay; //delay spread in ns
double2DVector_t g_pathloss;	//pathloss in DB
double2DVector_t g_phase;	//???what is this
double2DVector_t g_aodElevation;	//degree
double2DVector_t g_aodAzimuth;	//degree
double2DVector_t g_aoaElevation;	//degree
double2DVector_t g_aoaAzimuth;	//degree
uint16_t g_numPdps;	// number of PDPs in the trace file


MmWaveChannelRaytracing::MmWaveChannelRaytracing ()
	:m_antennaSeparation(0.5)
{
	m_uniformRv = CreateObject<UniformRandomVariable> ();
	m_transitory_time_seconds = 3.0;
	/*
	 * Select one method to load traces. LoadTracesMod plugs Unity traces in.
	 */
//	LoadTraces();
//	LoadTracesMod();

}

TypeId
MmWaveChannelRaytracing::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MmWaveChannelRaytracing")
	.SetParent<Object> ()
	.AddAttribute ("StartDistance",
			   "Select start point of the simulation, the range of data point is [0, 260] meters",
			   UintegerValue (0),
			   MakeUintegerAccessor (&MmWaveChannelRaytracing::m_startDistance),
			   MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("Speed",
			   "The speed of the node (m/s)",
			   DoubleValue (1.0),
			   MakeDoubleAccessor (&MmWaveChannelRaytracing::m_speed),
			   MakeDoubleChecker<double> ())
    .AddAttribute("TraceFileName",
    			"The name of the trace file",
				StringValue(""),
				MakeStringAccessor(&MmWaveChannelRaytracing::SetTraceFilePath),
				MakeStringChecker ())
	;
	return tid;
}

MmWaveChannelRaytracing::~MmWaveChannelRaytracing ()
{

}

void
MmWaveChannelRaytracing::DoDispose ()
{
	NS_LOG_FUNCTION (this);
}

void
MmWaveChannelRaytracing::SetConfigurationParameters (Ptr<MmWavePhyMacCommon> ptrConfig)
{
	m_phyMacConfig = ptrConfig;
}

Ptr<MmWavePhyMacCommon>
MmWaveChannelRaytracing::GetConfigurationParameters (void) const
{
	return m_phyMacConfig;
}

void
MmWaveChannelRaytracing::SetTraceFilePath (std::string file_path)
{
	m_traceFileName = file_path;
}

void
MmWaveChannelRaytracing::SetTraceIndex (uint16_t index)
{
	m_traceIndex = index;
}

void
MmWaveChannelRaytracing::LoadTraces()
{
	//std::string filename = "src/mmwave/model/Raytracing/Quadriga.txt";
	std::string filename = "src/mmwave/model/Raytracing/traces.txt";
//	std::string filename = "src/mmwave/model/Raytracing/tracesUnity2_delay.txt";
	NS_LOG_FUNCTION (this << "Loading Raytracing file " << filename);
	std::ifstream singlefile;
	singlefile.open (filename.c_str (), std::ifstream::in);

	NS_LOG_INFO (this << " File: " << filename);
	NS_ASSERT_MSG(singlefile.good (), " Raytracing file not found");
    std::string line;
    std::string token;
    uint16_t counter = 0;
    while( std::getline(singlefile, line) ) //Parse each line of the file
    {
    	if(counter == 8)
    	{
    		counter = 0;
    	}
    	doubleVector_t path;
        std::istringstream stream(line);
        while( getline(stream,token,',') ) //Parse each comma separated string in a line
        {
            double sigma = 0.00;
            std::stringstream stream( token );
            stream>>sigma;
        	path.push_back(sigma);
		}

		switch (counter)
		{
		case 0:
			g_path.push_back (path.at (0));
			break;
		case 1:
			g_delay.push_back (path);
			break;
		case 2:
			g_pathloss.push_back (path);
			break;
		case 3:
			g_phase.push_back (path);
			break;
		case 4:
			g_aodElevation.push_back (path);
			break;
		case 5:
			g_aodAzimuth.push_back (path);
			break;
		case 6:
			g_aoaElevation.push_back (path);
			break;
		case 7:
			g_aoaAzimuth.push_back (path);
			break;
		default:
			NS_FATAL_ERROR("Never call this");
			break;
		}
		counter++;
	}

	/*NS_LOG_UNCOND(g_path.size());
	NS_LOG_UNCOND(g_delay.size());
	NS_LOG_UNCOND(g_pathloss.size());
	NS_LOG_UNCOND(g_phase.size());
	NS_LOG_UNCOND(g_aodElevation.size());
	NS_LOG_UNCOND(g_aodAzimuth.size());
	NS_LOG_UNCOND(g_aoaElevation.size());
	NS_LOG_UNCOND(g_aoaAzimuth.size());*/

    g_numPdps = g_path.size();

}


void
MmWaveChannelRaytracing::LoadTracesMod()
{
	//std::string filename = "src/mmwave/model/Raytracing/Quadriga.txt";
//	std::string filename = "src/mmwave/model/Raytracing/traces.txt";
	std::string filename = m_traceFileName;
	if (filename.empty())
	{
		std::cout << "Trace file (" << filename << ") not provided" << std::endl;
		filename = "src/mmwave/model/Raytracing/NS3_corner1.txt";
	}
	NS_LOG_FUNCTION (this << "Loading Raytracing file " << filename);
	std::ifstream singlefile;
	singlefile.open (filename.c_str (), std::ifstream::in);

	NS_LOG_INFO (this << " File: " << filename);
	NS_ASSERT_MSG(singlefile.good (), " Raytracing file not found");
    std::string line;
    std::string token;
    uint16_t counter = 0;
    double2DVector_t delay_temp;
    while( std::getline(singlefile, line) ) //Parse each line of the file
    {
    	if(counter == 8)
    	{
    		counter = 0;
    	}
    	doubleVector_t path;
        std::istringstream stream(line);
        while( getline(stream,token,',') ) //Parse each comma separated string in a line
        {
            double sigma = 0.00;
            std::stringstream stream( token );
            stream>>sigma;
        	path.push_back(sigma);
		}

		switch (counter)
		{
		case 0:
			g_path.push_back (path.at (0));
			break;
		case 1:
			delay_temp.push_back (path);
			// Sort vector from smallest to greatest delay and keep indices for following reordering.
			g_delay.push_back (path);
			break;
		case 2:
			g_pathloss.push_back (path);
			break;
		case 3:
			g_phase.push_back (path);
			break;
		case 4:
			g_aodElevation.push_back (path);
			break;
		case 5:
			g_aodAzimuth.push_back (path);
			break;
		case 6:
			g_aoaElevation.push_back (path);
			break;
		case 7:
			g_aoaAzimuth.push_back (path);
			break;
		default:
			NS_FATAL_ERROR("Never call this");
			break;
		}
		counter++;
	}

	/*NS_LOG_UNCOND(g_path.size());
	NS_LOG_UNCOND(g_delay.size());
	NS_LOG_UNCOND(g_pathloss.size());
	NS_LOG_UNCOND(g_phase.size());
	NS_LOG_UNCOND(g_aodElevation.size());
	NS_LOG_UNCOND(g_aodAzimuth.size());
	NS_LOG_UNCOND(g_aoaElevation.size());
	NS_LOG_UNCOND(g_aoaAzimuth.size());*/

    g_numPdps = g_path.size();

}


void
MmWaveChannelRaytracing::ConnectDevices (Ptr<NetDevice> dev1, Ptr<NetDevice> dev2)
{
	key_t key = std::make_pair(dev1,dev2);

	std::map< key_t, int >::iterator iter = m_connectedPair.find(key);
	if (iter != m_connectedPair.end ())
	{
		m_connectedPair.erase (iter);
	}
	m_connectedPair.insert(std::make_pair(key,1));
}

void
MmWaveChannelRaytracing::Initial(NetDeviceContainer ueDevices, NetDeviceContainer enbDevices)
{

	NS_LOG_INFO (&ueDevices<<&enbDevices);

	for (NetDeviceContainer::Iterator i = ueDevices.Begin(); i != ueDevices.End(); i++)
	{
		Ptr<MmWaveUeNetDevice> UeDev =
						DynamicCast<MmWaveUeNetDevice> (*i);
		if (UeDev->GetTargetEnb ())
		{
			Ptr<NetDevice> targetBs = UeDev->GetTargetEnb ();
			ConnectDevices (*i, targetBs);
			ConnectDevices (targetBs, *i);

		}
	}

}

void
MmWaveChannelRaytracing::InitialModified(NetDeviceContainer ueDevices, NetDeviceContainer enbDevices)
{

	NS_LOG_INFO (&ueDevices<<&enbDevices);

	for (NetDeviceContainer::Iterator i = ueDevices.Begin(); i != ueDevices.End(); i++)
	{
		Ptr<MmWaveUeNetDevice> UeDev =
						DynamicCast<MmWaveUeNetDevice> (*i);
		if (UeDev->GetTargetEnb ())
		{
			Ptr<NetDevice> targetBs = UeDev->GetTargetEnb ();
			ConnectDevices (*i, targetBs);
			ConnectDevices (targetBs, *i);

		}
	}

}


SpectrumValue
MmWaveChannelRaytracing::GetSinrForBeamPairs (
			Ptr<NetDevice> ueDevice,
			Ptr<NetDevice> enbDevice,
			complexVector_t txBeamforming,
			complexVector_t rxBeamforming)
{


//	CalLongTerm(Params);
//	Params->m_txW = txBeamforming;
//	Params->m_rxW = rxBeamforming;
//
	// Now lets use the UE Phy to get the experienced SINR for this channel with the new combination of beams
	Ptr<MmWaveUeNetDevice> UeDev =
			DynamicCast<MmWaveUeNetDevice> (ueDevice);
	Ptr<MmWaveUePhy> uePhy = UeDev->GetPhy();
	Ptr<SpectrumValue> dummyPsd = uePhy->CreateTxPowerSpectralDensity();
	SpectrumValue experiencedSinr = CalSnr(dummyPsd,enbDevice,ueDevice,txBeamforming,rxBeamforming);

	return experiencedSinr;

}

SpectrumValue
MmWaveChannelRaytracing::CalSnr (
		Ptr<SpectrumValue>  txPsd,
		Ptr<NetDevice> enbNetDevice,
		Ptr<NetDevice> ueNetDevice,
		complexVector_t txBeamforming,
		complexVector_t rxBeamforming)
{
	NS_LOG_FUNCTION (this);
	Ptr<SpectrumValue> rxPsd = Copy (txPsd);
	Ptr<AntennaArrayModel> txAntennaArray, rxAntennaArray;

	// Get mobility model
	Ptr<MobilityModel> a = enbNetDevice->GetNode()->GetObject<MobilityModel> ();
	Ptr<MobilityModel> b = ueNetDevice->GetNode()->GetObject<MobilityModel> ();

	Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
	Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);
	Ptr<MmWaveEnbNetDevice> txEnb =
					DynamicCast<MmWaveEnbNetDevice> (txDevice);
	Ptr<MmWaveUeNetDevice> rxUe =
					DynamicCast<MmWaveUeNetDevice> (rxDevice);

	uint8_t txAntennaNum[2];
	uint8_t rxAntennaNum[2];

	bool dl = true;

	if(txEnb!=0 && rxUe!=0)
	{
		NS_LOG_INFO ("this is downlink case");

		txAntennaNum[0] = 16;
		txAntennaNum[1] = 4;
		rxAntennaNum[0] = 8;
		rxAntennaNum[1] = 2;

		txAntennaArray = DynamicCast<AntennaArrayModel> (
					txEnb->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
		rxAntennaArray = DynamicCast<AntennaArrayModel> (
					rxUe->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
	}
	else if (txEnb==0 && rxUe==0 )
	{
		NS_LOG_INFO ("this is uplink case");

		dl = false;
		Ptr<MmWaveUeNetDevice> txUe =
						DynamicCast<MmWaveUeNetDevice> (txDevice);
		Ptr<MmWaveEnbNetDevice> rxEnb =
						DynamicCast<MmWaveEnbNetDevice> (rxDevice);

		txAntennaNum[0] = 8;
		txAntennaNum[1] = 2;
		rxAntennaNum[0] = 16;
		rxAntennaNum[1] = 4;


		txAntennaArray = DynamicCast<AntennaArrayModel> (
					txUe->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
		rxAntennaArray = DynamicCast<AntennaArrayModel> (
					rxEnb->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
	}
	else
	{
		std::cout << "CalSnr: enb to enb or ue to ue transmission, skip beamforming" << std::endl;
	}

	Ptr<mmWaveBeamFormingTraces> bfParams = Create<mmWaveBeamFormingTraces> ();
	key_t key = std::make_pair(txDevice,rxDevice);

	double time = Simulator::Now().GetSeconds();

	/* This updates the trace.txt entry in use */
	/*
	uint16_t traceIndex = (m_startDistance+time*m_speed)*100;
	static uint16_t currentIndex = m_startDistance*100;
	if(traceIndex > 26050)
	{
		NS_FATAL_ERROR ("The maximum trace index is 26050");
	}
	*/
	/*
	uint16_t traceIndex = (m_startDistance+time*m_speed)*6;
	static uint16_t currentIndex = m_startDistance;
	if(traceIndex > g_path.size())
	{
		NS_FATAL_ERROR ("The maximum trace index is reached");
	}
	*/

	/* Unity traces: measurement resolution is 0.1 m (10 measurement points per meter)*/
	double corrected_time = time - m_transitory_time_seconds;
	if (corrected_time < 0.0)
	{
		corrected_time = 0.0;
	}
	uint16_t update_factor = 10;
	uint16_t traceIndex = (m_startDistance+(corrected_time*m_speed*update_factor));
	if (m_fingerprinting->IsFpDatabaseInitialized() == true)
	{
		m_fingerprinting->SetCurrentPathIndex(traceIndex);
	}
	static uint16_t currentIndex = m_startDistance;
	if(traceIndex > g_numPdps)
	{
		NS_FATAL_ERROR ("The maximum trace index is " << g_numPdps);
	}

	if(time > m_transitory_time_seconds && traceIndex != currentIndex)
	{
		currentIndex = traceIndex;
		m_channelMatrixMap.clear();
	}
	//NS_LOG_UNCOND (traceIndex);

	std::map< key_t, Ptr<TraceParams> >::iterator it = m_channelMatrixMap.find (key);
	if (it == m_channelMatrixMap.end ())
	{

		complex2DVector_t txSpatialMatrix;
		complex2DVector_t rxSpatialMatrix;
		if(dl)
		{
			txSpatialMatrix = GenSpatialMatrix (traceIndex,txAntennaNum, true);
			rxSpatialMatrix = GenSpatialMatrix (traceIndex,rxAntennaNum, false);
		}
		else
		{
			txSpatialMatrix = GenSpatialMatrix (traceIndex,txAntennaNum, false);
			rxSpatialMatrix = GenSpatialMatrix (traceIndex,rxAntennaNum, true);
		}
		doubleVector_t dopplerShift;
		for (unsigned int i = 0; i < g_path.at(traceIndex); i++)
		{
			dopplerShift.push_back(m_uniformRv->GetValue (0,1));
		}

		Ptr<TraceParams> channel = Create<TraceParams> ();

		channel->m_txSpatialMatrix = txSpatialMatrix;
		channel->m_rxSpatialMatrix = rxSpatialMatrix;
		channel->m_powerFraction = g_pathloss.at (traceIndex);
		channel->m_delaySpread = g_delay.at (traceIndex);
		channel->m_doppler = dopplerShift;


		m_channelMatrixMap.insert(std::make_pair(key,channel));

		key_t reverseKey = std::make_pair(rxDevice,txDevice);
		Ptr<TraceParams> reverseChannel = Create<TraceParams> ();
		reverseChannel->m_txSpatialMatrix = rxSpatialMatrix;
		reverseChannel->m_rxSpatialMatrix = txSpatialMatrix;
		reverseChannel->m_powerFraction = g_pathloss.at (traceIndex);;
		reverseChannel->m_delaySpread = g_delay.at (traceIndex);
		reverseChannel->m_doppler = dopplerShift;

		m_channelMatrixMap.insert(std::make_pair(reverseKey,reverseChannel));

		bfParams->m_channelParams = channel;
	}
	else
	{
		bfParams->m_channelParams = (*it).second;
	}

	//	calculate antenna weights, better method should be implemented

	std::map< key_t, int >::iterator it1 = m_connectedPair.find (key);
	if(it1 != m_connectedPair.end ())
	{
		if (dl)
		{
			bfParams->m_txW = txBeamforming;
			bfParams->m_rxW = rxBeamforming;
		}
		else
		{
			bfParams->m_txW = rxBeamforming;
			bfParams->m_rxW = txBeamforming;
		}
//		bfParams->m_txW = CalcBeamformingVector(bfParams->m_channelParams->m_txSpatialMatrix, bfParams->m_channelParams->m_powerFraction);
//		bfParams->m_rxW = CalcBeamformingVector(bfParams->m_channelParams->m_rxSpatialMatrix, bfParams->m_channelParams->m_powerFraction);
//		txAntennaArray->SetBeamformingVector(bfParams->m_txW,rxDevice);
//		rxAntennaArray->SetBeamformingVector(bfParams->m_rxW,txDevice);
	}
	else
	{
		bfParams->m_txW = txAntennaArray->GetBeamformingVector();
		bfParams->m_rxW = rxAntennaArray->GetBeamformingVector();
	}

	/*Vector rxSpeed = b->GetVelocity();
	Vector txSpeed = a->GetVelocity();
	double relativeSpeed = (rxSpeed.x-txSpeed.x)
			+(rxSpeed.y-txSpeed.y)+(rxSpeed.z-txSpeed.z);*/
	Ptr<SpectrumValue> bfPsd = GetChannelGain(rxPsd,bfParams, m_speed);


	/*loging the beamforming gain for debug*/
	/*Values::iterator bfit = bfPsd->ValuesBegin ();
	Values::iterator rxit = rxPsd->ValuesBegin ();
	uint32_t subChannel = 0;
	double value=0;
	while (bfit != bfPsd->ValuesEnd ())
	{
		value += (*bfit)/(*rxit);
		bfit++;
		rxit++;
		subChannel++;
	}
	NS_LOG_UNCOND("Gain("<<10*log10(value/72)<<"dB)");*/

	//NS_LOG_UNCOND ("TxAngle("<<txAngles.phi*180/M_PI<<") RxAngle("<<rxAngles.phi*180/M_PI
	//		<<") Speed["<<relativeSpeed<<"]");
	//NS_LOG_UNCOND ("Gain("<<10*Log10((*bfPsd))<<"dB)");


	double noiseFigure = 10.0;
	Ptr<SpectrumValue> noisePsd =
				MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_phyMacConfig, noiseFigure);

	SpectrumValue Sinr = (*bfPsd)/(*noisePsd);

//	double pathLossDb = g_pathloss.at (traceIndex);
//	Sinr * std::pow (10.0, (pathLossDb) / 10.0);

	return Sinr;
}


Ptr<SpectrumValue>
MmWaveChannelRaytracing::DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
                                                   Ptr<const MobilityModel> a,
                                                   Ptr<const MobilityModel> b) const
{
	NS_LOG_FUNCTION (this);
	Ptr<SpectrumValue> rxPsd = Copy (txPsd);
	Ptr<AntennaArrayModel> txAntennaArray, rxAntennaArray;

	Ptr<NetDevice> txDevice = a->GetObject<Node> ()->GetDevice (0);
	Ptr<NetDevice> rxDevice = b->GetObject<Node> ()->GetDevice (0);
	Ptr<MmWaveEnbNetDevice> txEnb =
					DynamicCast<MmWaveEnbNetDevice> (txDevice);
	Ptr<MmWaveUeNetDevice> rxUe =
					DynamicCast<MmWaveUeNetDevice> (rxDevice);

	uint8_t txAntennaNum[2];
	uint8_t rxAntennaNum[2];

	bool dl = true;

	if(txEnb!=0 && rxUe!=0)
	{
		NS_LOG_INFO ("this is downlink case");

//		txAntennaNum[0] = sqrt (txEnb->GetAntennaNum ());
//		txAntennaNum[1] = sqrt (txEnb->GetAntennaNum ());
//		rxAntennaNum[0] = sqrt (rxUe->GetAntennaNum ());
//		rxAntennaNum[1] = sqrt (rxUe->GetAntennaNum ());
		txAntennaNum[0] = 16;
		txAntennaNum[1] = 4;
		rxAntennaNum[0] = 8;
		rxAntennaNum[1] = 2;

		txAntennaArray = DynamicCast<AntennaArrayModel> (
					txEnb->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
		rxAntennaArray = DynamicCast<AntennaArrayModel> (
					rxUe->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
	}
	else if (txEnb==0 && rxUe==0 )
	{
		NS_LOG_INFO ("this is uplink case");

		dl = false;
		Ptr<MmWaveUeNetDevice> txUe =
						DynamicCast<MmWaveUeNetDevice> (txDevice);
		Ptr<MmWaveEnbNetDevice> rxEnb =
						DynamicCast<MmWaveEnbNetDevice> (rxDevice);

//		txAntennaNum[0] = sqrt (txUe->GetAntennaNum ());
//		txAntennaNum[1] = sqrt (txUe->GetAntennaNum ());
//		rxAntennaNum[0] = sqrt (rxEnb->GetAntennaNum ());
//		rxAntennaNum[1] = sqrt (rxEnb->GetAntennaNum ());
		txAntennaNum[0] = 8;
		txAntennaNum[1] = 2;
		rxAntennaNum[0] = 16;
		rxAntennaNum[1] = 4;

		txAntennaArray = DynamicCast<AntennaArrayModel> (
					txUe->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
		rxAntennaArray = DynamicCast<AntennaArrayModel> (
					rxEnb->GetPhy ()->GetDlSpectrumPhy ()->GetRxAntenna ());
	}
	else
	{
		NS_LOG_INFO ("enb to enb or ue to ue transmission, skip beamforming");
		std::cout << "enb to enb or ue to ue transmission, skip beamforming" << std::endl;
		return rxPsd;
	}

	Ptr<mmWaveBeamFormingTraces> bfParams = Create<mmWaveBeamFormingTraces> ();
	key_t key = std::make_pair(txDevice,rxDevice);

	double time = Simulator::Now().GetSeconds();

	/* This updates the trace.txt entry in use */
	/*
	uint16_t traceIndex = (m_startDistance+time*m_speed)*100;
	static uint16_t currentIndex = m_startDistance*100;
	if(traceIndex > 26050)
	{
		NS_FATAL_ERROR ("The maximum trace index is 26050");
	}
	*/
	/*
	uint16_t traceIndex = (m_startDistance+time*m_speed)*6;
	static uint16_t currentIndex = m_startDistance;
	if(traceIndex > g_path.size())
	{
		NS_FATAL_ERROR ("The maximum trace index is reached");
	}
	*/

	/* Unity traces: measurement resolution is 0.1 m (10 measurement points per meter)*/
	double corrected_time = time - m_transitory_time_seconds;
	if (corrected_time < 0.0)
	{
		corrected_time = 0.0;
	}
	uint16_t update_factor = 10;
	uint16_t traceIndex = (m_startDistance+(corrected_time*m_speed*update_factor));
	if (m_fingerprinting->IsFpDatabaseInitialized() == true)
	{
		m_fingerprinting->SetCurrentPathIndex(traceIndex);
	}

	static uint16_t currentIndex = m_startDistance;
	if(traceIndex > g_numPdps)
	{
		NS_FATAL_ERROR ("The maximum trace index is " << g_numPdps);
	}
	if(time > m_transitory_time_seconds && traceIndex != currentIndex)
	{
		currentIndex = traceIndex;
		m_channelMatrixMap.clear();
	}
//	NS_LOG_UNCOND (traceIndex);

	std::map< key_t, Ptr<TraceParams> >::iterator it = m_channelMatrixMap.find (key);
	if (it == m_channelMatrixMap.end ())
	{
		complex2DVector_t txSpatialMatrix;
		complex2DVector_t rxSpatialMatrix;
		if(dl)
		{
			txSpatialMatrix = GenSpatialMatrix (traceIndex,txAntennaNum, true);
			rxSpatialMatrix = GenSpatialMatrix (traceIndex,rxAntennaNum, false);
		}
		else
		{
			txSpatialMatrix = GenSpatialMatrix (traceIndex,txAntennaNum, false);
			rxSpatialMatrix = GenSpatialMatrix (traceIndex,rxAntennaNum, true);
		}
		doubleVector_t dopplerShift;
		for (unsigned int i = 0; i < g_path.at(traceIndex); i++)
		{
			dopplerShift.push_back(m_uniformRv->GetValue (0,1));
		}

		Ptr<TraceParams> channel = Create<TraceParams> ();

		channel->m_txSpatialMatrix = txSpatialMatrix;
		channel->m_rxSpatialMatrix = rxSpatialMatrix;
		channel->m_powerFraction = g_pathloss.at (traceIndex);
		channel->m_delaySpread = g_delay.at (traceIndex);
		channel->m_doppler = dopplerShift;


		m_channelMatrixMap.insert(std::make_pair(key,channel));

		key_t reverseKey = std::make_pair(rxDevice,txDevice);
		Ptr<TraceParams> reverseChannel = Create<TraceParams> ();
		reverseChannel->m_txSpatialMatrix = rxSpatialMatrix;
		reverseChannel->m_rxSpatialMatrix = txSpatialMatrix;
		reverseChannel->m_powerFraction = g_pathloss.at (traceIndex);
		reverseChannel->m_delaySpread = g_delay.at (traceIndex);
		reverseChannel->m_doppler = dopplerShift;

		m_channelMatrixMap.insert(std::make_pair(reverseKey,reverseChannel));

		bfParams->m_channelParams = channel;
	}
	else
	{
		bfParams->m_channelParams = (*it).second;
	}

	//	calculate antenna weights, better method should be implemented

	std::map< key_t, int >::iterator it1 = m_connectedPair.find (key);
	if(it1 != m_connectedPair.end ())
	{
		bfParams->m_txW = txAntennaArray->GetBeamformingVector(rxDevice);
		bfParams->m_rxW = rxAntennaArray->GetBeamformingVector(txDevice);
		// The first time (after initialization)this vectors should be empty
		if (bfParams->m_txW.size() == 0 || bfParams->m_rxW.size() == 0)
		{
			Ptr<MmWaveEnbPhy> pEnbPhy = txEnb->GetPhy();
			Ptr<MmWaveUePhy> pUePhy = rxUe->GetPhy();

			Ptr<MmWaveBeamManagement> pMngEnb = pEnbPhy->GetBeamManagement();
			Ptr<MmWaveBeamManagement> pMngUe = pUePhy->GetBeamManagement();

			bfParams->m_txW = pMngEnb->GetBeamSweepVector(0);
			bfParams->m_rxW = pMngUe->GetBeamSweepVector(0);

//			bfParams->m_txW = CalcBeamformingVector(bfParams->m_channelParams->m_txSpatialMatrix, bfParams->m_channelParams->m_powerFraction);
//			bfParams->m_rxW = CalcBeamformingVector(bfParams->m_channelParams->m_rxSpatialMatrix, bfParams->m_channelParams->m_powerFraction);
			txAntennaArray->SetBeamformingVector(bfParams->m_txW,rxDevice);
			rxAntennaArray->SetBeamformingVector(bfParams->m_rxW,txDevice);
			if (Simulator::Now() > 0)
			{
				std::cout << "Empty bf vectors in the antennas at " << Simulator::Now().GetNanoSeconds() << " ns" << std::endl;
			}
		}
	}
	else
	{
		bfParams->m_txW = txAntennaArray->GetBeamformingVector();
		bfParams->m_rxW = rxAntennaArray->GetBeamformingVector();
	}

	/*Vector rxSpeed = b->GetVelocity();
	Vector txSpeed = a->GetVelocity();
	double relativeSpeed = (rxSpeed.x-txSpeed.x)
			+(rxSpeed.y-txSpeed.y)+(rxSpeed.z-txSpeed.z);*/
	Ptr<SpectrumValue> bfPsd = GetChannelGain(rxPsd,bfParams, m_speed);

	// Uncomment to log gain values
	/*
	SpectrumValue bfGain = (*bfPsd)/(*rxPsd);
	uint8_t nbands = bfGain.GetSpectrumModel ()->GetNumBands ();
	if (dl == true)
	{
		NS_LOG_DEBUG ("****** DL BF gain == " << Sum (bfGain)/nbands << " RX PSD " << Sum(*rxPsd)/nbands); // print avg bf gain
	}
	else
	{
		NS_LOG_DEBUG ("****** UL BF gain == " << Sum (bfGain)/nbands << " RX PSD " << Sum(*rxPsd)/nbands);
	}
	*/


	/*loging the beamforming gain for debug*/
	/*Values::iterator bfit = bfPsd->ValuesBegin ();
	Values::iterator rxit = rxPsd->ValuesBegin ();
	uint32_t subChannel = 0;
	double value=0;
	while (bfit != bfPsd->ValuesEnd ())
	{
		value += (*bfit)/(*rxit);
		bfit++;
		rxit++;
		subChannel++;
	}
	NS_LOG_UNCOND("Gain("<<10*log10(value/72)<<"dB)");*/

	//NS_LOG_UNCOND ("TxAngle("<<txAngles.phi*180/M_PI<<") RxAngle("<<rxAngles.phi*180/M_PI
	//		<<") Speed["<<relativeSpeed<<"]");
	//NS_LOG_UNCOND ("Gain("<<10*Log10((*bfPsd))<<"dB)");
	return bfPsd;


}


complexVector_t
MmWaveChannelRaytracing::CalcBeamformingVector(complex2DVector_t spatialMatrix, doubleVector_t powerFraction) const
{
	complexVector_t antennaWeights;
	uint16_t antennaNum = spatialMatrix.at (0).size ();
	for (int i = 0; i< antennaNum; i++)
	{
		antennaWeights.push_back(spatialMatrix.at (0).at (i)/sqrt(antennaNum));
	}

	for(int iter = 0; iter<10; iter++)
	{
		complexVector_t antennaWeights_New;

		for(unsigned pathIndex = 0; pathIndex<spatialMatrix.size (); pathIndex++)
		{
			std::complex<double> sum;
			for (int i = 0; i< antennaNum; i++)
			{
				sum += std::conj(spatialMatrix.at (pathIndex).at (i))*antennaWeights.at (i);
			}

			for (int i = 0; i< antennaNum; i++)
			{
                double pathPowerLinear = std::pow (10.0, (powerFraction. at(pathIndex)) / 10.0);


				if(pathIndex ==0)
				{
					antennaWeights_New.push_back(pathPowerLinear*spatialMatrix.at (pathIndex).at (i)*sum);
				}
				else
				{
					antennaWeights_New.at (i) += pathPowerLinear*spatialMatrix.at (pathIndex).at (i)*sum;
				}
			}
		}
		//normalize antennaWeights;
		double weightSum = 0;
		for (int i = 0; i< antennaNum; i++)
		{
			weightSum += pow (std::abs(antennaWeights_New. at(i)),2);
		}
		for (int i = 0; i< antennaNum; i++)
		{
			antennaWeights_New. at(i) = antennaWeights_New. at(i)/sqrt(weightSum);
		}
		antennaWeights = antennaWeights_New;
	}
	return antennaWeights;
}



complex2DVector_t
MmWaveChannelRaytracing::GenSpatialMatrix (uint64_t traceIndex, uint8_t* antennaNum, bool bs) const
{
	complex2DVector_t spatialMatrix;
	uint16_t pathNum = g_path.at (traceIndex);
	for(unsigned int pathIndex = 0; pathIndex < pathNum; pathIndex++)
	{
		double azimuthAngle;
		double verticalAngle;
		if(bs)
		{
			azimuthAngle = g_aodAzimuth.at (traceIndex).at (pathIndex);
			verticalAngle = g_aodElevation.at (traceIndex).at (pathIndex);
		}
		else
		{
			azimuthAngle = g_aoaAzimuth.at (traceIndex).at (pathIndex);
			verticalAngle = g_aoaElevation.at (traceIndex).at (pathIndex);
		}
		complexVector_t singlePath;
		singlePath = GenSinglePathKron (azimuthAngle*M_PI/180, verticalAngle*M_PI/180, antennaNum);
		spatialMatrix.push_back(singlePath);
	}

	return spatialMatrix;
}


complexVector_t
MmWaveChannelRaytracing::GenSinglePath (double hAngle, double vAngle, uint8_t* antennaNum) const
{
	NS_LOG_FUNCTION (this);
	complexVector_t singlePath;
	uint16_t hSize = antennaNum[0];
	uint16_t vSize = antennaNum[1];

	for (int vIndex = 0; vIndex < vSize; vIndex++)
	{
		for (int hIndex =0; hIndex < hSize; hIndex++)
		{
			double w = (-2)*M_PI*hIndex*m_antennaSeparation*cos(-hAngle)*cos(vAngle)
										+ (-2)*M_PI*vIndex*m_antennaSeparation*cos(-hAngle)*sin(vAngle);
			singlePath.push_back (std::complex<double> (cos (w), sin (w)));
		}
	}
	return singlePath;
}

complexVector_t
MmWaveChannelRaytracing::GenSinglePathKron (double hAngle, double vAngle, uint8_t* antennaNum) const
{
	NS_LOG_FUNCTION (this);
	complexVector_t singlePath, steeringVector_h, steeringVector_v;
	uint16_t hSize = antennaNum[0];
	uint16_t vSize = antennaNum[1];
	// reserving space for std::vector might speed up vector memory allocation and allocate the necessary space
	steeringVector_h.reserve(hSize);
	steeringVector_v.reserve(vSize);

	// The steering vector of the horizontal plane antenna elements
	steeringVector_h.push_back(std::complex<double> (1.0f, 0.0f));
	for (int hIndex = 1; hIndex < hSize; hIndex++)
	{
		//double complex_phase = 2*M_PI*m_antennaSeparation*sin(-hAngle)*cos(vAngle);
		double complex_phase = 2*M_PI*m_antennaSeparation*hIndex*cos(hAngle)*sin(vAngle);
		steeringVector_h.push_back(std::complex<double> (cos(complex_phase), sin(complex_phase)));
	}

	// The steering vector of the vertical plane antenna elements
	steeringVector_v.push_back(std::complex<double> (1.0f, 0.0f));
	for (int vIndex = 1; vIndex < vSize; vIndex++)
	{
		//double complex_phase = 2*M_PI*m_antennaSeparation*sin(-hAngle)*sin(vAngle);
		double complex_phase = 2*M_PI*m_antennaSeparation*vIndex*cos(vAngle);
		steeringVector_v.push_back(std::complex<double> (cos(complex_phase), sin(complex_phase)));
	}

	// The steering vector of the rectangular array is the kronecker product of the previous two steering vectors
	// Note that the kronecker product of two vectors is another vector length equals to the product of the two vectors length
	singlePath = KroneckerProductVector(steeringVector_v, steeringVector_h);
	return singlePath;
}

/*
 * @brief: Function that returns the Kronecker (direct) product of two complex vectors representing the steering vectors of the horizontal and vertical planes of a rectangular antenna array. Please respect order
 * @param a_h: Steering vector of the horizontal linear array
 * @param a_h: Steering vector of the vertical linear array
 * @note: Careful with the product terms order. Consecutive codewords are group for the same elevation angle, so first term should be the vertical vector.
 */
complexVector_t
MmWaveChannelRaytracing::KroneckerProductVector(complexVector_t a_v, complexVector_t a_h) const
{
	unsigned int size_h, size_v, size_kron_vector;
	complexVector_t kron_vector;

	size_h = a_h.size();
	size_v = a_v.size();
	size_kron_vector = size_h*size_v;
	kron_vector.reserve(size_kron_vector);

	for (unsigned int v = 0; v < a_v.size(); v++)
	{
		for (unsigned int h = 0; h < a_h.size(); h++)
		{
			std::complex<double> value = a_h.at(h) * a_v.at(v);
			kron_vector.push_back(value);
//			std::cout << value << std::endl;
		}
	}

	return kron_vector;
}

Ptr<SpectrumValue>
MmWaveChannelRaytracing::GetChannelGain (Ptr<const SpectrumValue> txPsd, Ptr<mmWaveBeamFormingTraces> bfParams, double speed) const
{
	NS_LOG_FUNCTION (this);

	NS_ASSERT(bfParams->m_channelParams->m_rxSpatialMatrix.size () == bfParams->m_channelParams->m_txSpatialMatrix.size ());
	uint16_t pathNum = bfParams->m_channelParams->m_txSpatialMatrix.size ();
	Time time = Simulator::Now ();
	double t = time.GetSeconds ();
	Ptr<SpectrumValue> tempPsd = Copy<SpectrumValue> (txPsd);
	bool noSpeed = false;
	if (speed == 0)
	{
		noSpeed = true;
	}

	Values::iterator vit = tempPsd->ValuesBegin ();
	uint16_t iSubband = 0;
	while (vit != tempPsd->ValuesEnd ())
	{
		std::complex<double> subsbandGain (0.0,0.0);
		if ((*vit) != 0.00)
		{
			double fsb = m_phyMacConfig->GetCentreFrequency () - GetSystemBandwidth ()/2 + m_phyMacConfig->GetChunkWidth ()*iSubband ;
			for (unsigned int pathIndex = 0; pathIndex < pathNum; pathIndex++)
			{
				//need to convert ns to s
				double temp_delay = -1e-9*2*M_PI*fsb*bfParams->m_channelParams->m_delaySpread.at (pathIndex);
				std::complex<double> delay (cos (temp_delay), sin (temp_delay));

				std::complex<double> doppler;
				if (noSpeed)
				{
					doppler = std::complex<double> (1,0);
				}
				else
				{
					double f_d = speed*m_phyMacConfig->GetCentreFrequency ()/3e8;
					double temp_Doppler = 2*M_PI*t*f_d*bfParams->m_channelParams->m_doppler.at (pathIndex);
					doppler = std::complex<double> (cos (temp_Doppler), sin (temp_Doppler));
				}
				double pathPowerLinear = std::pow (10.0, (bfParams->m_channelParams->m_powerFraction. at(pathIndex)) / 10.0);

				std::complex<double> smallScaleFading = sqrt(pathPowerLinear)*doppler*delay;
				NS_LOG_INFO(doppler<<delay);

				if(bfParams->m_txW.empty ()||bfParams->m_rxW.empty ())
				{
					NS_FATAL_ERROR("antenna weights are empty");
				}
				else
				{
					/* beam forming*/
					std::complex<double> txSum, rxSum;
					for (unsigned i = 0; i < bfParams->m_txW.size (); i++)
					{
						txSum += std::conj(bfParams->m_channelParams->m_txSpatialMatrix.at (pathIndex).at (i))*bfParams->m_txW.at (i);
					}
					for (unsigned i = 0; i < bfParams->m_rxW.size (); i++)
					{
						rxSum += bfParams->m_channelParams->m_rxSpatialMatrix.at (pathIndex). at (i)*std::conj(bfParams->m_rxW.at (i));
					}
					subsbandGain = subsbandGain + txSum*rxSum*smallScaleFading;
				}
			}
			*vit = (*vit)*(norm (subsbandGain));
		}
		vit++;
		iSubband++;
	}
	return tempPsd;
}

double
MmWaveChannelRaytracing::GetSystemBandwidth () const
{
	double bw = 0.00;
	bw = m_phyMacConfig->GetChunkWidth () * m_phyMacConfig->GetNumChunkPerRb () * m_phyMacConfig->GetNumRb ();
	return bw;
}


void
MmWaveChannelRaytracing::UpdateBfChannelMatrix(Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice, BeamPairInfoStruct bestBeams)
{
	key_t key = std::make_pair(enbDevice,ueDevice);
	key_t keyReverse = std::make_pair(ueDevice,enbDevice);
	std::map< key_t, Ptr<TraceParams> >::iterator it = m_channelMatrixMap.find(key);
	std::map< key_t, Ptr<TraceParams> >::iterator itReverse = m_channelMatrixMap.find(keyReverse);
	Ptr<TraceParams> channelParams;
	if (it != m_channelMatrixMap.end ())
	{
		channelParams = it->second;
	}
	else if (itReverse != m_channelMatrixMap.end ())
	{
		channelParams = itReverse->second;
	}
	else
	{
		NS_ASSERT_MSG (it != m_channelMatrixMap.end (), "could not find");
		std::cout << "ERROR at " << Simulator::Now() << ": No channelParams in m_channelMap (UpdateBfChannelMatrix)" << std::endl;
	}

	Ptr<MmWaveEnbNetDevice> pMmwaveEnb = DynamicCast<MmWaveEnbNetDevice>(enbDevice);
	Ptr<MmWaveUeNetDevice> pMmwaveUe = DynamicCast<MmWaveUeNetDevice>(ueDevice);

	Ptr<MmWaveEnbPhy> pEnbPhy = pMmwaveEnb->GetPhy();
	Ptr<MmWaveUePhy> pUePhy = pMmwaveUe->GetPhy();

	Ptr<MmWaveBeamManagement> pMngEnb = pEnbPhy->GetBeamManagement();
	Ptr<MmWaveBeamManagement> pMngUe = pUePhy->GetBeamManagement();

	Ptr<mmWaveBeamFormingTraces> bfParams = Create<mmWaveBeamFormingTraces> ();
	bfParams->m_txW = pMngEnb->GetBeamSweepVector(bestBeams.m_txBeamId);
	bfParams->m_rxW = pMngUe->GetBeamSweepVector(bestBeams.m_rxBeamId);
	bfParams->m_channelParams = channelParams;

//	channelParams->m_txW = pMngEnb->GetBeamSweepVector(bestBeams.m_txBeamId);
//	channelParams->m_rxW = pMngUe->GetBeamSweepVector(bestBeams.m_rxBeamId);
//	CalLongTerm (channelParams);

	Ptr<AntennaArrayModel> enbAntennaArray = DynamicCast<AntennaArrayModel> (
			pEnbPhy->GetDlSpectrumPhy ()->GetRxAntenna ());
	Ptr<AntennaArrayModel> ueAntennaArray = DynamicCast<AntennaArrayModel> (
			pUePhy->GetDlSpectrumPhy ()->GetRxAntenna ());

	//Update the bf vectors in the MmWaveSpectrumPhy
	enbAntennaArray->SetBeamformingVector(bfParams->m_txW,ueDevice);
	ueAntennaArray->SetBeamformingVector(bfParams->m_rxW,enbDevice);
//	enbAntennaArray->ChangeBeamformingVector(ueDevice);
//	ueAntennaArray->ChangeBeamformingVector(enbDevice);

}


void
MmWaveChannelRaytracing::SetBeamSweepingVector (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice)
{
	key_t key = std::make_pair(enbDevice,ueDevice);
	key_t keyReverse = std::make_pair(ueDevice,enbDevice);

//	std::map< key_t, Ptr<BeamformingParams> >::iterator it = m_channelScanningMatrixMap.find(key);
	std::map< key_t, Ptr<TraceParams> >::iterator it = m_channelMatrixMap.find(key);
	std::map< key_t, Ptr<TraceParams> >::iterator itReverse = m_channelMatrixMap.find(keyReverse);
	Ptr<TraceParams> Params;
	if (it != m_channelMatrixMap.end ())
	{
		Params = it->second;
	}
	else if (itReverse != m_channelMatrixMap.end ())
	{
		Params = itReverse->second;
	}
	else
	{
		NS_ASSERT_MSG (it != m_channelMatrixMap.end (), "could not find");
	}
	Ptr<MmWaveEnbNetDevice> EnbDev =
			DynamicCast<MmWaveEnbNetDevice> (enbDevice);
	Ptr<MmWaveUeNetDevice> UeDev =
			DynamicCast<MmWaveUeNetDevice> (ueDevice);

	Ptr<MmWaveEnbPhy> enbPhy = EnbDev->GetPhy();
	Ptr<MmWaveUePhy> uePhy = UeDev->GetPhy();

	Ptr<MmWaveBeamManagement> enbBeamMng = enbPhy->GetBeamManagement();
	Ptr<MmWaveBeamManagement> ueBeamMng = uePhy->GetBeamManagement();

	// Update the bf params in the beam sweeping map:
	Ptr<mmWaveBeamFormingTraces> bfParams = Create<mmWaveBeamFormingTraces> ();
	bfParams->m_txW = enbBeamMng->GetBeamSweepVector();
	bfParams->m_rxW = ueBeamMng->GetBeamSweepVector();
	bfParams->m_channelParams = it->second;

	//TODO: Thinking considering multiplying the codebooks by the antenna sector (AoD/AoA)

//	Ptr<MobilityModel> a = enbDevice->GetNode()->GetObject<MobilityModel> ();
//	Ptr<MobilityModel> b = ueDevice->GetNode()->GetObject<MobilityModel> ();
//	UpdateChannelMatrixAntennaSpatialSignatures(a,b,Params);
//	Params->m_beam = GetLongTermFading(Params);
//	CalLongTerm(Params);

	Ptr<MmWaveEnbNetDevice> pMmwaveEnb = DynamicCast<MmWaveEnbNetDevice>(enbDevice);
	Ptr<MmWaveUeNetDevice> pMmwaveUe = DynamicCast<MmWaveUeNetDevice>(ueDevice);

	Ptr<MmWaveEnbPhy> pEnbPhy = pMmwaveEnb->GetPhy();
	Ptr<MmWaveUePhy> pUePhy = pMmwaveUe->GetPhy();

	Ptr<AntennaArrayModel> enbAntennaArray = DynamicCast<AntennaArrayModel> (
			pEnbPhy->GetDlSpectrumPhy ()->GetRxAntenna ());
	Ptr<AntennaArrayModel> ueAntennaArray = DynamicCast<AntennaArrayModel> (
			pUePhy->GetDlSpectrumPhy ()->GetRxAntenna ());

	//Update the bf vectors in the MmWaveSpectrumPhy
	enbAntennaArray->SetBeamformingVector(bfParams->m_txW);
	ueAntennaArray->SetBeamformingVector(bfParams->m_rxW);

	// Now lets use the UE Phy to get the experienced SINR for this channel with the new combination of beams
	//Ptr<const SpectrumModel> pSModel = enbPhy->GetDlSpectrumPhy()->GetRxSpectrumModel();	// the UePhy does not have content in this pointer
	Ptr<SpectrumValue> dummyPsd = uePhy->CreateTxPowerSpectralDensity();
	SpectrumValue experiencedSinr = CalSnr(dummyPsd,enbDevice,ueDevice,bfParams->m_txW,bfParams->m_rxW);

	// Now add the experienced SINR to the beam management node
	uint16_t txBeamId = enbBeamMng->GetCurrentBeamId();
	uint16_t rxBeamId = ueBeamMng->GetCurrentBeamId();
	ueBeamMng->AddEnbSinr(
			enbDevice,
			txBeamId,
			rxBeamId,
			experiencedSinr);

}


void
MmWaveChannelRaytracing::SetFingerprintingInRt (Ptr<FingerprintingDatabase> fp)
{
	m_fingerprinting = fp;
}


void
MmWaveChannelRaytracing::SetTransitoryTime (double t)
{
	m_transitory_time_seconds = t;
}



}// namespace ns3
