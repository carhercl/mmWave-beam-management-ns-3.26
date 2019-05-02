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


#ifndef MMWAVE_HELPER_H
#define MMWAVE_HELPER_H

#include <ns3/config.h>
#include <ns3/simulator.h>
#include <ns3/names.h>
#include <ns3/net-device.h>
#include <ns3/net-device-container.h>
#include <ns3/node.h>
#include <ns3/node-container.h>
#include <ns3/mobility-model.h>
#include <ns3/spectrum-phy.h>
#include <ns3/mmwave-ue-net-device.h>
#include <ns3/mmwave-enb-net-device.h>
#include <ns3/mmwave-phy.h>
#include <ns3/mmwave-ue-phy.h>
#include <ns3/mmwave-enb-phy.h>
#include <ns3/mmwave-spectrum-value-helper.h>
#include <ns3/mmwave-beamforming.h>
#include <ns3/mmwave-phy-mac-common.h>
#include <ns3/antenna-array-model.h>
#include <ns3/mmwave-rrc-protocol-ideal.h>
#include "mmwave-phy-rx-trace.h"
#include <ns3/epc-helper.h>
#include <ns3/epc-ue-nas.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/lte-ue-rrc.h>
#include <ns3/boolean.h>
#include <ns3/epc-helper.h>
#include <ns3/lte-ffr-algorithm.h>
#include <ns3/mmwave-beamforming.h>
#include <ns3/mmwave-channel-matrix.h>
#include <ns3/mmwave-bearer-stats-calculator.h>
#include <ns3/mmwave-bearer-stats-connector.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/mmwave-channel-raytracing.h>
#include <ns3/mmwave-3gpp-channel.h>
#include <ns3/mmwave-flex-tti-mac-scheduler.h>
#include <ns3/mmwave-beam-management.h>

namespace ns3 {

/* ... */
class MmWaveUePhy;
class MmWaveEnbPhy;
class SpectrumChannel;
class SpectrumpropagationLossModel;
class MmWaveSpectrumValueHelper;
class PropagationLossModel;


class MmWaveHelper : public Object
{
public:
	MmWaveHelper(void);
	virtual ~MmWaveHelper(void);

	static TypeId GetTypeId (void);
	virtual void DoDispose (void);

	NetDeviceContainer InstallUeDevice (NodeContainer c);
	NetDeviceContainer InstallEnbDevice (NodeContainer c);
	void SetAntenna (uint16_t Nrx, uint16_t Ntx);
	void SetPathlossModelType (std::string type);
	void SetChannelModelType (std::string type);

	/*
	 * \brief This function is a copy of AttachToClosestEnb without connecting eNBs and UEs for cell association process
	 */
	void InitiateChannel (NetDeviceContainer ueDevices, NetDeviceContainer enbDevices);

	void AttachToClosestEnb (NetDeviceContainer ueDevices, NetDeviceContainer enbDevices);
	void EnableTraces ();

	void SetSchedulerType (std::string type);
	std::string GetSchedulerType () const;

	void SetPhyMacConfigurationParameters (std::string paramName, std::string value);
    Ptr<MmWavePhyMacCommon> GetPhyMacConfigurable (void);
	void ActivateDataRadioBearer (NetDeviceContainer ueDevices, EpsBearer bearer);
	void ActivateDataRadioBearer (Ptr<NetDevice> ueDevice, EpsBearer bearer);
	void SetEpcHelper (Ptr<EpcHelper> epcHelper);

	void SetHarqEnabled (bool harqEnabled);
	bool GetHarqEnabled ();
	void SetSnrTest (bool snrTest);
	bool GetSnrTest ();
	Ptr<PropagationLossModel> GetPathLossModel (void);

	// Carlos modification
	NetDeviceContainer GetMmwaveEnbNetDeviceDeployedList ();
	NetDeviceContainer GetMmwaveUeNetDeviceDeployedList ();
//	void GetMmwaveEnbNetDeviceDeployedList (Ptr<MmWaveEnbNetDevice>);
//	void GetMmwaveUeNetDeviceDeployedList (Ptr<MmWaveUeNetDevice>);
	void AddMmWaveNetDevicesToPhy ();

	void SetEnbPhyArchitecture (Architecture arch);
	void SetUePhyArchitecture (Architecture arch);

	void SetCodebooksPaths (std::string gnbPath, std::string uePath);
//	void SetEnbCodebookPath (std::string path);
//	void SetUeCodebookPath (std::string path);

	void SetSsBurstPeriod (MmWavePhyMacCommon::SsBurstPeriods period);
	void SetSsBurstSetPeriod (MmWavePhyMacCommon::SsBurstPeriods period);

	void SetBeamReportingPeriod (MmWavePhyMacCommon::CsiReportingPeriod period);
	MmWavePhyMacCommon::CsiReportingPeriod GetBeamReportingPeriod ();

	void SetCsiResourceAllocationPeriod (MmWavePhyMacCommon::CsiReportingPeriod period);
	MmWavePhyMacCommon::CsiReportingPeriod GetCsiResourceAllocationPeriod ();

	void SetPeriodicCsiReportingConditionFlag (bool state);

	void SetRaytracingFilePath(std::string file_path);
	void SetStartingTraceIndex (uint16_t index);

	void SetTxPower(double powerDb);

	void SetCandidateListForTrackingStrategy(uint16_t alt, uint16_t alpha, uint16_t beta, bool memory);

	void SetFingerprintingMap(Ptr<FingerprintingDatabase> fp);

	void SetFingerprintingFilePath(std::string path);
	// End of modification

protected:
	virtual void DoInitialize();

private:
	Ptr<NetDevice> InstallSingleUeDevice (Ptr<Node> n);
	Ptr<NetDevice> InstallSingleEnbDevice (Ptr<Node> n);
	void AttachToClosestEnb (Ptr<NetDevice> ueDevice, NetDeviceContainer enbDevices);
	void EnableDlPhyTrace ();
	void EnableUlPhyTrace ();
	void EnableEnbPacketCountTrace ();
	void EnableUePacketCountTrace ();
	void EnableTransportBlockTrace ();
	void EnableRlcTraces (void);
	Ptr<MmWaveBearerStatsCalculator> GetRlcStats (void);
	void EnablePdcpTraces (void);
	Ptr<MmWaveBearerStatsCalculator> GetPdcpStats (void);

	Ptr<SpectrumChannel> m_channel;
	Ptr<MmWaveBeamforming> m_beamforming;
	Ptr<MmWaveChannelMatrix> m_channelMatrix;
	Ptr<MmWaveChannelRaytracing> m_raytracing;
	Ptr<MmWave3gppChannel> m_3gppChannel;

	Ptr<Object> m_pathlossModel;
	std::string m_pathlossModelType;

	std::string m_channelModelType;

	ObjectFactory m_enbNetDeviceFactory;
	ObjectFactory m_ueNetDeviceFactory;
	ObjectFactory m_channelFactory;
	ObjectFactory m_pathlossModelFactory;
	ObjectFactory m_schedulerFactory;
	ObjectFactory m_phyMacCommonFactory;

	uint64_t m_imsiCounter;
	uint16_t m_cellIdCounter;

	uint16_t m_noTxAntenna;
	uint16_t m_noRxAntenna;

	std::string m_txCodebookPath;	//Path to the tx codebook file
	std::string m_rxCodebookPath;	//Path to the rx codebook file

	Ptr<MmWavePhyRxTrace> m_phyStats;

	Ptr<MmWavePhyMacCommon> m_phyMacCommon;
	Architecture m_uePhyArch;
	Architecture m_enbPhyArch;
	MmWavePhyMacCommon::SsBurstPeriods m_ssBurstSetPeriod;
	bool m_csiReportingEnabled;

	ObjectFactory m_ffrAlgorithmFactory;
	ObjectFactory m_enbAntennaModelFactory;
	ObjectFactory m_ueAntennaModelFactory;

	Ptr<EpcHelper> m_epcHelper;

	bool m_harqEnabled;
	bool m_rlcAmEnabled;
	bool m_snrTest;
	Ptr<MmWaveBearerStatsCalculator> m_rlcStats;
	Ptr<MmWaveBearerStatsCalculator> m_pdcpStats;
	MmWaveBearerStatsConnector m_radioBearerStatsConnector;

  	// Carlos modification
  	NetDeviceContainer m_enbMmwaveNetDevices;	// All the instantiated enbNetDevices
  	NetDeviceContainer m_ueMmwaveNetDevices; // All the instantiated ueNetDevices

  	MmWavePhyMacCommon::CsiReportingPeriod m_csiResourcePeriodicity;
  	MmWavePhyMacCommon::CsiReportingPeriod m_beamReportingPeriodicity;

  	// Parameters to configure the list of candidate beams to be used for beam tracking.
  	uint8_t m_trackingListStrategy;
  	bool m_memory;	// If all SSB beam measurement combinations must be stored
  	uint16_t m_alpha;	// Only used in Alt.3 and Alt.5 in MmWaveBeamManagement
  	uint16_t m_beta;	// Only used in Alt.4 and Alt.5 in MmWaveBeamManagement

  	std::string m_raytracingFilePath;	//Path to raytracing file (only for raytracing channel)
  	uint16_t m_startTraceIndex;			// Starting trace index

  	Ptr<FingerprintingDatabase> m_fingerprinting;

  	double m_txPower;	// TX power
};

}

#endif /* MMWAVE_HELPER_H */

