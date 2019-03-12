/*
 * mmwave-beam-management.cc
 *
 *  Created on: 12 oct. 2017
 *      Author: Carlos Herranz
 */


#include "mmwave-beam-management.h"

#include <ns3/log.h>
#include <fstream>
#include <ns3/simulator.h>
#include <ns3/abort.h>
#include <ns3/mmwave-enb-net-device.h>
#include <ns3/mmwave-ue-net-device.h>
#include <ns3/mmwave-ue-phy.h>
#include <ns3/antenna-array-model.h>
#include <ns3/node.h>
#include <algorithm>
#include <ns3/double.h>
#include <ns3/boolean.h>
#include "mmwave-spectrum-value-helper.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("MmWaveBeamManagement");

NS_OBJECT_ENSURE_REGISTERED (MmWaveBeamManagement);




BeamPairInfoStruct::BeamPairInfoStruct()
{
	m_avgSinr = -100;
	m_txBeamId = 65535;
	m_rxBeamId = 65535;
	m_sinrPsd = -1.0;
}


MmWaveBeamManagement::MmWaveBeamManagement()
{
	m_beamSweepParams.m_currentBeamId = 0;
	m_ssBlocksLastBeamSweepUpdate = 0;
	m_maxNumBeamPairCandidates = 20;
	m_beamReportingEnabled = false;
	m_txFilePath = "";
	m_rxFilePath = "";
	m_memorySs = true;
	m_beamCandidateListStrategy = 2;
	m_alpha = 2;
	m_beta = 4;
}


MmWaveBeamManagement::~MmWaveBeamManagement()
{
	std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1;
	for (it1 = m_enbSinrMap.begin(); it1 != m_enbSinrMap.end(); ++it1)
	{
		it1->second.clear();
	}
	m_enbSinrMap.clear();
}


TypeId
MmWaveBeamManagement::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MmWaveBeamManagement")
		.SetParent<Object> ()
	;
  	return tid;
}

void
MmWaveBeamManagement::InitializeBeamManagerEnb (Ptr<MmWavePhyMacCommon> phyMacConfig, Ptr<MmWaveEnbPhy> phy)
{
	double beamPeriodicity = phyMacConfig->GetSlotPeriod();
	Time beamPeriodicityTime = NanoSeconds(1000*beamPeriodicity);
	if (m_txFilePath.empty() == true)
	{
		SetTxCodebookFilePath(m_txFilePath);
	}
	InitializeBeamSweepingTx(beamPeriodicityTime);
	MmWavePhyMacCommon::SsBurstPeriods ssBurstSetperiod = phyMacConfig->GetSsBurstSetPeriod();
	ScheduleSsSlotSetStart(ssBurstSetperiod);

	//CSI-RS reporting period
	m_beamReportingPeriod = phyMacConfig->GetCsiReportPeriodicity();
	m_beamReportingEnabled = false;

	// Start SS block transmission and measurement routine
	Time Period = GetNextSsBlockTransmissionTime(phyMacConfig,GetNumBlocksSinceLastBeamSweepUpdate()); //m_currentSsBlockSlotId
	Simulator::Schedule(Period,&MmWaveEnbPhy::StartSsBlockSlot,phy); //	StartSsBlockSlot();
}


void
MmWaveBeamManagement::InitializeBeamManagerUe(Ptr<MmWavePhyMacCommon> phyMacConfig, Ptr<MmWaveUePhy> phy)
{
	double beamPeriodicity = phyMacConfig->GetSlotPeriod();
	Time beamPeriodicityTime = NanoSeconds(1000*beamPeriodicity);
	if (m_rxFilePath.empty() == true)
	{
		SetRxCodebookFilePath(m_rxFilePath);
	}
	InitializeBeamSweepingRx(beamPeriodicityTime);
	MmWavePhyMacCommon::SsBurstPeriods ssBurstSetperiod = phyMacConfig->GetSsBurstSetPeriod();
	ScheduleSsSlotSetStart(ssBurstSetperiod);

	//CSI-RS reporting period
	m_beamReportingPeriod = phyMacConfig->GetCsiReportPeriodicity();
	m_beamReportingEnabled = false;

	// Start SS block transmission and measurement routine
	Time Period = GetNextSsBlockTransmissionTime(phyMacConfig,GetNumBlocksSinceLastBeamSweepUpdate()); //m_currentSsBlockSlotId
	Simulator::Schedule(Period,&MmWaveUePhy::StartSsBlockSlot,phy); //	StartSsBlockSlot();
}


void
MmWaveBeamManagement::InitializeBeamSweepingTx(Time beamChangeTime)
{
	m_beamSweepParams.m_currentBeamId = 0;

//	std::string txFilePath = "src/mmwave/model/BeamFormingMatrix/TxCodebook.txt";
	std::string txFilePath = "src/mmwave/model/BeamFormingMatrix/KronCodebook16h4v.txt";
//	std::string txFilePath = "../../../InputFiles/KronCodebook16h4v.txt";

	if (m_txFilePath == "")
	{
		m_txFilePath = txFilePath;
	}
	m_beamSweepParams.m_codebook = LoadCodebookFile(m_txFilePath);

	this->SetBeamChangeInterval(beamChangeTime);
	m_lastBeamSweepUpdate = Simulator::Now();
	NS_LOG_INFO ("InitializeBeamSweepingTx");
//	DisplayCurrentBeamId();

}

void
MmWaveBeamManagement::InitializeBeamSweepingRx(Time beamChangeTime)
{
	m_beamSweepParams.m_currentBeamId = 0;

//	std::string rxFilePath = "src/mmwave/model/BeamFormingMatrix/RxCodebook.txt";
	std::string rxFilePath = "src/mmwave/model/BeamFormingMatrix/KronCodebook8h2v.txt";
//	std::string rxFilePath = "../../../InputFiles/KronCodebook8h2v.txt";

	if (m_rxFilePath == "")
	{
		m_rxFilePath = rxFilePath;
	}
	m_beamSweepParams.m_codebook = LoadCodebookFile(m_rxFilePath);

	this->SetBeamChangeInterval(beamChangeTime);
	NS_LOG_INFO ("InitializeBeamSweepingRx");
	m_lastBeamSweepUpdate = Simulator::Now();
//	DisplayCurrentBeamId();

}

void MmWaveBeamManagement::SetBeamChangeInterval (Time period)
{
	m_beamSweepParams.m_steerBeamInterval = period;
}

void MmWaveBeamManagement::SetBeamSweepCodebook (complex2DVector_t codebook)
{
	m_beamSweepParams.m_codebook = codebook;
}

complexVector_t MmWaveBeamManagement::GetBeamSweepVector ()
{
	return m_beamSweepParams.m_codebook.at(m_beamSweepParams.m_currentBeamId);

}

complexVector_t MmWaveBeamManagement::GetBeamSweepVector (uint16_t index)
{
	return m_beamSweepParams.m_codebook.at(index);

}

void MmWaveBeamManagement::BeamSweepStep()
{

	Time currentTime = Simulator::Now();
	uint16_t numCodes = m_beamSweepParams.m_codebook.size();
//	NS_LOG_INFO ("[" << currentTime << "] Beam id " << m_beamSweepParams.m_currentBeamId << " of " << numCodes - 1);
	m_beamSweepParams.m_currentBeamId = (m_beamSweepParams.m_currentBeamId + 1) % numCodes;
//	m_lastBeamSweepUpdate = currentTime;
//	NS_LOG_INFO ("[" << currentTime << "] Beam id " << m_beamSweepParams.m_currentBeamId << " of " << numCodes - 1);

}

void MmWaveBeamManagement::BeamSweepStepTx()
{
//	Time currentTime = Simulator::Now();
//	if (currentTime == 0 || currentTime >= m_lastBeamSweepUpdate + m_beamSweepParams.m_steerBeamInterval)
	{
		BeamSweepStep();
	}
//	else
//	{
//		NS_LOG_INFO ("MmWaveBeamManagement::BeamSweepStepTx() This should not happen");
//	}
}

void MmWaveBeamManagement::BeamSweepStepRx()
{
//	Time currentTime = Simulator::Now();
//	if (currentTime >= m_lastBeamSweepUpdate + m_beamSweepParams.m_steerBeamInterval)
	{
		BeamSweepStep();
	}
}


void MmWaveBeamManagement::DisplayCurrentBeamId ()
{
	Time currentTime = Simulator::Now();
	uint16_t numCodes = m_beamSweepParams.m_codebook.size();
	NS_LOG_INFO ("[" << currentTime << "] Beam id " << m_beamSweepParams.m_currentBeamId << " of " << numCodes - 1);
}

uint16_t
MmWaveBeamManagement::GetCurrentBeamId ()
{
	return m_beamSweepParams.m_currentBeamId;
}


uint16_t
MmWaveBeamManagement::GetNumBlocksSinceLastBeamSweepUpdate ()
{
	return m_ssBlocksLastBeamSweepUpdate;
}

uint16_t
MmWaveBeamManagement::IncreaseNumBlocksSinceLastBeamSweepUpdate ()
{
	m_ssBlocksLastBeamSweepUpdate++;
	return m_ssBlocksLastBeamSweepUpdate;

}

void MmWaveBeamManagement::ResetNumBlocksSinceLastBeamSweepUpdate ()
{
	m_ssBlocksLastBeamSweepUpdate = 0;
}

void
MmWaveBeamManagement::AddEnbSinr (Ptr<NetDevice> enbNetDevice, uint16_t enbBeamId, uint16_t ueBeamId, SpectrumValue sinr)
{
	sinrKey key = std::make_pair(enbBeamId,ueBeamId);
	std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.find(enbNetDevice);
	if (it1 != m_enbSinrMap.end ())
	{
		std::map <sinrKey,SpectrumValue>::iterator it2 = it1->second.find(key);
		if (it2 != it1->second.end())
		{
			it2->second = sinr;
		}
		else
		{
			it1->second.insert(make_pair(key,sinr));
		}
//		m_enbSinrMap.erase (it1);
	}
	else
	{
		std::map <sinrKey,SpectrumValue> m2;
		m2.insert(make_pair(key,sinr));
		m_enbSinrMap.insert(make_pair(enbNetDevice,m2));
	}
//	std::cout << Simulator::Now().GetNanoSeconds() << " " << enbBeamId << " " << ueBeamId << " "
//			<< Sum(sinr)/sinr.GetSpectrumModel()->GetNumBands() << std::endl;

}

/*
 * Alt1. Beam tracking strategy 1: List of candidate beams is created out of top N best SINR.
 * N value is class member m_maxNumBeamPairCandidates.
 */
void
MmWaveBeamManagement::FindBeamPairCandidatesSinr ()
{
//	BeamPairInfoStruct bestPairInfo;
//	double bestAvgSinr = -1.0;

	for (std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.begin();
			it1 != m_enbSinrMap.end();
			++it1)
	{
		Ptr<NetDevice> pDevice = it1->first;
		std::vector<BeamPairInfoStruct> candidateBeamPairs;

		// Internal SINR values to control the size of the candidate beam vector.
		double minSinr=0;
		uint8_t numBeamPairs = 0;
		//double maxSinr=0;

		for (std::map <sinrKey,SpectrumValue>::iterator it2 = it1->second.begin();
				it2 != it1->second.end();
				++it2)
		{
			int nbands = it2->second.GetSpectrumModel ()->GetNumBands ();
			double avgSinr = Sum (it2->second)/nbands;

			// Skip the current pair of beams if they provide lower SINR than the current candidate pairs
			if(avgSinr < minSinr)
				continue;

			// The SINR of the current pair of beams is large enough to be included in the list of candidate beam pairs
			// Populate the beam pair info structure
			BeamPairInfoStruct beamPair;
			beamPair.m_avgSinr = avgSinr;
			beamPair.m_sinrPsd = it2->second;
			beamPair.m_targetNetDevice = it1->first;
			beamPair.m_txBeamId = it2->first.first;
			beamPair.m_rxBeamId = it2->first.second;

			// The candidate beam pairs are stored in descending order of SINR.
			// If the list of candidate beam pairs is empty, add the struct to the list directly.
			if(candidateBeamPairs.empty())
			{
				candidateBeamPairs.push_back(beamPair);
				numBeamPairs = 1;
			}
			// In case the list of candidate beam pairs is not empty, add the new node in the right position of the list
			else
			{
				bool iterate = true;
				std::vector<BeamPairInfoStruct>::iterator itV = candidateBeamPairs.begin();
				while (iterate && itV != candidateBeamPairs.end())
				{
					if (itV->m_avgSinr < avgSinr)
					{
						// insertion happens in the position following itV
						candidateBeamPairs.insert(itV,beamPair);
						numBeamPairs++;
						iterate = false;	// Stop iteration
					}
					++itV;	// Increase list iterator
				}
				// Only get best m_numMaxCandidateBeams pair of beams
				if(candidateBeamPairs.size() > m_maxNumBeamPairCandidates)
				{
					// It gets rid of the last element in the vector
					candidateBeamPairs.resize(m_maxNumBeamPairCandidates);
					numBeamPairs = m_maxNumBeamPairCandidates;
				}
			}

			//Update internal sinr values
			minSinr = candidateBeamPairs.at(numBeamPairs-1).m_avgSinr;
		}

		//to map:
		BeamTrackingParams trackingParamStruct;
		trackingParamStruct.m_beamPairList = candidateBeamPairs;
		trackingParamStruct.m_numBeamPairs = candidateBeamPairs.size();
		trackingParamStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
		trackingParamStruct.m_csiResourceLastAllocation = Simulator::Now();

		UpdateBeamTrackingInfo(pDevice,trackingParamStruct);
//		std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator itMap = m_candidateBeamsMap.find(pDevice);
//		if(itMap == m_candidateBeamsMap.end())
//		{
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,trackingParamStruct));
//		}
//		else
//		{
//			m_candidateBeamsMap.erase(itMap);	//Erase the entry so it can be updated
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,trackingParamStruct));
//			//(*itMap).second = trackingParamStruct;
//			//itMap->second = trackingParamStruct;
//		}
	}
}

std::vector<uint16_t>
MmWaveBeamManagement::GetSideImmediateNeighborBeams(uint16_t beam_id, uint16_t num_beams_h, uint16_t num_beams_v)
{

	uint16_t txBeamId = beam_id;
	uint16_t txBeamIdRow = floor(txBeamId / num_beams_h);
	uint16_t txBeamIdColumn = txBeamId % num_beams_h;

	std::vector<uint16_t> beam_id_vector;
	beam_id_vector.clear();

	uint16_t tempTxBeamId;
	// Same tx beam
	tempTxBeamId = txBeamId;
	beam_id_vector.push_back(tempTxBeamId);
	// Right beam
	tempTxBeamId = txBeamIdRow*num_beams_h + (txBeamIdColumn + 1)%num_beams_h;
	beam_id_vector.push_back(tempTxBeamId);
	// Left beam
	tempTxBeamId = txBeamIdRow*num_beams_h + (num_beams_h + txBeamIdColumn - 1)%num_beams_h;
	beam_id_vector.push_back(tempTxBeamId);
	// Upper beam
	if (num_beams_v > 1)
	{
		//	tempTxBeamId = ((txBeamIdRow + 1)*16 + txBeamIdColumn)%64;
		tempTxBeamId = txBeamIdColumn + ((txBeamIdRow + 1)%num_beams_v)*num_beams_h;
		beam_id_vector.push_back(tempTxBeamId);
		// Bottom beam
		if (num_beams_v > 2)
		{
			tempTxBeamId = txBeamIdColumn + ((num_beams_v+txBeamIdRow - 1)%num_beams_v)*num_beams_h;
			beam_id_vector.push_back(tempTxBeamId);
		}
	}

	return beam_id_vector;
}


std::vector<uint16_t>
MmWaveBeamManagement::GetAlphaSpacedAzimuthBeamsFromOptimal(uint16_t opt_beam_id, uint16_t alpha, uint16_t num_beams_h)
{
	std::vector<uint16_t> extra_beam_id_vector;
	extra_beam_id_vector.clear();
	uint16_t beamIdEl = floor(opt_beam_id / num_beams_h);
	uint16_t beamIdAz = opt_beam_id % num_beams_h;
	uint16_t beam_offset = alpha;
	while(beam_offset < num_beams_h)
	{
		uint16_t temp_beam_id = beamIdEl*num_beams_h + (beamIdAz+beam_offset)%num_beams_h;
		extra_beam_id_vector.push_back(temp_beam_id);
		beam_offset += alpha;
	}

	return extra_beam_id_vector;

}

/*
 * Alt2.
 */
void
MmWaveBeamManagement::Alt2BeamTrackingList ()
{
	for (std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.begin();
				it1 != m_enbSinrMap.end();
				++it1)
	{
		Ptr<NetDevice> pDevice = it1->first;
		std::vector<BeamPairInfoStruct> candidateBeamPairs;

		// Internal SINR value to control the size of the candidate beam vector.
//		double minSinr=0;
		int nbands = m_enbSinrMap.begin()->second.begin()->second.GetSpectrumModel()->GetNumBands();
		BeamPairInfoStruct beamPair, beamPairExtra;
		beamPair = GetBestScannedBeamPair();
		// The best beam pair is the first one in the tracking list.
		candidateBeamPairs.push_back(beamPair);

		// Now construct the rest of the beam pairs in the immediate vicinity
		// TODO: Hard-coded for tx 16x4 and rx 8x2 arrays. Make it compatible with any geometry

		std::vector<uint16_t> tx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_txBeamId,16,4);
		std::vector<uint16_t> rx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_rxBeamId,8,2);

		for (uint16_t numTxBeams = 0; numTxBeams < tx_beam_ids.size(); numTxBeams++)
		{
			uint16_t tempTxBeamId = tx_beam_ids.at(numTxBeams);
			for(uint16_t numRxBeams = 0; numRxBeams < rx_beam_ids.size(); numRxBeams++)
			{
				if (numTxBeams == 0 && numRxBeams == 0)
				{
					continue;	//This one is already added in the vector
				}
				uint16_t tempRxBeamId = rx_beam_ids.at(numRxBeams);
				std::map <sinrKey,SpectrumValue>::iterator itExtra =
								it1->second.find(std::make_pair(tempTxBeamId,tempRxBeamId));
				beamPairExtra.m_targetNetDevice = pDevice;
				beamPairExtra.m_txBeamId = tempTxBeamId;
				beamPairExtra.m_rxBeamId = tempRxBeamId;
				if(itExtra != it1->second.end())	// Check whether SINR info is already available in the map
				{
					beamPairExtra.m_avgSinr = Sum(itExtra->second)/nbands;
					beamPairExtra.m_sinrPsd = itExtra->second;
				}
				else
				{
					//No SINR value in the map. Just populating beam ids
					beamPairExtra.m_avgSinr = -1;
					beamPairExtra.m_sinrPsd = 0;
				}
				candidateBeamPairs.push_back(beamPairExtra);
			}


		}

		//to map:
		BeamTrackingParams beamTrackingStruct;
		beamTrackingStruct.m_beamPairList = candidateBeamPairs;
		beamTrackingStruct.m_numBeamPairs = candidateBeamPairs.size();
		beamTrackingStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
//		std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator itMap = m_candidateBeamsMap.find(pDevice);
//		if(itMap == m_candidateBeamsMap.end())
//		{
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,beamTrackingStruct));
//		}
//		else
//		{
//			itMap->second = beamTrackingStruct;
//		}
		UpdateBeamTrackingInfo(pDevice,beamTrackingStruct);
	}
}


void
MmWaveBeamManagement::Alt3BeamTrackingList (uint16_t alpha)
{
	for (std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.begin();
				it1 != m_enbSinrMap.end();
				++it1)
	{
		Ptr<NetDevice> pDevice = it1->first;
		std::vector<BeamPairInfoStruct> candidateBeamPairs;

		// Internal SINR value to control the size of the candidate beam vector.
//		double minSinr=0;
		int nbands = m_enbSinrMap.begin()->second.begin()->second.GetSpectrumModel()->GetNumBands();
		BeamPairInfoStruct beamPair, beamPairExtra;
		beamPair = GetBestScannedBeamPair();
		// The best beam pair is the first one in the tracking list.
		candidateBeamPairs.push_back(beamPair);

		// Now construct the rest of the beam pairs in the immediate vicinity
		// TODO: Hard-coded for tx 16x4 and rx 8x2 arrays. Make it compatible with any geometry

		std::vector<uint16_t> tx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_txBeamId,16,4);
		std::vector<uint16_t> rx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_rxBeamId,8,2);

		std::vector<uint16_t> add_rx_beam_ids = GetAlphaSpacedAzimuthBeamsFromOptimal(beamPair.m_rxBeamId,alpha,8);

		// Append additional vector at the end of the original rx beam id vector
		rx_beam_ids.insert(rx_beam_ids.end(),add_rx_beam_ids.begin(),add_rx_beam_ids.end());

		for (uint16_t numTxBeams = 0; numTxBeams < tx_beam_ids.size(); numTxBeams++)
		{
			uint16_t tempTxBeamId = tx_beam_ids.at(numTxBeams);
			for(uint16_t numRxBeams = 0; numRxBeams < rx_beam_ids.size(); numRxBeams++)
			{
				if (numTxBeams == 0 && numRxBeams == 0)
				{
					continue;	//This one is already added in the vector
				}
				uint16_t tempRxBeamId = rx_beam_ids.at(numRxBeams);
				std::map <sinrKey,SpectrumValue>::iterator itExtra =
								it1->second.find(std::make_pair(tempTxBeamId,tempRxBeamId));
				beamPairExtra.m_targetNetDevice = pDevice;
				beamPairExtra.m_txBeamId = tempTxBeamId;
				beamPairExtra.m_rxBeamId = tempRxBeamId;
				if(itExtra != it1->second.end())	// Check whether SINR info is already available in the map
				{
					beamPairExtra.m_avgSinr = Sum(itExtra->second)/nbands;
					beamPairExtra.m_sinrPsd = itExtra->second;
				}
				else
				{
					//No SINR value in the map. Just populating beam ids
					beamPairExtra.m_avgSinr = -1;
					beamPairExtra.m_sinrPsd = 0;
				}
				candidateBeamPairs.push_back(beamPairExtra);
			}


		}

		//to map:
		BeamTrackingParams beamTrackingStruct;
		beamTrackingStruct.m_beamPairList = candidateBeamPairs;
		beamTrackingStruct.m_numBeamPairs = candidateBeamPairs.size();
		beamTrackingStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
//		std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator itMap = m_candidateBeamsMap.find(pDevice);
//		if(itMap == m_candidateBeamsMap.end())
//		{
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,beamTrackingStruct));
//		}
//		else
//		{
//			itMap->second = beamTrackingStruct;
//		}
		UpdateBeamTrackingInfo(pDevice,beamTrackingStruct);
	}
}


/*
 * Alt.4
 */
void
MmWaveBeamManagement::Alt4BeamTrackingList (uint16_t alpha)
{
	for (std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.begin();
				it1 != m_enbSinrMap.end();
				++it1)
	{
		Ptr<NetDevice> pDevice = it1->first;
		std::vector<BeamPairInfoStruct> candidateBeamPairs;

		// Internal SINR value to control the size of the candidate beam vector.
//		double minSinr=0;
		int nbands = m_enbSinrMap.begin()->second.begin()->second.GetSpectrumModel()->GetNumBands();
		BeamPairInfoStruct beamPair, beamPairExtra;
		beamPair = GetBestScannedBeamPair();
		// The best beam pair is the first one in the tracking list.
		candidateBeamPairs.push_back(beamPair);

		// Now construct the rest of the beam pairs in the immediate vicinity
		// TODO: Hard-coded for tx 16x4 and rx 8x2 arrays. Make it compatible with any geometry

		std::vector<uint16_t> tx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_txBeamId,16,4);
		std::vector<uint16_t> rx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_rxBeamId,8,2);

		uint16_t alpha = 4;
		std::vector<uint16_t> add_tx_beam_ids = GetAlphaSpacedAzimuthBeamsFromOptimal(beamPair.m_txBeamId,alpha,16);

		// Append additional vector at the end of the original tx beam id vector
		tx_beam_ids.insert(tx_beam_ids.end(),add_tx_beam_ids.begin(),add_tx_beam_ids.end());

		for (uint16_t numTxBeams = 0; numTxBeams < tx_beam_ids.size(); numTxBeams++)
		{
			uint16_t tempTxBeamId = tx_beam_ids.at(numTxBeams);
			for(uint16_t numRxBeams = 0; numRxBeams < rx_beam_ids.size(); numRxBeams++)
			{
				if (numTxBeams == 0 && numRxBeams == 0)
				{
					continue;	//This one is already added in the vector
				}
				uint16_t tempRxBeamId = rx_beam_ids.at(numRxBeams);
				std::map <sinrKey,SpectrumValue>::iterator itExtra =
								it1->second.find(std::make_pair(tempTxBeamId,tempRxBeamId));
				beamPairExtra.m_targetNetDevice = pDevice;
				beamPairExtra.m_txBeamId = tempTxBeamId;
				beamPairExtra.m_rxBeamId = tempRxBeamId;
				if(itExtra != it1->second.end())	// Check whether SINR info is already available in the map
				{
					beamPairExtra.m_avgSinr = Sum(itExtra->second)/nbands;
					beamPairExtra.m_sinrPsd = itExtra->second;
				}
				else
				{
					//No SINR value in the map. Just populating beam ids
					beamPairExtra.m_avgSinr = -1;
					beamPairExtra.m_sinrPsd = 0;
				}
				candidateBeamPairs.push_back(beamPairExtra);
			}


		}

		//to map:
		BeamTrackingParams beamTrackingStruct;
		beamTrackingStruct.m_beamPairList = candidateBeamPairs;
		beamTrackingStruct.m_numBeamPairs = candidateBeamPairs.size();
		beamTrackingStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
//		std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator itMap = m_candidateBeamsMap.find(pDevice);
//		if(itMap == m_candidateBeamsMap.end())
//		{
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,beamTrackingStruct));
//		}
//		else
//		{
//			itMap->second = beamTrackingStruct;
//		}
		UpdateBeamTrackingInfo(pDevice,beamTrackingStruct);
	}
}


/*
 * Alt.5
 */
void
MmWaveBeamManagement::Alt5BeamTrackingList (uint16_t beta, uint16_t alpha)
{
	for (std::map< Ptr<NetDevice>, std::map <sinrKey,SpectrumValue>>::iterator it1 = m_enbSinrMap.begin();
				it1 != m_enbSinrMap.end();
				++it1)
	{
		Ptr<NetDevice> pDevice = it1->first;
		std::vector<BeamPairInfoStruct> candidateBeamPairs;

		// Internal SINR value to control the size of the candidate beam vector.
//		double minSinr=0;
		int nbands = m_enbSinrMap.begin()->second.begin()->second.GetSpectrumModel()->GetNumBands();
		BeamPairInfoStruct beamPair, beamPairExtra;
		beamPair = GetBestScannedBeamPair();
		// The best beam pair is the first one in the tracking list.
		candidateBeamPairs.push_back(beamPair);

		// Now construct the rest of the beam pairs in the immediate vicinity
		// TODO: Hard-coded for tx 16x4 and rx 8x2 arrays. Make it compatible with any geometry

		std::vector<uint16_t> tx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_txBeamId,16,4);
		std::vector<uint16_t> rx_beam_ids = GetSideImmediateNeighborBeams(beamPair.m_rxBeamId,8,2);

		std::vector<uint16_t> add_tx_beam_ids = GetAlphaSpacedAzimuthBeamsFromOptimal(beamPair.m_txBeamId,beta,16);
		std::vector<uint16_t> add_rx_beam_ids = GetAlphaSpacedAzimuthBeamsFromOptimal(beamPair.m_rxBeamId,alpha,8);

		// Append additional vector at the end of the original tx beam id vector
		tx_beam_ids.insert(tx_beam_ids.end(),add_tx_beam_ids.begin(),add_tx_beam_ids.end());
		// Append additional vector at the end of the original rx beam id vector
		rx_beam_ids.insert(rx_beam_ids.end(),add_rx_beam_ids.begin(),add_rx_beam_ids.end());

		for (uint16_t numTxBeams = 0; numTxBeams < tx_beam_ids.size(); numTxBeams++)
		{
			uint16_t tempTxBeamId = tx_beam_ids.at(numTxBeams);
			for(uint16_t numRxBeams = 0; numRxBeams < rx_beam_ids.size(); numRxBeams++)
			{
				if (numTxBeams == 0 && numRxBeams == 0)
				{
					continue;	//This one is already added in the vector
				}
				uint16_t tempRxBeamId = rx_beam_ids.at(numRxBeams);
				std::map <sinrKey,SpectrumValue>::iterator itExtra =
								it1->second.find(std::make_pair(tempTxBeamId,tempRxBeamId));
				beamPairExtra.m_targetNetDevice = pDevice;
				beamPairExtra.m_txBeamId = tempTxBeamId;
				beamPairExtra.m_rxBeamId = tempRxBeamId;
				if(itExtra != it1->second.end())	// Check whether SINR info is already available in the map
				{
					beamPairExtra.m_avgSinr = Sum(itExtra->second)/nbands;
					beamPairExtra.m_sinrPsd = itExtra->second;
				}
				else
				{
					//No SINR value in the map. Just populating beam ids
					beamPairExtra.m_avgSinr = -1;
					beamPairExtra.m_sinrPsd = 0;
				}
				candidateBeamPairs.push_back(beamPairExtra);
			}


		}

		//to map:
		BeamTrackingParams beamTrackingStruct;
		beamTrackingStruct.m_beamPairList = candidateBeamPairs;
		beamTrackingStruct.m_numBeamPairs = candidateBeamPairs.size();
		beamTrackingStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
//		std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator itMap = m_candidateBeamsMap.find(pDevice);
//		if(itMap == m_candidateBeamsMap.end())
//		{
//			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(pDevice,beamTrackingStruct));
//		}
//		else
//		{
//			itMap->second = beamTrackingStruct;
//		}
		UpdateBeamTrackingInfo(pDevice,beamTrackingStruct);
	}
}

uint16_t
MmWaveBeamManagement::GetCurrentNumBeamPairCandidates()
{
	uint16_t numBeamPairs = 0;
	if (m_candidateBeamsMap.empty() == false)
	{
		for(std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.begin();
					it != m_candidateBeamsMap.end(); ++it)
		{
			numBeamPairs += it->second.m_beamPairList.size();
		}
	}

	return numBeamPairs;
}

uint16_t
MmWaveBeamManagement::GetMaxNumBeamPairCandidates()
{
	return m_maxNumBeamPairCandidates;
}

void MmWaveBeamManagement::SetMaxNumBeamPairCandidates(uint16_t nBeamPairs)
{
	m_maxNumBeamPairCandidates = nBeamPairs;
}

/*
 * This function creates the list of candidate beams to monitor
 */
void
MmWaveBeamManagement::FindBeamPairCandidates()
{

	switch (m_beamCandidateListStrategy)
	{
	case 1:
		FindBeamPairCandidatesSinr();
		break;
	case 2:
		Alt2BeamTrackingList();
		break;
	case 3:
		Alt3BeamTrackingList(m_alpha);
		break;
	case 4:
		Alt4BeamTrackingList(m_beta);
		break;
	case 5:
		Alt5BeamTrackingList(m_beta,m_alpha);
		break;
	default:
		break;
	}
}

BeamPairInfoStruct
MmWaveBeamManagement::FindBestScannedBeamPairAndCreateBeamTrackingList ()
{
	BeamPairInfoStruct bestPairInfo = ExhaustiveBestScannedBeamPairSearch();
	// First create all beam pair candidates
	if (GetBeamReportingEnabledCondition() == true)
	{
		FindBeamPairCandidates();
	}

	return bestPairInfo;
}


BeamPairInfoStruct
MmWaveBeamManagement::ExhaustiveBestScannedBeamPairSearch ()
{
	BeamPairInfoStruct bestPairInfo;
	bestPairInfo.m_txBeamId = 0;
	bestPairInfo.m_rxBeamId = 0;
	bestPairInfo.m_avgSinr = -1;
	bestPairInfo.m_sinrPsd = 0;
	bestPairInfo.m_targetNetDevice = NULL;

	// Now iterate along the map and find the best gNB providing the best beam pairs in terms of SINR
	for (std::map <Ptr<NetDevice>,std::map <sinrKey,SpectrumValue>>::iterator itEnb = m_enbSinrMap.begin();
			itEnb != m_enbSinrMap.end();
			++itEnb)
	{
		Ptr<NetDevice> pDevice = itEnb->first;
		for(std::map <sinrKey,SpectrumValue>::iterator itSinrMap = itEnb->second.begin();
				itSinrMap != itEnb->second.end();
				++itSinrMap)
		{
			int nbands = itSinrMap->second.GetSpectrumModel ()->GetNumBands ();
			double avgSinr = Sum (itSinrMap->second)/nbands;
			if (avgSinr > bestPairInfo.m_avgSinr)
			{
				bestPairInfo.m_targetNetDevice = pDevice;
				bestPairInfo.m_txBeamId = itSinrMap->first.first;
				bestPairInfo.m_rxBeamId = itSinrMap->first.second;
				bestPairInfo.m_avgSinr = avgSinr;
				bestPairInfo.m_sinrPsd = itSinrMap->second;
			}
		}

	}

	return bestPairInfo;
}


void
MmWaveBeamManagement::NotifyBeamPairCandidatesToPeer (Ptr<NetDevice> ueNetDevice)
{
	if(!m_candidateBeamsMap.empty())
	{
		// Get the calling UE NetDevice pointer, which is the first argument in the SetCandidateBeamPairSet call.
		Ptr<NetDevice> thisDevice = ueNetDevice;
		// Iterate along the gNBs
		for (std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.begin();
				it != m_candidateBeamsMap.end();
				++it)
		{
			//
			Ptr<NetDevice> pNetDevice = it->first;
			Ptr<MmWaveEnbNetDevice> pGnbNetDevice= DynamicCast<MmWaveEnbNetDevice> (pNetDevice);
			Ptr<MmWaveEnbPhy> phy = pGnbNetDevice->GetPhy();
			phy->GetBeamManagement()->SetCandidateBeamPairSet(thisDevice,it->second.m_beamPairList);
		}
	}
}

void
MmWaveBeamManagement::SetCandidateBeamPairSet (Ptr<NetDevice> device, std::vector<BeamPairInfoStruct> vector)
{
	std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.find(device);
	BeamTrackingParams beamTrackingStruct;
	beamTrackingStruct.m_beamPairList = vector;
	beamTrackingStruct.m_numBeamPairs = vector.size();
	beamTrackingStruct.csiReportPeriod = static_cast<MmWavePhyMacCommon::CsiReportingPeriod>(m_beamReportingPeriod);
	beamTrackingStruct.m_csiResourceLastAllocation = Simulator::Now();
	if (it != m_candidateBeamsMap.end()) //Found
	{
		it->second = beamTrackingStruct;
	}
	else	//Not found
	{
		m_candidateBeamsMap.insert(std::make_pair(device,beamTrackingStruct));
	}

	//TODO: Consider creating a new method with the following.
//	// Now schedule CSI-RS transmissions for tracking
//	for (std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.begin();
//			it != m_candidateBeamsMap.end(); ++it)
//	{
//		Ptr<NetDevice> pDevice = it->first;
//		std::vector<BeamPairInfoStruct> listOfBeamPairs = it->second.m_beamPairList;
//		for (std::vector<BeamPairInfoStruct>::iterator itV = listOfBeamPairs.begin();
//				itV != listOfBeamPairs.end(); itV++)
//		{
//			//TODO: Call here. params: std::vector<BeamPairInfoStruct>,
//		}
//	}

}


BeamPairInfoStruct
MmWaveBeamManagement::GetBestScannedBeamPair()
{
	return m_bestScannedEnb;
}


void MmWaveBeamManagement::UpdateBestScannedEnb()
{
	// TODO: Modify this function to support multiple beams monitoring.
	// Update the candidate beam set from all available gNBs.


	// Get the strongest pair of beams from the best gNB.
	BeamPairInfoStruct bestScannedBeamPair = FindBestScannedBeamPairAndCreateBeamTrackingList();
	SetBestScannedEnb(bestScannedBeamPair);
	Time currentTime = Simulator::Now();
//	NS_LOG_INFO("[" << currentTime.GetSeconds() <<"]Best beam pair update: tx=" << bestScannedBeamPair.m_txBeamId <<
//			" rx=" << bestScannedBeamPair.m_rxBeamId <<
//			" avgSinr=" << bestScannedBeamPair.m_avgSinr);
	std::cout << "[" << currentTime.GetSeconds() <<"]Best beam pair update: tx=" << bestScannedBeamPair.m_txBeamId <<
				" rx=" << bestScannedBeamPair.m_rxBeamId << " avgSinr=" << bestScannedBeamPair.m_avgSinr <<
				" (SS)" << std::endl;

	// erase map if memoryless beam tracking strategy.
	if(m_memorySs == false && m_beamReportingEnabled == true)
	{
		//ClearAllSinrMapEntries(); //FIXME: memory issues: access to erased data... Schedule this call
		Simulator::Schedule(NanoSeconds(10),&MmWaveBeamManagement::ClearAllSinrMapEntries,this);
	}

}


void
MmWaveBeamManagement::ClearAllSinrMapEntries ()
{
	std::map <Ptr<NetDevice>,std::map <sinrKey,SpectrumValue>>::iterator it;
	for (it = m_enbSinrMap.begin(); it != m_enbSinrMap.end(); ++it)
	{
		it->second.clear();
	}
}


void
MmWaveBeamManagement::ScheduleSsSlotSetStart(MmWavePhyMacCommon::SsBurstPeriods period)
{
	m_ssBlocksLastBeamSweepUpdate = 0;
	Simulator::Schedule(MicroSeconds(1000*period)-NanoSeconds(1),&MmWaveBeamManagement::ScheduleSsSlotSetStart,this,period);
}


std::complex<double>
MmWaveBeamManagement::ParseComplex (std::string strCmplx)
{
    double re = 0.00;
    double im = 0.00;
    size_t findj = 0;
    std::complex<double> out_complex;

    findj = strCmplx.find("i");
    if( findj == std::string::npos )
    {
        im = -1.00;
    }
    else
    {
        strCmplx[findj] = '\0';
    }
    if( ( strCmplx.find("+",1) == std::string::npos && strCmplx.find("-",1) == std::string::npos ) && im != -1 )
    {
        /* No real value */
        re = -1.00;
    }
    std::stringstream stream( strCmplx );
    if( re != -1.00 )
    {
        stream>>re;
    }
    else
    {
        re = 0;
    }
    if( im != -1 )
    {
        stream>>im;
    }
    else
    {
        im = 0.00;
    }
    //  std::cout<<" ---  "<<re<<"  "<<im<<std::endl;
    out_complex = std::complex<double>(re,im);
    return out_complex;
}


complex2DVector_t
MmWaveBeamManagement::LoadCodebookFile (std::string inputFilename)
{
	//std::string filename = "src/mmwave/model/BeamFormingMatrix/TxAntenna.txt";
	complex2DVector_t output;
	NS_LOG_FUNCTION (this << "Loading TxAntenna file " << inputFilename);
	std::ifstream singlefile;
	std::complex<double> complexVar;
	singlefile.open (inputFilename.c_str (), std::ifstream::in);

	NS_LOG_INFO (this << " File: " << inputFilename);
	NS_ASSERT_MSG(singlefile.good (), inputFilename << " file not found");
    std::string line;
    std::string token;
    while( std::getline(singlefile, line) ) //Parse each line of the file
    {
    	complexVector_t txAntenna;
        std::istringstream stream(line);
        while( getline(stream,token,',') ) //Parse each comma separated string in a line
        {
        	complexVar = ParseComplex(token);
		    txAntenna.push_back(complexVar);
		}
        output.push_back(txAntenna);
	}
    return output;
//    NS_LOG_INFO ("TxAntenna[instance:"<<g_enbAntennaInstance.size()<<"][antennaSize:"<<g_enbAntennaInstance[0].size()<<"]");
}

void MmWaveBeamManagement::SetBestScannedEnb(BeamPairInfoStruct bestEnbBeamInfo)
{
	if (bestEnbBeamInfo.m_targetNetDevice)
	{
		m_bestScannedEnb = bestEnbBeamInfo;
	}
}

Time MmWaveBeamManagement::GetNextSsBlockTransmissionTime (Ptr<MmWavePhyMacCommon> mmWaveCommon, uint16_t currentSsBlock)
{
	Time nextScheduledSsBlock;
	//m_currentSsBlockSlotId
	uint16_t beamPatternLength = mmWaveCommon->GetSsBlockPatternLength();
	if (currentSsBlock >= beamPatternLength)
	{
		std::cout << "ERROR at MmWaveBeamManagement::GetNextSsBlockTransmissionTime: "
				"currentSsBlock cannot be larger than beam pattern size (" << beamPatternLength << ")" << std::endl;
	}
	uint16_t nextSsBlockId = currentSsBlock + 1;
	uint16_t currentOfdmSymbol = mmWaveCommon->GetSsBurstOfdmIndex(currentSsBlock);
	uint16_t nextOfdmSymbol;
	Time now = Simulator::Now();
	if(nextSsBlockId < beamPatternLength)
	{
		nextOfdmSymbol = mmWaveCommon->GetSsBurstOfdmIndex(nextSsBlockId);
		uint16_t numSymbols = nextOfdmSymbol-currentOfdmSymbol;
		nextScheduledSsBlock = MicroSeconds(mmWaveCommon->GetSymbolPeriod()*numSymbols);
	}
	else
	{
		nextOfdmSymbol = mmWaveCommon->GetSsBurstOfdmIndex(0);
		m_lastBeamSweepUpdate += MilliSeconds(mmWaveCommon->GetSsBurstSetPeriod());
		Time t2 = NanoSeconds(1000.0 * mmWaveCommon->GetSymbolPeriod() * nextOfdmSymbol);
		nextScheduledSsBlock = m_lastBeamSweepUpdate - now + t2;
	}


	return nextScheduledSsBlock;
}

std::map <Ptr<NetDevice>,BeamTrackingParams>
MmWaveBeamManagement::GetDevicesMapToExpireTimer (Time margin)
{
	std::map <Ptr<NetDevice>,BeamTrackingParams> newMap;
	newMap.clear();
	Time currentTime = Simulator::Now();
	for (std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.begin();
			it != m_candidateBeamsMap.end();
			++it)
	{
		BeamTrackingParams beamsInfo = it->second;
		if (currentTime - beamsInfo.m_csiResourceLastAllocation + margin > MilliSeconds(beamsInfo.csiReportPeriod))
		{
			std::pair<std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator,bool> ret;
			std::pair<Ptr<NetDevice>,BeamTrackingParams> newPair(it->first,it->second);
			ret = newMap.insert(newPair);	//Insert the current iterator position
			if(!ret.second)
			{
				std::cout << "Unable to insert into newMap" << std::endl;
			}
		}
	}
	return newMap;
}

void
MmWaveBeamManagement::IncreaseBeamReportingTimers (std::map <Ptr<NetDevice>,BeamTrackingParams> devicesToUpdate)
{
	if (!devicesToUpdate.empty())
	{
		// Find it in the map stored in the manager
		for (std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it1 = devicesToUpdate.begin();
				it1 != devicesToUpdate.end();
				++it1)
		{
			std::map <Ptr<NetDevice>,BeamTrackingParams>::iterator it2 = m_candidateBeamsMap.find(it1->first);
			if (it2 != m_candidateBeamsMap.end())	// This check might be redundant. The beams were obtained from this map
			{
				it2->second.m_csiResourceLastAllocation =
						it2->second.m_csiResourceLastAllocation + MilliSeconds(it2->second.csiReportPeriod);//Simulator::Now();
			}
		}

	}
}

BeamTrackingParams
MmWaveBeamManagement::GetBeamsToTrackForEnb(Ptr<NetDevice> enb)
{
	BeamTrackingParams beamInfo;
	std::map<Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.find(enb);
	if (it == m_candidateBeamsMap.end())
	{
		std::cout << "ERROR in GetBeamsToTrackForEnb(): m_candidateBeamsMap is empty. Maybe you called this too early" << std::endl;
		beamInfo.m_beamPairList.clear();
		beamInfo.m_numBeamPairs = 0;
		beamInfo.m_csiResourceLastAllocation = Seconds(0);
	}
	else
	{
		beamInfo = it->second;
	}
	return beamInfo;
}

BeamTrackingParams
MmWaveBeamManagement::GetBeamsToTrack()
{
	BeamTrackingParams beamInfo = m_candidateBeamsMap.begin()->second;
	return beamInfo;
}


void
MmWaveBeamManagement::UpdateBeamTrackingInfo (Ptr<NetDevice> peer, BeamTrackingParams beamTrackingInfo)
{
	std::map<Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.find(peer);
		if(it == m_candidateBeamsMap.end())
		{
			m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(peer,beamTrackingInfo));
		}
		else
		{
			(*it).second = beamTrackingInfo;
		}
}


void
MmWaveBeamManagement::UpdateBeamTrackingInfoValues (Ptr<NetDevice> peer, BeamTrackingParams beamTrackingInfo)
{
	std::map<Ptr<NetDevice>,BeamTrackingParams>::iterator it = m_candidateBeamsMap.find(peer);
	if(it == m_candidateBeamsMap.end())
	{
		m_candidateBeamsMap.insert(std::pair<Ptr<NetDevice>,BeamTrackingParams>(peer,beamTrackingInfo));
	}
	else
	{
		// It only has to update the SINR values in the internal list of beam pairs in the manager
//		(*it).second = beamTrackingInfo;
		uint16_t numElements = std::min(it->second.m_beamPairList.size(),beamTrackingInfo.m_beamPairList.size());
		for (uint16_t i = 0; i < numElements; i++)
		{
			// Careful: the beamTrackingInfo beam pairs must match the ones in the manager
			if(it->second.m_beamPairList.at(i).m_txBeamId == beamTrackingInfo.m_beamPairList.at(i).m_txBeamId &&
					it->second.m_beamPairList.at(i).m_rxBeamId == beamTrackingInfo.m_beamPairList.at(i).m_rxBeamId)
			{
//				std::cout << "[tx=" << it->second.m_beamPairList.at(i).m_txBeamId << ",rx="
//										<< it->second.m_beamPairList.at(i).m_rxBeamId << "] oldSinr=" <<
//										it->second.m_beamPairList.at(i).m_avgSinr << " newSinr=" <<
//										beamTrackingInfo.m_beamPairList.at(i).m_avgSinr << std::endl;
				it->second.m_beamPairList.at(i).m_sinrPsd = beamTrackingInfo.m_beamPairList.at(i).m_sinrPsd;
				it->second.m_beamPairList.at(i).m_avgSinr = beamTrackingInfo.m_beamPairList.at(i).m_avgSinr;
				it->second.m_csiResourceLastAllocation = beamTrackingInfo.m_csiResourceLastAllocation;
			}
//			else
//			{
//				std::cout << "CSI-RS measurement with a previous beam pair information" << std::endl;
//			}
		}


	}
}

//void
//MmWaveBeamManagement::SetCandidateBeamAlternative(uint16_t alt, uint16_t alpha)
//{
//	m_beamCandidateListStrategy = alt;
//	if (alt == 3 && (alpha < 0 || alpha > 7))
//	{
//		std::cout << "Wrong alpha parameter. Using Alt.2 instead of Alt.3" << std::endl;
//		m_alpha = 0;
//		m_beamCandidateListStrategy = 2;
//	}
//	m_alpha = alpha;
//	if (m_beamReportingEnabled && m_beamReportingPeriod > 0 && alt > 1)
//	{
//		m_memorySs = false;
//	}
//}


void
MmWaveBeamManagement::SetCandidateBeamAlternative(uint16_t alt, uint16_t alpha, uint16_t beta, bool memory)
{
	m_beamCandidateListStrategy = alt;
	m_alpha = alpha;
	m_memorySs = memory;
	if (alt < 0 || alt > 5)
	{
		std::cout << "Unrecognized beam tracking list strategy option " << alt << ". Please check." << std::endl;
		m_beamCandidateListStrategy = 2;
	}
	else if (alt == 0)
	{
		m_memorySs = true;
		m_maxNumBeamPairCandidates = 1024;
	}
	// Check Alt 1 configuration
	else if (alt == 1)
	{
		m_memorySs = false;
		//m_maxNumBeamPairCandidates = 1024;
	}
	// Check Alt 3 configuration
	else if (alt == 3 && (alpha < 0 || alpha > 7))
	{
		std::cout << "Wrong alpha parameter. Using Alt.3 with alpha = 2" << std::endl;
		m_alpha = 2;
	}
	// Check Alt 4 configuration
	else if (alt == 4 && (beta < 0 || beta > 15))
	{
		std::cout << "Wrong alpha parameter. Using Alt.4 with alpha = 4" << std::endl;
		m_beta = 4;
	}
	// Check Alt 5 configuration
	else if (alt == 5)
	{
		m_alpha = 2;
		m_beta = 4;
	}
}

void
MmWaveBeamManagement::ConfigureBeamReporting ()
{
	EnableBeamReporting();	//Enable beam reporting

//	The use of memory is now configurable with a simulation parameter.
//	if (m_beamCandidateListStrategy > 1)
//	{
//		DisableSsbMeasMemory();
//	}
}

void
MmWaveBeamManagement::EnableSsbMeasMemory ()
{
	m_memorySs = true;
}

void
MmWaveBeamManagement::DisableSsbMeasMemory ()
{
	m_memorySs = false;
}

}

