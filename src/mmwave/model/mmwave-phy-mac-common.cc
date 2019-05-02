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


#include <ns3/mmwave-phy-mac-common.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/double.h>
#include <ns3/string.h>
#include <ns3/attribute-accessor-helper.h>
#include <ns3/simulator.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("MmWavePhyMacCommon");

NS_OBJECT_ENSURE_REGISTERED (MmWavePhyMacCommon);

TypeId
MmWavePhyMacCommon::GetTypeId (void)
{
	static TypeId tid = TypeId("ns3::MmWavePhyMacCommon")
			.SetParent<Object> ()
			.AddConstructor<MmWavePhyMacCommon> ()
			.AddAttribute ("SymbolPerSlot",
						   "Number of symbols per slot",
						   UintegerValue (14),		//UintegerValue (30),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_symbolsPerSlot),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("SymbolsPerSubframe",
							 "OFDM symbols per subframe",
							 UintegerValue (112),	// SCS = 120 KHz //UintegerValue (24),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_symbolsPerSubframe),
							 MakeUintegerChecker<uint32_t> ())
			 .AddAttribute ("SubframePeriod",
							 "Symbol period in microseconds",
							 DoubleValue (1000.0),	//DoubleValue (100.0),
							 MakeDoubleAccessor (&MmWavePhyMacCommon::m_subframePeriod),
							 MakeDoubleChecker<double> ())
		   .AddAttribute ("CtrlSymbols",
							 "Number of OFDM symbols for DL control per subframe",
							 UintegerValue (1),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_ctrlSymbols),
							 MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("SymbolPeriod",
						   "Symbol period in microseconds",
						   DoubleValue (8.33),	//For SCS of 120 KHz	//DoubleValue (4.16),
						   MakeDoubleAccessor (&MmWavePhyMacCommon::m_symbolPeriod),
						   MakeDoubleChecker<double> ())
			.AddAttribute ("SlotsPerSubframe",
						   "Number of slots in one subframe",
						   UintegerValue (8),	// SCS = 120 KHz //UintegerValue (8),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_slotsPerSubframe),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("SubframePerFrame",
						   "Number of subframe per frame",
						   UintegerValue (10),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_subframesPerFrame),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("SubcarriersPerChunk",
						   "Number of sub-carriers per chunk",
						   UintegerValue (48),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_numSubCarriersPerChunk),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("ChunkPerRB",
						   "Number of chunks comprising a resource block",
						   UintegerValue (72),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_chunksPerRb),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("ChunkWidth",
						   "Width of each chunk in Hz",
						   DoubleValue (13.889e6),
						   MakeDoubleAccessor (&MmWavePhyMacCommon::m_chunkWidth),
						   MakeDoubleChecker<double> ())
			.AddAttribute ("ResourceBlockNum",
						   "Number of resource blocks the entire bandwidth is split into",
						   UintegerValue (1),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_numRb),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("NumReferenceSymbols",
						   "Number of reference symbols per slot",
						   UintegerValue (6),
						   MakeUintegerAccessor (&MmWavePhyMacCommon::m_numRefSymbols),
						   MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("CenterFreq",
						   "The center frequency in Hz",
						   DoubleValue (28e9),
						   MakeDoubleAccessor (&MmWavePhyMacCommon::m_centerFrequency),
						   MakeDoubleChecker<double> ())
			.AddAttribute ("TDDPattern",
						   "The control-data pattern for TDD transmission",
						   StringValue ("ccddccdd"),		//StringValue ("ccdddddd"),
						   MakeStringAccessor (&MmWavePhyMacCommon::m_staticTddPattern),
						   MakeStringChecker ())
		  .AddAttribute ("UlSchedDelay",
							 "Number of TTIs between UL scheduling decision and subframe to which it applies",
							 UintegerValue (1),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_ulSchedDelay),
							 MakeUintegerChecker<uint32_t> ())
		  .AddAttribute ("NumRbPerRbg",
							 "Number of resource blocks per resource block group",
							 UintegerValue (1),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_numRbPerRbg),
							 MakeUintegerChecker<uint32_t> ())
			.AddAttribute ("WbCqiPeriod",
							 "Microseconds between wideband DL-CQI reports",
							 UintegerValue (500),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_wbCqiPeriodUs),
							 MakeUintegerChecker<uint32_t> ())
		 .AddAttribute ("GuardPeriod",
							 "Guard period for UL to DL slot transition in microseconds",
							 DoubleValue (16.67),	//DoubleValue (4.16),
							 MakeDoubleAccessor (&MmWavePhyMacCommon::m_guardPeriod),
							 MakeDoubleChecker<double> ())
		 .AddAttribute ("NumHarqProcess",
							 "Number of concurrent stop-and-wait Hybrid ARQ processes per user",
							 UintegerValue (20),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_numHarqProcess),
							 MakeUintegerChecker<uint8_t> ())
		 .AddAttribute ("HarqDlTimeout",
							 "Number of concurrent stop-and-wait Hybrid ARQ processes per user",
							 UintegerValue (20),
							 MakeUintegerAccessor (&MmWavePhyMacCommon::m_harqTimeout),
							 MakeUintegerChecker<uint8_t> ())
		 .AddAttribute ("TbDecodeLatency",
									 "Number of concurrent stop-and-wait Hybrid ARQ processes per user",
									 UintegerValue (100.0),
									 MakeUintegerAccessor (&MmWavePhyMacCommon::m_tbDecodeLatencyUs),
									 MakeUintegerChecker<uint32_t> ())
	;

	return tid;
}

MmWavePhyMacCommon::MmWavePhyMacCommon ()
: m_symbolsPerSlot (14),		// 30
  m_symbolPeriod (8.33),		// 4.16
  m_symbolsPerSubframe (112),	// 24
  m_subframePeriod (1000.0),
  m_ctrlSymbols (1),
  m_dlCtrlSymbols (64),	// 4 OFDM symbols for SS block. Former value was 1
  m_ulCtrlSymbols (1),
  m_slotsPerSubframe (8),
  m_subframesPerFrame (10),
  m_numRefSymbols (6),
	m_numRbPerRbg (1),
  m_numSubCarriersPerChunk (48),
  m_chunksPerRb (72),
  m_numRefScPerRb (6),
  m_numRefScPerSym (864),
  m_chunkWidth (14e6),
  m_numRb (1),
  m_numHarqProcess (20),
  m_harqTimeout (20),
  m_centerFrequency (28e9),
	m_guardPeriod (4.16),
  m_l1L2CtrlLatency (2),
  m_l1L2DataLatency (2),
  m_ulSchedDelay (1),
	m_wbCqiPeriodUs (500),
	m_tbDecodeLatencyUs (100.0),
	m_maxTbSizeBytes (0x7FFFFFFF),
	m_scs (SCS120KHz),		//Carlos modification
	m_slotPeriod (125),
	m_csiPeriodicReportPeriodicity(5),
	m_ssBurstPattern (),
	m_currentSsBlockSlotId (0),
	m_maxSsBlockSlotId (4*5),
	m_ssBurstSetLength (ms5),
	m_ssBurstSetPeriod (ms20),
	m_Xp (ms10),
	m_csiPeriodicResourceAllocationSlots (slot80),
	m_numRbsForPeriodicCsiResource (52)

{
	NS_LOG_INFO ("Initialized MmWavePhyMacCommon");
}

// Carlos modification
void MmWavePhyMacCommon::SetScs (SubCarrierSpacing scs, bool paternFlag)
{
	m_scs = scs;
	m_symbolsPerSlot = 14;
	// The configurations below are assuming slots with 14 OFDM symbols
	switch(scs)
	{
	case SCS15KHz:
		m_symbolPeriod = 66.67;
		m_slotsPerSubframe = 1;
		m_slotPeriod = 1;
		break;
	case SCS30KHz:
		m_symbolPeriod = 33.34;
		m_slotsPerSubframe = 2;
		m_slotPeriod = 500;
		break;
	case SCS60KHz:
		m_symbolPeriod = 16.67;
		m_slotsPerSubframe = 4;
		m_slotPeriod = 250;
		m_ssBurstPattern = ssBurstSetPattern_scs60Khz;
		break;
	case SCS120KHz:
		m_symbolPeriod = 8.33;
		m_slotsPerSubframe = 8;
		m_slotPeriod = 125;
		m_ssBurstPattern = ssBurstSetPattern_scs120Khz;
		break;
	case SCS240KHz:
		m_symbolPeriod = 4.16;
		m_slotsPerSubframe = 16;
		m_slotPeriod = 62.5;
		m_ssBurstPattern = ssBurstSetPattern_scs240Khz;
		break;
	case SCS480KHz:
		m_symbolPeriod = 2.08;
		m_slotsPerSubframe = 32;
		m_slotPeriod = 31.25;
		break;
	case SCS960KHz:
		m_symbolPeriod = 1.04;
		m_slotsPerSubframe = 64;
		m_slotPeriod = 15.625;
		break;
	}
	if(!paternFlag)	//disables the SCS pattern and all slots transmit an SS/PBCH block
	{
		m_ssBurstPattern.clear();
	}
	m_symbolsPerSubframe = m_symbolsPerSlot * m_slotsPerSubframe;
}


void
MmWavePhyMacCommon::SetSsBurstSetPeriod(SsBurstPeriods ssBurstSetPeriod)
{

	m_ssBurstSetLength = ms5; //ssBurstSetLength;
	m_ssBurstSetPeriod = ssBurstSetPeriod;

	m_maxSsBlockSlotId =  m_ssBurstPattern.size();  //Now the number of SS blocks per half-frame is the pattern length. FIXME: Consider moving this line out of the function, it no longer depends on the SS bursts values
}


bool MmWavePhyMacCommon::GetSsBlockSlotStatus ()
{
//	if(m_ssBurstPattern.empty())
//		return true;
//
//	for (std::vector<uint16_t>::iterator it = m_ssBurstPattern.begin(); it < m_ssBurstPattern.end(); it++)
//	{
//		if (*it == m_currentSsBlockSlotId)
//			return true;
//	}
//	return false;
	return CheckSsBlockSlotStatus(m_currentSsBlockSlotId);
}

bool MmWavePhyMacCommon::CheckSsBlockSlotStatus (uint16_t slotId)
{
	if(m_ssBurstPattern.empty())
		return true;

	for (std::vector<uint16_t>::iterator it = m_ssBurstPattern.begin();
			it != m_ssBurstPattern.end() && *it <= slotId; it++)
	{
		if (*it == slotId)
		{
			NS_LOG_INFO ("Beam id " << slotId << " is included in the pattern");
			return true;
		}

	}
	NS_LOG_INFO ("Beam id " << slotId << " is not included in the pattern");
	return false;
}


uint16_t MmWavePhyMacCommon::GetSsBurstOfdmIndex(uint16_t index)
{
	if (m_ssBurstPattern.size() == 0)
	{
		NS_LOG_ERROR("SCS beam pattern is not initialized!");
	}
	return m_ssBurstPattern.at(index);
}


uint32_t
MmWavePhyMacCommon::GetDlBlockSize(uint16_t frameNum, uint8_t subframeNum)
{

	uint16_t numRbs = 1; // Minimum size when no SS block is to be transmitted in the sf
	uint16_t frameSubFrameNum = 10 * frameNum + subframeNum; // Get the sub-frame millisecond
	uint16_t index = frameSubFrameNum % m_ssBurstSetPeriod;
	// index values from 0 to 4 the current sub-frame transmits SS blocks
	if (index < 5)
	{
	    uint16_t numSymbolsPerSf= (uint16_t)m_symbolsPerSubframe;
	    uint16_t min=index*numSymbolsPerSf;
	    uint16_t max=(index+1)*numSymbolsPerSf -1;
	    numRbs = 0;
		for (std::vector<uint16_t>::iterator it = m_ssBurstPattern.begin();
				it != m_ssBurstPattern.end(); it++)
	    {
	        if (*it >= min && *it < max)
	        {
	            numRbs += 4;
	        }
	    }
	}
	return numRbs;

}

void
MmWavePhyMacCommon::SetPeriodicCsiResourcePeriodicity(CsiReportingPeriod Xp)
{
	if (Xp < ms10 || Xp > ms80)
	{
		NS_LOG_ERROR("Periodic CSI-RS resources for tracking must be 10, 20, 40 or 80 ms. Check your configuration");
	}
	m_Xp = Xp;
	uint16_t resourcePeriod = (uint16_t)m_slotsPerSubframe * Xp;
	m_csiPeriodicResourceAllocationSlots = static_cast<ns3::MmWavePhyMacCommon::CsiReportingPeriodSlots>(resourcePeriod);
}

/*
 * @brief sets the size of a CSI-RS resource in number of RBs. It must be a multiple of 4
 */
void
MmWavePhyMacCommon::SetNumBlocksForCsiResources(uint16_t size)
{
	if (size % 4 != 0)
	{
		NS_LOG_ERROR("Periodic CSI-RS resource must be a multiple of 4 RBs. Check your configuration");
	}
	//The size must be the minimum between 24 and size (June 2018).
	m_numRbsForPeriodicCsiResource = (24<size)?24:size;	//std::min(54,size);
}

}

