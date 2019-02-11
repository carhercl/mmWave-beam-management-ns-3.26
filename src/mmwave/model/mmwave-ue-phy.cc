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


#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cfloat>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/double.h>
#include "mmwave-ue-phy.h"
#include "mmwave-ue-net-device.h"
#include "mmwave-spectrum-value-helper.h"
#include <ns3/pointer.h>
#include <ns3/node.h>
#include <ns3/mmwave-helper.h>
#include <ns3/multi-model-spectrum-channel.h>

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("MmWaveUePhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveUePhy);

MmWaveUePhy::MmWaveUePhy ()
{
	NS_LOG_FUNCTION (this);
	NS_FATAL_ERROR ("This constructor should not be called");
}

MmWaveUePhy::MmWaveUePhy (Ptr<MmWaveSpectrumPhy> dlPhy, Ptr<MmWaveSpectrumPhy> ulPhy)
: MmWavePhy(dlPhy, ulPhy),
  m_prevSlot (0),
  m_rnti (0)
{
	NS_LOG_FUNCTION (this);
	m_wbCqiLast = Simulator::Now ();
	m_ueCphySapProvider = new MemberLteUeCphySapProvider<MmWaveUePhy> (this);
	Simulator::ScheduleNow (&MmWaveUePhy::SubframeIndication, this, 0, 0);
	m_csiReportCounter = 5;
	m_currentState = CELL_SEARCH;
	m_bestTxBeamId = 65000;
	m_bestRxBeamId = 65000;
}

MmWaveUePhy::~MmWaveUePhy ()
{
	NS_LOG_FUNCTION (this);
}

TypeId
MmWaveUePhy::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::MmWaveUePhy")
	    .SetParent<MmWavePhy> ()
	    .AddConstructor<MmWaveUePhy> ()
	    .AddAttribute ("TxPower",
	                   "Transmission power in dBm",
	                   DoubleValue (30.0), //TBD zml
	                   MakeDoubleAccessor (&MmWaveUePhy::SetTxPower,
	                                       &MmWaveUePhy::GetTxPower),
	                   MakeDoubleChecker<double> ())
		.AddAttribute ("DlSpectrumPhy",
					    "The downlink MmWaveSpectrumPhy associated to this MmWavePhy",
					    TypeId::ATTR_GET,
					    PointerValue (),
					    MakePointerAccessor (&MmWaveUePhy::GetDlSpectrumPhy),
					    MakePointerChecker <MmWaveSpectrumPhy> ())
		.AddAttribute ("UlSpectrumPhy",
					    "The uplink MmWaveSpectrumPhy associated to this MmWavePhy",
					    TypeId::ATTR_GET,
					    PointerValue (),
					    MakePointerAccessor (&MmWaveUePhy::GetUlSpectrumPhy),
					    MakePointerChecker <MmWaveSpectrumPhy> ())
		.AddTraceSource ("ReportCurrentCellRsrpSinr",
						 "RSRP and SINR statistics.",
						 MakeTraceSourceAccessor (&MmWaveUePhy::m_reportCurrentCellRsrpSinrTrace),
						 "ns3::CurrentCellRsrpSinr::TracedCallback")
		.AddTraceSource ("ReportUplinkTbSize",
						 "Report allocated uplink TB size for trace.",
						 MakeTraceSourceAccessor (&MmWaveUePhy::m_reportUlTbSize),
						 "ns3::UlTbSize::TracedCallback")
		.AddTraceSource ("ReportDownlinkTbSize",
						 "Report allocated downlink TB size for trace.",
						 MakeTraceSourceAccessor (&MmWaveUePhy::m_reportDlTbSize),
						 "ns3::DlTbSize::TracedCallback")
;

	return tid;
}

void
MmWaveUePhy::DoInitialize (void)
{
	NS_LOG_FUNCTION (this);
	m_dlCtrlPeriod = NanoSeconds (1000 * m_phyMacConfig->GetDlCtrlSymbols(m_frameNum, m_sfNum) * m_phyMacConfig->GetSymbolPeriod());
	m_ulCtrlPeriod = NanoSeconds (1000 * m_phyMacConfig->GetUlCtrlSymbols() * m_phyMacConfig->GetSymbolPeriod());

	for (unsigned i = 0; i < m_phyMacConfig->GetSubframesPerFrame(); i++)
	{
		m_sfAllocInfo.push_back(SfAllocInfo(SfnSf (0, i, 0)));
		SlotAllocInfo dlCtrlSlot;
		dlCtrlSlot.m_slotType = SlotAllocInfo::CTRL;
		dlCtrlSlot.m_numCtrlSym = 4;
		dlCtrlSlot.m_tddMode = SlotAllocInfo::DL;
		dlCtrlSlot.m_dci.m_numSym = 1;
		dlCtrlSlot.m_dci.m_symStart = 0;
//		SlotAllocInfo dummySlot;
//		dummySlot.m_slotType = SlotAllocInfo::CTRL_DATA;
//		dummySlot.m_tddMode = SlotAllocInfo::NA;
//		dummySlot.m_dci.m_numSym = 1;
//		dummySlot.m_dci.m_symStart = 0;
		SlotAllocInfo ulCtrlSlot;
		ulCtrlSlot.m_slotType = SlotAllocInfo::CTRL;
		ulCtrlSlot.m_numCtrlSym = 1;
		ulCtrlSlot.m_tddMode = SlotAllocInfo::UL;
		ulCtrlSlot.m_slotIdx = (uint8_t)m_phyMacConfig->GetSlotsPerSubframe()-1;
		ulCtrlSlot.m_dci.m_numSym = 1;
		ulCtrlSlot.m_dci.m_symStart = m_phyMacConfig->GetSymbolsPerSubframe()-1;
		m_sfAllocInfo[i].m_slotAllocInfo.push_back (dlCtrlSlot);
//		for (unsigned j = 1; j < m_phyMacConfig->GetSlotsPerSubframe()-1; j++)
//		{
//			dummySlot.m_slotIdx = j;
//			dummySlot.m_dci.m_symStart = j*m_phyMacConfig->GetSymbPerSlot();
//			m_sfAllocInfo[i].m_slotAllocInfo.push_back (dummySlot);
//		}
		m_sfAllocInfo[i].m_slotAllocInfo.push_back (ulCtrlSlot);
	}

	for (unsigned i = 0; i < m_phyMacConfig->GetTotalNumChunk(); i++)
	{
		m_channelChunks.push_back(i);
	}

	m_sfPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSubframePeriod ());

	// Carlos modification
//	MmWavePhyMacCommon::SsBurstPeriods ssBurstSetperiod = m_phyMacConfig->GetSsBurstSetPeriod();
//	double beamUpdatePeriod = ssBurstSetperiod;
//	Time beamUpdatePeriodTime = MicroSeconds(1000*beamUpdatePeriod);
//	m_beamManagement = CreateObject<MmWaveBeamManagement>();
//	m_beamManagement->InitializeBeamSweepingRx(beamUpdatePeriodTime);
//	m_beamManagement->ScheduleSsSlotSetStart(ssBurstSetperiod);

	MmWavePhy::DoInitialize ();

//	Time Period = m_beamManagement->GetNextSsBlockTransmissionTime(m_phyMacConfig,m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate()); //m_currentSsBlockSlotId
//	Simulator::Schedule(Period,&MmWaveUePhy::StartSsBlockSlot,this); //	StartSsBlockSlot();
	// End of Carlos modification

}

void
MmWaveUePhy::DoDispose (void)
{

}

void
MmWaveUePhy::SetUeCphySapUser (LteUeCphySapUser* s)
{
  NS_LOG_FUNCTION (this);
  m_ueCphySapUser = s;
}

LteUeCphySapProvider*
MmWaveUePhy::GetUeCphySapProvider ()
{
  NS_LOG_FUNCTION (this);
  return (m_ueCphySapProvider);
}

void
MmWaveUePhy::SetTxPower (double pow)
{
	m_txPower = pow;
}
double
MmWaveUePhy::GetTxPower () const
{
	return m_txPower;
}

void
MmWaveUePhy::SetNoiseFigure (double pf)
{

}

double
MmWaveUePhy::GetNoiseFigure () const
{
	return m_noiseFigure;
}

Ptr<SpectrumValue>
MmWaveUePhy::CreateTxPowerSpectralDensity()
{
	// Carlos modification
	// This is done for beam scanning the whole bandwidth. Otherwise, there are no channels to sense
	Ptr<SpectrumValue> psd;
	if(m_subChannelsForTx.empty()) //TODO: Once the UE is attached to an eNB, i guess this vector won't be empty.
	{
		std::vector<int> listOfSubchannels;
		for (unsigned i = 0; i < m_phyMacConfig->GetTotalNumChunk(); i++)
		{
			listOfSubchannels.push_back(i);
		}
		psd = MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (m_phyMacConfig, m_txPower, listOfSubchannels );
	}
	else
	{
		//End of Carlos modification
		psd = MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (m_phyMacConfig, m_txPower, m_subChannelsForTx );
	}
	return psd;
}

void
MmWaveUePhy::DoSetSubChannels()
{

}

void
MmWaveUePhy::SetSubChannelsForReception(std::vector <int> mask)
{

}

std::vector <int>
MmWaveUePhy::GetSubChannelsForReception(void)
{
	std::vector <int> vec;

	return vec;
}

void
MmWaveUePhy::SetSubChannelsForTransmission(std::vector <int> mask)
{
	m_subChannelsForTx = mask;
	Ptr<SpectrumValue> txPsd = CreateTxPowerSpectralDensity ();
	NS_ASSERT (txPsd);
	m_downlinkSpectrumPhy->SetTxPowerSpectralDensity (txPsd);
}

std::vector <int>
MmWaveUePhy::GetSubChannelsForTransmission(void)
{
	std::vector <int> vec;

	return vec;
}

void
MmWaveUePhy::DoSendControlMessage (Ptr<MmWaveControlMessage> msg)
{
  NS_LOG_FUNCTION (this << msg);
  SetControlMessage (msg);
}


void
MmWaveUePhy::RegisterToEnb (uint16_t cellId, Ptr<MmWavePhyMacCommon> config)
{
	m_cellId = cellId;
	//TBD how to assign bandwitdh and earfcn
	m_noiseFigure = 5.0;
	m_phyMacConfig = config;

	Ptr<SpectrumValue> noisePsd =
			MmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (m_phyMacConfig, m_noiseFigure);
	m_downlinkSpectrumPhy->SetNoisePowerSpectralDensity (noisePsd);
	m_downlinkSpectrumPhy->GetSpectrumChannel()->AddRx(m_downlinkSpectrumPhy);
	m_downlinkSpectrumPhy->SetCellId(m_cellId);
//	UpdateChannelMap();
	SetCurrentState(SYNCHRONIZED);
}

Ptr<MmWaveSpectrumPhy>
MmWaveUePhy::GetDlSpectrumPhy () const
{
  return m_downlinkSpectrumPhy;
}

Ptr<MmWaveSpectrumPhy>
MmWaveUePhy::GetUlSpectrumPhy () const
{
  return m_uplinkSpectrumPhy;
}

void
MmWaveUePhy::ReceiveControlMessageList (std::list<Ptr<MmWaveControlMessage> > msgList)
{
	NS_LOG_FUNCTION (this);

	std::list<Ptr<MmWaveControlMessage> >::iterator it;
	for (it = msgList.begin (); it != msgList.end (); it++)
	{
		Ptr<MmWaveControlMessage> msg = (*it);

		if (msg->GetMessageType() == MmWaveControlMessage::DCI_TDMA)
		{
			NS_ASSERT_MSG (m_slotNum == 0, "UE" << m_rnti << " got DCI on slot != 0");
			Ptr<MmWaveTdmaDciMessage> dciMsg = DynamicCast<MmWaveTdmaDciMessage> (msg);
			DciInfoElementTdma dciInfoElem = dciMsg->GetDciInfoElement ();
			SfnSf dciSfn = dciMsg->GetSfnSf ();

			if(dciSfn.m_frameNum != m_frameNum || dciSfn.m_sfNum != m_sfNum)
			{
				NS_FATAL_ERROR ("DCI intended for different subframe (dci= "
						<< (uint16_t)dciSfn.m_frameNum<<" "<<(uint16_t)dciSfn.m_sfNum<<", actual= "<<(uint16_t)m_frameNum<<" "<<(uint16_t)m_sfNum);
			}

//			NS_LOG_DEBUG ("UE" << m_rnti << " DCI received for RNTI " << dciInfoElem.m_rnti << " in frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " slot " << (unsigned)m_slotNum << " format " << (unsigned)dciInfoElem.m_format << " symStart " << (unsigned)dciInfoElem.m_symStart << " numSym " << (unsigned)dciInfoElem.m_numSym);

			if (dciInfoElem.m_rnti != m_rnti)
			{
				continue; // DCI not for me
			}

			if (dciInfoElem.m_format == DciInfoElementTdma::DL) // set downlink slot schedule for current slot
			{
				NS_LOG_DEBUG ("UE" << m_rnti << " DL-DCI received for frame " << m_frameNum << " subframe " << (unsigned)m_sfNum
								<< " symStart " << (unsigned)dciInfoElem.m_symStart << " numSym " << (unsigned)dciInfoElem.m_numSym  << " tbs " << dciInfoElem.m_tbSize
								<< " harqId " << (unsigned)dciInfoElem.m_harqProcess);

				SlotAllocInfo slotInfo;
				slotInfo.m_tddMode = SlotAllocInfo::DL;
				slotInfo.m_dci = dciInfoElem;
				slotInfo.m_slotIdx = 0;
				std::deque <SlotAllocInfo>::iterator itSlot;
				for (itSlot = m_currSfAllocInfo.m_slotAllocInfo.begin ();
						itSlot != m_currSfAllocInfo.m_slotAllocInfo.end (); itSlot++)
				{
					if (itSlot->m_tddMode == SlotAllocInfo::UL)
					{
						break;
					}
					slotInfo.m_slotIdx++;
				}
				//m_currSfAllocInfo.m_slotAllocInfo.push_back (slotInfo);  // add SlotAllocInfo to current SfAllocInfo
				m_currSfAllocInfo.m_slotAllocInfo.insert (itSlot, slotInfo);
			}
			else if (dciInfoElem.m_format == DciInfoElementTdma::UL) // set downlink slot schedule for t+Tul_sched slot
			{
				uint8_t ulSfIdx = (m_sfNum + m_phyMacConfig->GetUlSchedDelay()) % m_phyMacConfig->GetSubframesPerFrame ();
				uint16_t dciFrame = (ulSfIdx > m_sfNum) ? m_frameNum : m_frameNum+1;

				NS_LOG_DEBUG ("UE" << m_rnti << " UL-DCI received for frame " << dciFrame << " subframe " << (unsigned)ulSfIdx
						     << " symStart " << (unsigned)dciInfoElem.m_symStart << " numSym " << (unsigned)dciInfoElem.m_numSym << " tbs " << dciInfoElem.m_tbSize
						     << " harqId " << (unsigned)dciInfoElem.m_harqProcess);

				SlotAllocInfo slotInfo;
				slotInfo.m_tddMode = SlotAllocInfo::UL;
				slotInfo.m_dci = dciInfoElem;
				SlotAllocInfo ulCtrlSlot = m_sfAllocInfo[ulSfIdx].m_slotAllocInfo.back ();
				m_sfAllocInfo[ulSfIdx].m_slotAllocInfo.pop_back ();
				//ulCtrlSlot.m_slotIdx++;
				slotInfo.m_slotIdx = m_sfAllocInfo[ulSfIdx].m_slotAllocInfo.size ();
				m_sfAllocInfo[ulSfIdx].m_slotAllocInfo.push_back (slotInfo);
				m_sfAllocInfo[ulSfIdx].m_slotAllocInfo.push_back (ulCtrlSlot);
			}

			m_phySapUser->ReceiveControlMessage (msg);
		}
		else if (msg->GetMessageType () == MmWaveControlMessage::MIB)
		{
			NS_LOG_INFO ("received MIB");
			NS_ASSERT (m_cellId > 0);
			Ptr<MmWaveMibMessage> msg2 = DynamicCast<MmWaveMibMessage> (msg);
			m_ueCphySapUser->RecvMasterInformationBlock (m_cellId, msg2->GetMib ());
		}
		else if (msg->GetMessageType () == MmWaveControlMessage::SIB1)
		{
			NS_ASSERT (m_cellId > 0);
			Ptr<MmWaveSib1Message> msg2 = DynamicCast<MmWaveSib1Message> (msg);
			m_ueCphySapUser->RecvSystemInformationBlockType1 (m_cellId, msg2->GetSib1 ());
		}
		else if (msg->GetMessageType () == MmWaveControlMessage::RAR)
		{
			NS_LOG_INFO ("received RAR");
			NS_ASSERT (m_cellId > 0);

			Ptr<MmWaveRarMessage> rarMsg = DynamicCast<MmWaveRarMessage> (msg);

			for (std::list<MmWaveRarMessage::Rar>::const_iterator it = rarMsg->RarListBegin ();
					it != rarMsg->RarListEnd ();
					++it)
			{
				if (it->rapId == m_raPreambleId)
				{
					m_phySapUser->ReceiveControlMessage (rarMsg);
				}
			}
		}
		else
		{
			NS_LOG_DEBUG ("Control message not handled. Type: "<< msg->GetMessageType());
		}
	}
}

void
MmWaveUePhy::QueueUlTbAlloc (TbAllocInfo m)
{
  NS_LOG_FUNCTION (this);
//  NS_LOG_DEBUG ("UL TB Info Elem queue size == " << m_ulTbAllocQueue.size ());
  m_ulTbAllocQueue.at (m_phyMacConfig->GetUlSchedDelay ()-1).push_back (m);
}

std::list<TbAllocInfo>
MmWaveUePhy::DequeueUlTbAlloc (void)
{
	NS_LOG_FUNCTION (this);

	if (m_ulTbAllocQueue.empty())
	{
		std::list<TbAllocInfo> emptylist;
		return (emptylist);
	}

	if (m_ulTbAllocQueue.at (0).size () > 0)
	{
		std::list<TbAllocInfo> ret = m_ulTbAllocQueue.at (0);
		m_ulTbAllocQueue.erase (m_ulTbAllocQueue.begin ());
		std::list<TbAllocInfo> l;
		m_ulTbAllocQueue.push_back (l);
		return (ret);
	}
	else
	{
		m_ulTbAllocQueue.erase (m_ulTbAllocQueue.begin ());
		std::list<TbAllocInfo> l;
		m_ulTbAllocQueue.push_back (l);
		std::list<TbAllocInfo> emptylist;
		return (emptylist);
	}
}

void
MmWaveUePhy::SubframeIndication (uint16_t frameNum, uint8_t sfNum)
{
	m_frameNum = frameNum;
	m_sfNum = sfNum;
	m_lastSfStart = Simulator::Now();
	m_currSfAllocInfo = m_sfAllocInfo[m_sfNum];
	NS_ASSERT ((m_currSfAllocInfo.m_sfnSf.m_frameNum == m_frameNum) &&
	           (m_currSfAllocInfo.m_sfnSf.m_sfNum == m_sfNum));
	m_sfAllocInfo[m_sfNum] = SfAllocInfo (SfnSf (m_frameNum+1, m_sfNum, 0));
	SlotAllocInfo dlCtrlSlot;
	dlCtrlSlot.m_slotType = SlotAllocInfo::CTRL;
	dlCtrlSlot.m_numCtrlSym = 4;
	dlCtrlSlot.m_tddMode = SlotAllocInfo::DL;
	dlCtrlSlot.m_slotIdx = 0x00;
	dlCtrlSlot.m_dci.m_numSym = 1;
	dlCtrlSlot.m_dci.m_symStart = 0;
//	SlotAllocInfo dummySlot;
//	dummySlot.m_slotType = SlotAllocInfo::CTRL_DATA;
//	dummySlot.m_tddMode = SlotAllocInfo::NA;
//	dummySlot.m_dci.m_numSym = 1;
//	dummySlot.m_dci.m_symStart = 0;
	SlotAllocInfo ulCtrlSlot;
	ulCtrlSlot.m_slotType = SlotAllocInfo::CTRL;
	ulCtrlSlot.m_numCtrlSym = 1;
	ulCtrlSlot.m_tddMode = SlotAllocInfo::UL;
	ulCtrlSlot.m_slotIdx = 0xFF;
	ulCtrlSlot.m_dci.m_numSym = 1;
	ulCtrlSlot.m_dci.m_symStart = m_phyMacConfig->GetSymbolsPerSubframe()-1;
	m_sfAllocInfo[m_sfNum].m_slotAllocInfo.push_front (dlCtrlSlot);
//	for (unsigned i = 1; i < m_phyMacConfig->GetSlotsPerSubframe()-1; i++)
//	{
//		dummySlot.m_slotIdx = i;
//		dummySlot.m_dci.m_symStart = i*m_phyMacConfig->GetSymbPerSlot();
//		m_sfAllocInfo[m_sfNum].m_slotAllocInfo.push_back (dummySlot);
//	}
	m_sfAllocInfo[m_sfNum].m_slotAllocInfo.push_back (ulCtrlSlot);

	StartSlot ();
}

void
MmWaveUePhy::StartSlot ()
{
	//unsigned slotInd = 0;
	SlotAllocInfo currSlot;


	/*if (m_slotNum >= m_currSfAllocInfo.m_dlSlotAllocInfo.size ())
	{
		if (m_currSfAllocInfo.m_ulSlotAllocInfo.size () > 0)
		{
			slotInd = m_slotNum - m_currSfAllocInfo.m_dlSlotAllocInfo.size ();
			currSlot = m_currSfAllocInfo.m_ulSlotAllocInfo[slotInd];
		}
	}
	else
	{
		if (m_currSfAllocInfo.m_ulSlotAllocInfo.size () > 0)
		{
			slotInd = m_slotNum;
			currSlot = m_currSfAllocInfo.m_dlSlotAllocInfo[slotInd];
		}
	}*/

//	m_beamManagement->DisplayCurrentBeamId();

	currSlot = m_currSfAllocInfo.m_slotAllocInfo[m_slotNum];
	m_currSlot = currSlot;

//	Time CurrentTime = Simulator::Now();
//	std::cout << "[" << CurrentTime << "] UE slot: " << m_slotNum << std::endl;

	NS_LOG_INFO ("UE " << m_rnti << " frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " slot " << (unsigned)m_slotNum);

	Time slotPeriod;
	m_receptionEnabled = false;

	if (m_slotNum == 0)  // reserved DL control
	{
		uint32_t DlCtrlSymbols = m_phyMacConfig->GetDlCtrlSymbols (m_frameNum, m_sfNum);
		slotPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod () * DlCtrlSymbols);
		NS_LOG_DEBUG ("UE" << m_rnti << " RXing DL CTRL frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
		              << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1) <<
				              "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod));
	}
	else if (m_slotNum == m_currSfAllocInfo.m_slotAllocInfo.size()-1) // reserved UL control
	{
		currSlot = m_currSfAllocInfo.m_slotAllocInfo[m_currSfAllocInfo.m_slotAllocInfo.size()-1];
		SetSubChannelsForTransmission (m_channelChunks);
		slotPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod() * currSlot.m_dci.m_numSym);
		std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
		NS_LOG_DEBUG ("UE" << m_rnti << " TXing UL CTRL frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
		              << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1) <<
			              "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod-NanoSeconds(1.0)));
		SendCtrlChannels (ctrlMsg, slotPeriod-NanoSeconds(1.0));
	}
	else if (currSlot.m_dci.m_format == DciInfoElementTdma::DL)  // scheduled DL data slot
	{
		m_receptionEnabled = true;

		uint16_t numSym = currSlot.m_dci.m_numSym;
//		if(numSym > m_phyMacConfig->GetSymbPerSlot())
//		{
//			numSym = m_phyMacConfig->GetSymbPerSlot();
//		}
		slotPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod() * numSym);//currSlot.m_dci.m_numSym);

		m_downlinkSpectrumPhy->AddExpectedTb (currSlot.m_dci.m_rnti, currSlot.m_dci.m_ndi, currSlot.m_dci.m_tbSize, currSlot.m_dci.m_mcs,
		                                      m_channelChunks, currSlot.m_dci.m_harqProcess, currSlot.m_dci.m_rv, true,
		                                      currSlot.m_dci.m_symStart, currSlot.m_dci.m_numSym);
		m_reportDlTbSize (GetDevice ()->GetObject <MmWaveUeNetDevice> ()->GetImsi(), currSlot.m_dci.m_tbSize);
		NS_LOG_DEBUG ("UE" << m_rnti << " RXing DL DATA frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
		              << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1) <<
		              "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod));

	}
	else if (currSlot.m_dci.m_format == DciInfoElementTdma::UL) // scheduled UL data slot
	{
		SetSubChannelsForTransmission (m_channelChunks);

		uint16_t numSym = currSlot.m_dci.m_numSym;
		slotPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod() * numSym);//currSlot.m_dci.m_numSym);

		std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
		Ptr<PacketBurst> pktBurst = GetPacketBurst (SfnSf(m_frameNum, m_sfNum, currSlot.m_dci.m_symStart));
		if (pktBurst && pktBurst->GetNPackets () > 0)
		{
			std::list< Ptr<Packet> > pkts = pktBurst->GetPackets ();
			MmWaveMacPduTag tag;
			pkts.front ()->PeekPacketTag (tag);
			NS_ASSERT ((tag.GetSfn().m_sfNum == m_sfNum) && (tag.GetSfn().m_slotNum == currSlot.m_dci.m_symStart));

			LteRadioBearerTag bearerTag;
			if(!pkts.front ()->PeekPacketTag (bearerTag))
			{
				NS_FATAL_ERROR ("No radio bearer tag");
			}
		}
		else
		{
			// sometimes the UE will be scheduled when no data is queued
			// in this case, send an empty PDU
			MmWaveMacPduTag tag (SfnSf(m_frameNum, m_sfNum, currSlot.m_dci.m_symStart));
			Ptr<Packet> emptyPdu = Create <Packet> ();
			MmWaveMacPduHeader header;
			MacSubheader subheader (3, 0);  // lcid = 3, size = 0
			header.AddSubheader (subheader);
			emptyPdu->AddHeader (header);
			emptyPdu->AddPacketTag (tag);
			LteRadioBearerTag bearerTag (m_rnti, 3, 0);
			emptyPdu->AddPacketTag (bearerTag);
			pktBurst = CreateObject<PacketBurst> ();
			pktBurst->AddPacket (emptyPdu);
		}
		m_reportUlTbSize (GetDevice ()->GetObject <MmWaveUeNetDevice> ()->GetImsi(), currSlot.m_dci.m_tbSize);
		NS_LOG_DEBUG ("UE" << m_rnti << " TXing UL DATA frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
		              << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1)
		              << "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod));

		Simulator::Schedule (NanoSeconds(1.0), &MmWaveUePhy::SendDataChannels, this, pktBurst, ctrlMsg, slotPeriod-NanoSeconds(2.0), m_slotNum);
	}
//	else if (currSlot.m_tddMode == SlotAllocInfo::NA)
//	{
////		std::cout << "ue NA" << std::endl;
//		slotPeriod = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod() * currSlot.m_dci.m_numSym);
//	}

	m_prevSlotDir = currSlot.m_tddMode;

	m_phySapUser->SubframeIndication (SfnSf(m_frameNum, m_sfNum, m_slotNum)); 	// trigger mac


	//NS_LOG_DEBUG ("MmWaveUePhy: Scheduling slot end for " << slotPeriod);
//	slotPeriod = NanoSeconds (1000.0*m_phyMacConfig->GetSlotPeriod());
	Simulator::Schedule (slotPeriod, &MmWaveUePhy::EndSlot, this);
}


void
MmWaveUePhy::EndSlot ()
{
	if (m_slotNum == m_currSfAllocInfo.m_slotAllocInfo.size()-1)
//	if (m_slotNum == m_phyMacConfig->GetSlotsPerSubframe()-1)
	{	// end of subframe
		uint16_t frameNum;
		uint8_t sfNum;
		if (m_sfNum == m_phyMacConfig->GetSubframesPerFrame ()-1)
		{
			sfNum = 0;
			frameNum = m_frameNum + 1;
		}
		else
		{
			frameNum = m_frameNum;
			sfNum = m_sfNum + 1;
		}
		m_slotNum = 0;
		//NS_LOG_DEBUG ("MmWaveUePhy: Next subframe scheduled for " << m_lastSfStart + m_sfPeriod - Simulator::Now());
		Simulator::Schedule (m_lastSfStart + m_sfPeriod - Simulator::Now(), &MmWaveUePhy::SubframeIndication, this, frameNum, sfNum);
	}
	else
	{
		Time nextSlotStart;
		/*uint8_t slotInd = m_slotNum+1;
		if (slotInd >= m_currSfAllocInfo.m_dlSlotAllocInfo.size ())
		{
			if (m_currSfAllocInfo.m_ulSlotAllocInfo.size () > 0)
			{
				slotInd = slotInd - m_currSfAllocInfo.m_dlSlotAllocInfo.size ();
				nextSlotStart = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod () *
				                             m_currSfAllocInfo.m_ulSlotAllocInfo[slotInd].m_dci.m_symStart);
			}
		}
		else
		{
			if (m_currSfAllocInfo.m_ulSlotAllocInfo.size () > 0)
			{
				nextSlotStart = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod () *
				                             m_currSfAllocInfo.m_dlSlotAllocInfo[slotInd].m_dci.m_symStart);
			}
		}*/
		m_slotNum++;
		nextSlotStart = NanoSeconds (1000.0 * m_phyMacConfig->GetSymbolPeriod () *
						                             m_currSfAllocInfo.m_slotAllocInfo[m_slotNum].m_dci.m_symStart);
//		nextSlotStart = NanoSeconds(1000.0 * m_phyMacConfig->GetSlotPeriod() * (uint16_t)m_slotNum);
		Simulator::Schedule (nextSlotStart+m_lastSfStart-Simulator::Now(), &MmWaveUePhy::StartSlot, this);
//		nextSlotStart = NanoSeconds(1000*m_phyMacConfig->GetSlotPeriod()*(uint16_t)m_slotNum)+m_lastSfStart-Simulator::Now();
//		Simulator::Schedule (nextSlotStart, &MmWaveUePhy::StartSlot, this);
	}

	if (m_receptionEnabled)
	{
		m_receptionEnabled = false;
	}
}


void
MmWaveUePhy::StartSsBlockSlot()
{
	// Do the beam management: increase ss block beam id to select the next beam to transmit the ss block
//	if (m_phyMacConfig->GetSsBlockSlotStatus() && m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() < 64)	//Check if the current slot contains a SS block to transmit
//	uint16_t numTxBeamsBeforeRxBeamUpdate = m_phyMacConfig->GetSsBlockPatternLength();  //SS block pattern vector length
//	uint16_t numTxBeamsOnCurrentRxBeam = m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate();
//	if (numTxBeamsOnCurrentRxBeam < numTxBeamsBeforeRxBeamUpdate)	//Check if the current slot contains a SS block to transmit
	Time Period = m_beamManagement->GetNextSsBlockTransmissionTime(m_phyMacConfig,m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate()); //m_currentSsBlockSlotId
	BeamPairInfoStruct bestBeams;
	{
		Architecture phyArch = GetPhyArchitecture();

		switch(phyArch)
		{
		case Hybrid:
			NS_ABORT_MSG("UE PHY: Unsupported physical (transceiver) architecture");
			break;
		// Analog case
		case Analog:
			// Get the gain of the current beam
			GetBeamGain();

			// Call to update beam sweeping beam id if it is time to do so
			if (m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() == m_phyMacConfig->GetSsBlockPatternLength()-1)
			{
//				m_beamManagement->BeamSweepStepRx();
				Simulator::Schedule(Period-NanoSeconds(1), &MmWaveBeamManagement::BeamSweepStepRx,m_beamManagement);

//				// Reset counter: this is now scheduled every SS burst set period. See MmWaveUePhy::DoInitialize()
//				m_beamManagement->ResetNumBlocksSinceLastBeamSweepUpdate();
				// If ss slot id is zero for the first time get the best serving enb and pair of beams
				// This means that all beams have been measured and the UE selects the best one

				if(m_beamManagement->GetCurrentBeamId() == 0)	//If id is zero the beam sweep has been completed
				{
					m_beamManagement->UpdateBestScannedEnb();
					bestBeams = m_beamManagement->GetBestScannedBeamPair();
					if(bestBeams.m_txBeamId != m_bestTxBeamId || bestBeams.m_rxBeamId != m_bestRxBeamId)
					{
						m_bestTxBeamId = bestBeams.m_txBeamId;
						m_bestRxBeamId = bestBeams.m_rxBeamId;
						UpdateChannelMap();
						if (GetCurrentState() == CELL_SEARCH)
						{
							//Attach to eNB and change state to SYNCRONIZED
							BeamPairInfoStruct bestBeamInfo = m_beamManagement->GetBestScannedBeamPair();
	//						uint16_t cellId = bestBeamInfo.m_targetNetDevice->GetObject<MmWaveEnbNetDevice> ()->GetCellId ();
							//FIXME:
	//						std::cout << "Current time = "<< Simulator::Now() << std::endl;
							Time updateTime =Simulator::Now()+MicroSeconds(bestBeamInfo.m_txBeamId * m_phyMacConfig->GetSlotPeriod());
	//						Simulator::Schedule(MicroSeconds(bestBeamInfo.m_txBeamId * m_phyMacConfig->GetSlotPeriod())
	//								,&MmWaveUePhy::RegisterToEnb,this,cellId,m_phyMacConfig);
							Simulator::Schedule(updateTime,&MmWaveUePhy::AttachToSelectedEnb,this,m_netDevice,bestBeamInfo.m_targetNetDevice);

	//						AttachToSelectedEnb
						}
//						else
						{
							//TODO: Attach to the eNB if it is a different one
	//						Simulator::Schedule (MicroSeconds(3*100), &MmWaveSpectrumPhy::UpdateSinrPerceived,
	//																	 enbDev->GetPhy()->GetDlSpectrumPhy (), specVals);
							Ptr<MmWaveEnbNetDevice> enb =
									DynamicCast<MmWaveEnbNetDevice>(bestBeams.m_targetNetDevice);
	//						Simulator::Schedule (NanoSeconds(1), &MmWaveSpectrumPhy::UpdateSinrPerceived,
	//								(enb->GetPhy()->GetDlSpectrumPhy(),m_beamManagement->GetBestScannedBeamPair().m_sinrPsd));
							enb->GetPhy()->GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);
							GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);

							Ptr<NetDevice> UeDevice = m_netDevice;
							m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);
	//						SetCurrentState(SYNCHRONIZED);

							// In the first report, enable beam measurement for periodic CSI
							if (m_phyMacConfig->GetPeriodicCsiResourceAllocationCondition() == true &&
									!m_beamManagement->GetBeamReportingEnabledCondition())
							{
								m_beamManagement->FindBeamPairCandidates();	// Create initial list of beams to track
	//							m_beamManagement->EnableBeamReporting();	//Enable beam reporting
								m_beamManagement->ConfigureBeamReporting(); // Configures and enables beam reporting
								m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);	// Notify list to peer
								// Schedule periodic CSI measurements
								Time CsiReporting = MilliSeconds(m_beamManagement->GetBeamReportingPeriod());
	//							std::cout << "CSI periodic resource allocation time: " << CsiReporting.GetMilliSeconds() << std::endl;
								Simulator::Schedule(CsiReporting,&MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine,this);
							}
						}
					}

	//				// Uplink
	//				if(GetCurrentState() == SYNCHRONIZED)
	//				{
	//					SetSubChannelsForTransmission (m_channelChunks);
	//					std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
	//					NS_LOG_DEBUG ("UE" << m_rnti << " TXing UL CTRL frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
	//								  << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1) <<
	//									  "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod-NanoSeconds(1.0)));
	//					SendCtrlChannels (ctrlMsg, slotPeriod-NanoSeconds(1.0));
	//				}
				}

			}
			else if(m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() > 64)
			{
				std::cout << "Current SS block: " << m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() << std::endl;
			}
			break;

		// Analog_fast case
		case Analog_fast:
			// Get the gain of the current beam
			GetBeamGain();

			// Call to update beam sweeping beam id if it is time to do so
			if (m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() == m_phyMacConfig->GetSsBlockPatternLength()-1) // FIXME: TX codebook length is 64
			{
//				m_beamManagement->BeamSweepStepRx();
				Simulator::Schedule(Period-NanoSeconds(1), &MmWaveBeamManagement::BeamSweepStepRx,m_beamManagement);

//				// Reset counter: this is now scheduled every SS burst set period. See MmWaveUePhy::DoInitialize()
//				m_beamManagement->ResetNumBlocksSinceLastBeamSweepUpdate();
				// If ss slot id is zero for the first time get the best serving enb and pair of beams
				// This means that all beams have been measured and the UE selects the best one

				m_beamManagement->UpdateBestScannedEnb();
				bestBeams = m_beamManagement->GetBestScannedBeamPair();
				if(bestBeams.m_txBeamId != m_bestTxBeamId || bestBeams.m_rxBeamId != m_bestRxBeamId)
				{
					m_bestTxBeamId = bestBeams.m_txBeamId;
					m_bestRxBeamId = bestBeams.m_rxBeamId;
					UpdateChannelMap();
					if (GetCurrentState() == CELL_SEARCH)
					{
						//Attach to eNB and change state to SYNCRONIZED
//						uint16_t cellId = bestBeamInfo.m_targetNetDevice->GetObject<MmWaveEnbNetDevice> ()->GetCellId ();
						//FIXME:
//						std::cout << "Current time = "<< Simulator::Now() << std::endl;
						Time updateTime =Simulator::Now()+MicroSeconds(bestBeams.m_txBeamId * m_phyMacConfig->GetSlotPeriod());
//						Simulator::Schedule(MicroSeconds(bestBeamInfo.m_txBeamId * m_phyMacConfig->GetSlotPeriod())
//								,&MmWaveUePhy::RegisterToEnb,this,cellId,m_phyMacConfig);
						Simulator::Schedule(updateTime,&MmWaveUePhy::AttachToSelectedEnb,this,m_netDevice,bestBeams.m_targetNetDevice);

//						AttachToSelectedEnb
					}
//					else
					{
						//TODO: Attach to the eNB if it is a different one
//						Simulator::Schedule (MicroSeconds(3*100), &MmWaveSpectrumPhy::UpdateSinrPerceived,
//																	 enbDev->GetPhy()->GetDlSpectrumPhy (), specVals);
						Ptr<MmWaveEnbNetDevice> enb =
								DynamicCast<MmWaveEnbNetDevice>(bestBeams.m_targetNetDevice);
//						Simulator::Schedule (NanoSeconds(1), &MmWaveSpectrumPhy::UpdateSinrPerceived,
//								(enb->GetPhy()->GetDlSpectrumPhy(),m_beamManagement->GetBestScannedBeamPair().m_sinrPsd));
						enb->GetPhy()->GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);
						GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);

						Ptr<NetDevice> UeDevice = m_netDevice;

						// In the first report, enable beam measurement for periodic CSI
						if (m_phyMacConfig->GetPeriodicCsiResourceAllocationCondition() == true &&
								!m_beamManagement->GetBeamReportingEnabledCondition())
						{
							m_beamManagement->FindBeamPairCandidates();	// Create initial list of beams to track
//							m_beamManagement->EnableBeamReporting();	//Enable beam reporting
							m_beamManagement->ConfigureBeamReporting(); // Configures and enables beam reporting							m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);	// Notify list to peer
							// Schedule periodic CSI measurements
							Time CsiReporting = MilliSeconds(m_beamManagement->GetBeamReportingPeriod());
//							std::cout << "CSI periodic resource allocation time: " << CsiReporting.GetMilliSeconds() << std::endl;
							Simulator::Schedule(CsiReporting,&MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine,this);
						}
						m_beamManagement->FindBeamPairCandidates();	// Create list of beams to track
						m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);

//						SetCurrentState(SYNCHRONIZED);
					}
				}

//				// Uplink
//				if(GetCurrentState() == SYNCHRONIZED)
//				{
//					SetSubChannelsForTransmission (m_channelChunks);
//					std::list<Ptr<MmWaveControlMessage> > ctrlMsg = GetControlMessages ();
//					NS_LOG_DEBUG ("UE" << m_rnti << " TXing UL CTRL frame " << m_frameNum << " subframe " << (unsigned)m_sfNum << " symbols "
//								  << (unsigned)currSlot.m_dci.m_symStart << "-" << (unsigned)(currSlot.m_dci.m_symStart+currSlot.m_dci.m_numSym-1) <<
//									  "\t start " << Simulator::Now() << " end " << (Simulator::Now()+slotPeriod-NanoSeconds(1.0)));
//					SendCtrlChannels (ctrlMsg, slotPeriod-NanoSeconds(1.0));
//				}
			}

			break;

		// In the digital case, all receiving beam directions (all the codebooks) can be obtained at the same time
		case Digital:

			uint16_t rxBeamId = m_beamManagement->GetCurrentBeamId();
			if (rxBeamId != 0)
				std::cout << "ERROR: RX beam id should not be zero at the beginning of the beam sweeping" << std::endl;

			while (rxBeamId != 16)	//FIXME: Implement a method to obtain the codebook length
			{
				// Get the gain of the current beam
				GetBeamGain();
				//
				m_beamManagement->BeamSweepStepRx();
				// next iteration
				rxBeamId++;
			}
//			uint16_t txBeamId = m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate();
//			if (txBeamId == m_phyMacConfig->GetSsBlockPatternLength()-1)	//FIXME: Implement a method to obtain the codebook length
			{
				m_beamManagement->UpdateBestScannedEnb();
				bestBeams = m_beamManagement->GetBestScannedBeamPair();
				if(bestBeams.m_txBeamId != m_bestTxBeamId || bestBeams.m_rxBeamId != m_bestRxBeamId)
				{
					m_bestTxBeamId = bestBeams.m_txBeamId;
					m_bestRxBeamId = bestBeams.m_rxBeamId;
					UpdateChannelMap();

					if (GetCurrentState() == CELL_SEARCH)
					{
						//Attach to eNB and change state to SYNCRONIZED
	//					uint16_t cellId = m_beamManagement->GetBestScannedBeamPair().m_targetNetDevice->GetObject<MmWaveEnbNetDevice> ()->GetCellId ();
	//					RegisterToEnb (cellId, m_phyMacConfig);
						AttachToSelectedEnb(m_netDevice,bestBeams.m_targetNetDevice);
						SetCurrentState(SYNCHRONIZED);	// This is called in RegisterToEnb method
					}
//					else
					{
						Ptr<MmWaveEnbNetDevice> enb =
								DynamicCast<MmWaveEnbNetDevice>(bestBeams.m_targetNetDevice);
//						Simulator::Schedule (NanoSeconds(1), &MmWaveSpectrumPhy::UpdateSinrPerceived,
//								(enb->GetPhy()->GetDlSpectrumPhy(),m_beamManagement->GetBestScannedBeamPair().m_sinrPsd));
						enb->GetPhy()->GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);
						GetDlSpectrumPhy()->UpdateSinrPerceived(bestBeams.m_sinrPsd);

						Ptr<NetDevice> UeDevice = m_netDevice;
						m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);

						// In the first report, enable beam measurement for periodic CSI
						if (m_phyMacConfig->GetPeriodicCsiResourceAllocationCondition() == true &&
								!m_beamManagement->GetBeamReportingEnabledCondition())
						{
							m_beamManagement->FindBeamPairCandidates();	// Create initial list of beams to track
//							m_beamManagement->EnableBeamReporting();	//Enable beam reporting
							m_beamManagement->ConfigureBeamReporting(); // Configures and enables beam reporting
							m_beamManagement->NotifyBeamPairCandidatesToPeer(UeDevice);	// Notify list to peer
							// Schedule periodic CSI measurements
							Time CsiReporting = MilliSeconds(m_beamManagement->GetBeamReportingPeriod());
//							std::cout << "CSI periodic resource allocation time: " << CsiReporting.GetMilliSeconds() << std::endl;
							Simulator::Schedule(CsiReporting,&MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine,this);
						}

						//TODO: Attach to the eNB if it is a different one
					}
					// Reset counter: this is now scheduled every SS burst set period. See MmWaveUePhy::DoInitialize()
	//				m_beamManagement->ResetNumBlocksSinceLastBeamSweepUpdate();
				}

			}

			break;
		}


	}


	// Schedule reception of the next SS block transmission
//	Time slotPeriod = NanoSeconds (1000.0*m_phyMacConfig->GetSlotPeriod());
//	Simulator::Schedule (slotPeriod, &MmWaveUePhy::StartSsBlockSlot, this);
//	Time Period = m_beamManagement->GetNextSsBlockTransmissionTime(m_phyMacConfig,m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate()); //m_currentSsBlockSlotId
	m_beamManagement->IncreaseNumBlocksSinceLastBeamSweepUpdate();
//	if(m_beamManagement->IncreaseNumBlocksSinceLastBeamSweepUpdate() >= m_phyMacConfig->GetSsBlockPatternLength())
//	{
//		std::cout << m_beamManagement->GetNumBlocksSinceLastBeamSweepUpdate() << std::endl;
//	}
	Simulator::Schedule (Period, &MmWaveUePhy::StartSsBlockSlot, this);

//	// Get the next slot ID for the SS Block slot condition determination
//	m_phyMacConfig->IncreaseCurrentSsSlotId();
}


uint32_t
MmWaveUePhy::GetSubframeNumber (void)
{
	return m_slotNum;
}

void
MmWaveUePhy::PhyDataPacketReceived (Ptr<Packet> p)
{
  Simulator::ScheduleWithContext (m_netDevice->GetNode()->GetId(),
                                  MicroSeconds(m_phyMacConfig->GetTbDecodeLatency()),
                                  &MmWaveUePhySapUser::ReceivePhyPdu,
                                  m_phySapUser,
                                  p);
//	m_phySapUser->ReceivePhyPdu (p);
}

void
MmWaveUePhy::SendDataChannels (Ptr<PacketBurst> pb, std::list<Ptr<MmWaveControlMessage> > ctrlMsg, Time duration, uint8_t slotInd)
{

	//Ptr<AntennaArrayModel> antennaArray = DynamicCast<AntennaArrayModel> (GetDlSpectrumPhy ()->GetRxAntenna());
	/* set beamforming vector;
	 * for UE, you can choose 16 antenna with 0-7 sectors, or 4 antenna with 0-3 sectors
	 * input is (sector, antenna number)
	 *
	 * */
	//antennaArray->SetSector (3,16);

	if (pb->GetNPackets() > 0)
	{
		LteRadioBearerTag tag;
		if(!pb->GetPackets().front()->PeekPacketTag (tag))
		{
			NS_FATAL_ERROR ("No radio bearer tag");
		}
	}

//	Ptr<MultiModelSpectrumChannel> pChannel = DynamicCast<MultiModelSpectrumChannel>(m_downlinkSpectrumPhy->GetSpectrumChannel());
//	Ptr<MmWaveSpectrumPhy> sphy = GetObject<MmWaveSpectrumPhy>();
	m_downlinkSpectrumPhy->StartTxDataFrames (pb, ctrlMsg, duration, slotInd);
}

void
MmWaveUePhy::SendCtrlChannels (std::list<Ptr<MmWaveControlMessage> > ctrlMsg, Time prd)
{
	m_downlinkSpectrumPhy->StartTxDlControlFrames(ctrlMsg,prd);
}


uint32_t
MmWaveUePhy::GetAbsoluteSubframeNo ()
{
	return ((m_frameNum-1)*8 + m_slotNum);
}

Ptr<MmWaveDlCqiMessage>
MmWaveUePhy::CreateDlCqiFeedbackMessage (const SpectrumValue& sinr)
{
	if (!m_amc)
	{
		m_amc = CreateObject <MmWaveAmc> (m_phyMacConfig);
	}
	NS_LOG_FUNCTION (this);
	SpectrumValue newSinr = sinr;
	//SpectrumValue newSinr = m_beamManagement->GetBestScannedBeamPair().m_sinrPsd;
	// CREATE DlCqiLteControlMessage
	Ptr<MmWaveDlCqiMessage> msg = Create<MmWaveDlCqiMessage> ();
	DlCqiInfo dlcqi;

	dlcqi.m_rnti = m_rnti;
	dlcqi.m_cqiType = DlCqiInfo::WB;

	std::vector<int> cqi;

	//uint8_t dlBandwidth = m_phyMacConfig->GetNumChunkPerRb () * m_phyMacConfig->GetNumRb ();
	NS_ASSERT (m_currSlot.m_dci.m_format==0);
	int mcs;
	dlcqi.m_wbCqi = m_amc->CreateCqiFeedbackWbTdma (newSinr, m_currSlot.m_dci.m_numSym, m_currSlot.m_dci.m_tbSize, mcs);

//	int activeSubChannels = newSinr.GetSpectrumModel()->GetNumBands ();
	/*cqi = m_amc->CreateCqiFeedbacksTdma (newSinr, m_currNumSym);
	int nbSubChannels = cqi.size ();
	double cqiSum = 0.0;
	// average the CQIs of the different RBs
	for (int i = 0; i < nbSubChannels; i++)
	{
		if (cqi.at (i) != -1)
		{
			cqiSum += cqi.at (i);
			activeSubChannels++;
		}
//		NS_LOG_DEBUG (this << " subch " << i << " cqi " <<  cqi.at (i));
	}*/
//	if (activeSubChannels > 0)
//	{
//		dlcqi.m_wbCqi = ((uint16_t) cqiSum / activeSubChannels);
//	}
//	else
//	{
//		// approximate with the worst case -> CQI = 1
//		dlcqi.m_wbCqi = 1;
//	}
	msg->SetDlCqi (dlcqi);
	return msg;
}

void
MmWaveUePhy::GenerateDlCqiReport (const SpectrumValue& sinr)
{
	if(m_ulConfigured && (m_rnti > 0) && m_receptionEnabled)
	{
		if (Simulator::Now () > m_wbCqiLast + m_wbCqiPeriod)
		{
			SpectrumValue newSinr = sinr;
			//SpectrumValue newSinr = m_beamManagement->GetBestScannedBeamPair().m_sinrPsd;
			Ptr<MmWaveDlCqiMessage> msg = CreateDlCqiFeedbackMessage (newSinr);

			if (msg)
			{
				DoSendControlMessage (msg);
			}
			Ptr<MmWaveUeNetDevice> UeRx = DynamicCast<MmWaveUeNetDevice> (GetDevice());
			m_reportCurrentCellRsrpSinrTrace (UeRx->GetImsi(), newSinr, newSinr);
		}
	}
}

void
MmWaveUePhy::ReceiveLteDlHarqFeedback (DlHarqInfo m)
{
  NS_LOG_FUNCTION (this);
  // generate feedback to eNB and send it through ideal PUCCH
  Ptr<MmWaveDlHarqFeedbackMessage> msg = Create<MmWaveDlHarqFeedbackMessage> ();
  msg->SetDlHarqFeedback (m);
  Simulator::Schedule (MicroSeconds(m_phyMacConfig->GetTbDecodeLatency()), &MmWaveUePhy::DoSendControlMessage, this, msg);
//  if (m.m_harqStatus == DlHarqInfo::NACK)  // Notify MAC/RLC
//  {
//  	m_phySapUser->NotifyHarqDeliveryFailure (m.m_harqProcessId);
//  }
}

bool
MmWaveUePhy::IsReceptionEnabled ()
{
	return m_receptionEnabled;
}

void
MmWaveUePhy::ResetReception()
{
	m_receptionEnabled = false;
}

uint16_t
MmWaveUePhy::GetRnti ()
{
	return m_rnti;
}


void
MmWaveUePhy::DoReset ()
{
	NS_LOG_FUNCTION (this);
}

void
MmWaveUePhy::DoStartCellSearch (uint16_t dlEarfcn)
{
	NS_LOG_FUNCTION (this << dlEarfcn);
}

void
MmWaveUePhy::DoSynchronizeWithEnb (uint16_t cellId, uint16_t dlEarfcn)
{
	NS_LOG_FUNCTION (this << cellId << dlEarfcn);
	DoSynchronizeWithEnb (cellId);
}

void
MmWaveUePhy::DoSetPa (double pa)
{
  NS_LOG_FUNCTION (this << pa);
}


void
MmWaveUePhy::DoSynchronizeWithEnb (uint16_t cellId)
{
	NS_LOG_FUNCTION (this << cellId);
	if (cellId == 0)
	{
		NS_FATAL_ERROR ("Cell ID shall not be zero");
	}
}

void
MmWaveUePhy::DoSetDlBandwidth (uint8_t dlBandwidth)
{
	NS_LOG_FUNCTION (this << (uint32_t) dlBandwidth);
}


void
MmWaveUePhy::DoConfigureUplink (uint16_t ulEarfcn, uint8_t ulBandwidth)
{
	NS_LOG_FUNCTION (this << ulEarfcn << ulBandwidth);
  m_ulConfigured = true;
}

void
MmWaveUePhy::DoConfigureReferenceSignalPower (int8_t referenceSignalPower)
{
	NS_LOG_FUNCTION (this << referenceSignalPower);
}

void
MmWaveUePhy::DoSetRnti (uint16_t rnti)
{
	NS_LOG_FUNCTION (this << rnti);
	m_rnti = rnti;
}

void
MmWaveUePhy::DoSetTransmissionMode (uint8_t txMode)
{
	NS_LOG_FUNCTION (this << (uint16_t)txMode);
}

void
MmWaveUePhy::DoSetSrsConfigurationIndex (uint16_t srcCi)
{
	NS_LOG_FUNCTION (this << srcCi);
}

void
MmWaveUePhy::SetPhySapUser (MmWaveUePhySapUser* ptr)
{
	m_phySapUser = ptr;
}

void
MmWaveUePhy::SetHarqPhyModule (Ptr<MmWaveHarqPhy> harq)
{
  m_harqPhyModule = harq;
}


void
MmWaveUePhy::GetBeamGain()
{
	Ptr<MobilityModel> mm = m_netDevice->GetNode()->GetObject<MobilityModel>();
	if(!mm)
	{
		std::cout << "Not found" << std::endl;
	}


	//std::vector<MmWaveEnbNetDevice> p = DynamicCast <std::vector<MmWaveEnbNetDevice>>(m_enbNetDevicesList);
	for(NetDeviceContainer::Iterator it = m_enbNetDevicesList.Begin(); it != m_enbNetDevicesList.End(); ++it)
	{
		Ptr<NetDevice> node = *it;
		Ptr<MmWaveEnbNetDevice> mmNode = DynamicCast<MmWaveEnbNetDevice>(node);
		Ptr<MobilityModel> m = mmNode->GetNode()->GetObject<MobilityModel>();//GetObject<MobilityModel>();
		Ptr<MmWaveEnbPhy> p = mmNode->GetPhy();
//		Ptr<MmWaveBeamManagement> mng = p->GetBeamManagement();

		//TODO: To search the bfGain of the current iteration
//		std::vector<int> listOfSubchannels;
//		for (unsigned i = 0; i < m_phyMacConfig->GetTotalNumChunk(); i++)
//		{
//			listOfSubchannels.push_back(i);
//		}
//		Ptr<SpectrumValue> fakePsd =
//				MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (m_phyMacConfig, 0, listOfSubchannels);

		// Look for the Channel (MmWaveBeamforming) along the classes
		Ptr<MmWaveSpectrumPhy> pSpecPhy = p->GetDlSpectrumPhy();
		Ptr<SpectrumChannel> pChannel = pSpecPhy->GetSpectrumChannel();
		Ptr<MultiModelSpectrumChannel> pMmodel = DynamicCast<MultiModelSpectrumChannel>(pChannel);
		Ptr<SpectrumPropagationLossModel> pModel = pMmodel->GetSpectrumPropagationLossModel();//pChannel->GetObject<SpectrumPropagationLossModel>();
		Ptr<MmWaveBeamforming> bf = DynamicCast<MmWaveBeamforming>(pModel);
		Ptr<MmWave3gppChannel> p3gpp = DynamicCast<MmWave3gppChannel>(pModel);
		Ptr<MmWaveChannelRaytracing> pRaytracing = DynamicCast<MmWaveChannelRaytracing>(pModel);
		if (bf)
		{
			bf->SetBeamSweepingVector(m_netDevice,node);
		}
		else if (p3gpp)
		{
			p3gpp->SetBeamSweepingVector(m_netDevice,node);
		}
		else if (pRaytracing)
		{
			pRaytracing->SetBeamSweepingVector(m_netDevice,node);
		}
		else
		{
			std::cout << "ERROR" << std::endl;
		}
	}

}

void MmWaveUePhy::UpdateChannelMap()
{
	BeamPairInfoStruct bestBeams = m_beamManagement->GetBestScannedBeamPair();
	UpdateChannelMapWithBeamPair (bestBeams);
}


void MmWaveUePhy::UpdateChannelMapWithBeamPair (BeamPairInfoStruct beamPairInfo)
{
	Ptr<MobilityModel> mm = m_netDevice->GetNode()->GetObject<MobilityModel>();
	if(!mm)
	{
		std::cout << "Not found at MmWaveUePhy::UpdateChannelMap()" << std::endl;
	}

	Ptr<NetDevice> enbNetDevice = beamPairInfo.m_targetNetDevice;
	if (beamPairInfo.m_targetNetDevice == NULL)
	{
		std::cout << "bestBeams.m_targetNetDevice is null and it should not be!" << std::endl;
	}
	Ptr<NetDevice> ueNetDevice = m_netDevice;

	Ptr<MmWaveEnbNetDevice> pMmEnbNetDevice = DynamicCast<MmWaveEnbNetDevice>(enbNetDevice);
	Ptr<MmWaveEnbPhy> pPhyEnb = pMmEnbNetDevice->GetPhy();
	Ptr<MmWaveSpectrumPhy> pSpecPhy = pPhyEnb->GetDlSpectrumPhy();
	Ptr<SpectrumChannel> pChannel = pSpecPhy->GetSpectrumChannel();
	Ptr<MultiModelSpectrumChannel> pMmodel = DynamicCast<MultiModelSpectrumChannel>(pChannel);
	Ptr<SpectrumPropagationLossModel> pModel = pMmodel->GetSpectrumPropagationLossModel();//pChannel->GetObject<SpectrumPropagationLossModel>();
	Ptr<MmWaveBeamforming> bf = DynamicCast<MmWaveBeamforming>(pModel);
	Ptr<MmWave3gppChannel> p3gpp = DynamicCast<MmWave3gppChannel>(pModel);
	Ptr<MmWaveChannelRaytracing> pRaytracing = DynamicCast<MmWaveChannelRaytracing>(pModel);
	if(bf)
	{
		bf->UpdateBfChannelMatrix(ueNetDevice, enbNetDevice, beamPairInfo);
	}
	else if (p3gpp)
	{
		p3gpp->UpdateBfChannelMatrix(ueNetDevice, enbNetDevice, beamPairInfo);
	}
	else if (pRaytracing)
	{
		pRaytracing->UpdateBfChannelMatrix(ueNetDevice, enbNetDevice, beamPairInfo);
	}
	else
	{
		std::cout << "ERROR" << std::endl;
	}
}


Ptr<MmWaveBeamManagement> MmWaveUePhy::GetBeamManagement ()
{
	return m_beamManagement;
}


void MmWaveUePhy::SetCurrentState (State s)
{
	m_currentState = s;
}

MmWaveUePhy::State MmWaveUePhy::GetCurrentState ()
{
	return m_currentState;
}


void
MmWaveUePhy::AttachToSelectedEnb (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice)
{
	NS_LOG_FUNCTION (this);

	uint16_t cellId = enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetCellId ();
	Ptr<MmWavePhyMacCommon> configParams = enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetPhy()->GetConfigurationParameters();

	enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetPhy ()->AddUePhy (ueDevice->GetObject<MmWaveUeNetDevice> ()->GetImsi (), ueDevice);
	ueDevice->GetObject<MmWaveUeNetDevice> ()->GetPhy ()->RegisterToEnb (cellId, configParams);
	enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetMac ()->AssociateUeMAC (ueDevice->GetObject<MmWaveUeNetDevice> ()->GetImsi ());

	Ptr<EpcUeNas> ueNas = ueDevice->GetObject<MmWaveUeNetDevice> ()->GetNas ();
	ueNas->Connect (enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetCellId (),
			enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetEarfcn ());

//	if (m_epcHelper != 0)
//	{
//		// activate default EPS bearer
//		m_epcHelper->ActivateEpsBearer (ueDevice, ueDevice->GetObject<MmWaveUeNetDevice> ()->GetImsi (), EpcTft::Default (), EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT));
//	}

	// tricks needed for the simplified LTE-only simulations
	//if (m_epcHelper == 0)
	//{
	ueDevice->GetObject<MmWaveUeNetDevice> ()->SetTargetEnb (enbDevice->GetObject<MmWaveEnbNetDevice> ());
	//}

//	enbDevice->GetObject<MmWaveEnbNetDevice> ()->GetPhy ()->GetDlSpectrumPhy()->UpdateSinrPerceived(
//			ueDevice->GetObject<MmWaveUeNetDevice> ()->GetPhy()->GetBeamManagement()->GetBestScannedBeamPair().m_sinrPsd);
}

void MmWaveUePhy::SetBeamManagement (Ptr<MmWaveBeamManagement> beamMng)
{
	m_beamManagement = beamMng;
}

void
MmWaveUePhy::GetBeamGainForCsi()
{
	Ptr<MobilityModel> mm = m_netDevice->GetNode()->GetObject<MobilityModel>();
		if(!mm)
		{
			std::cout << "Not found" << std::endl;
		}

		for(NetDeviceContainer::Iterator it = m_enbNetDevicesList.Begin(); it != m_enbNetDevicesList.End(); ++it)
		{
			Ptr<NetDevice> enb = *it;
			GetBeamGainForCsi(enb);
		}
}


/*
 * This method perform the beam measurements indicated in the list of beams to monitor. Calling this function implies
 * updating the entries in the channel information map of the measured pair of beams.
 */
void
MmWaveUePhy::GetBeamGainForCsi(Ptr<NetDevice> enb)
{
	Ptr<MobilityModel> mm = m_netDevice->GetNode()->GetObject<MobilityModel>();
	if(!mm)
	{
		std::cout << "Not found" << std::endl;
	}

	BeamTrackingParams BeamPairs = m_beamManagement->GetBeamsToTrackForEnb(enb);
	if (BeamPairs.m_numBeamPairs == 0)	// When simulating in PC cluster, this call interrupts the simulation.
	{
		return;
	}

	// Get gNB Beam Manager from PHY
	Ptr<MmWaveEnbNetDevice> gnbNetDevice = DynamicCast<MmWaveEnbNetDevice>(enb);
	Ptr<MmWaveEnbPhy> enbPhy = gnbNetDevice->GetPhy();
	Ptr<MmWaveBeamManagement> enbBeamMng = enbPhy->GetBeamManagement();

	// Look for the channel pointer along the classes
	Ptr<MmWaveSpectrumPhy> pSpecPhy = enbPhy->GetDlSpectrumPhy();
	Ptr<SpectrumChannel> pChannel = pSpecPhy->GetSpectrumChannel();
	Ptr<MultiModelSpectrumChannel> pMmodel = DynamicCast<MultiModelSpectrumChannel>(pChannel);
	Ptr<SpectrumPropagationLossModel> pModel = pMmodel->GetSpectrumPropagationLossModel();//pChannel->GetObject<SpectrumPropagationLossModel>();
	Ptr<MmWaveBeamforming> bf = DynamicCast<MmWaveBeamforming>(pModel);
	Ptr<MmWave3gppChannel> p3gpp = DynamicCast<MmWave3gppChannel>(pModel);
	Ptr<MmWaveChannelRaytracing> pRaytracing = DynamicCast<MmWaveChannelRaytracing>(pModel);

	SpectrumValue sinr;
//		double currentAvgSinr = -1;
//		BeamPairInfoStruct bestCandidatePair;
	Time csiTime = Simulator::Now();
	BeamPairs.m_csiResourceLastAllocation = csiTime;
	for (uint8_t i = 0; i < BeamPairs.m_numBeamPairs; i++)
	{
		complexVector_t beamformingTx =
				enbBeamMng->GetBeamSweepVector(BeamPairs.m_beamPairList.at(i).m_txBeamId);
		complexVector_t beamformingRx =
				m_beamManagement->GetBeamSweepVector(BeamPairs.m_beamPairList.at(i).m_rxBeamId);

		if (bf)
		{
			std::cout << "ERROR. Not implemented for beamforming channel class" << std::endl;
//				bf->SetBeamSweepingVector(m_netDevice,enb);
		}
		else if (p3gpp)
		{
			sinr = p3gpp->GetSinrForBeamPairs(m_netDevice,enb,beamformingTx,beamformingRx);
		}
		else if (pRaytracing)
		{
			sinr = pRaytracing->GetSinrForBeamPairs(m_netDevice,enb,beamformingTx,beamformingRx);
		}
		else
		{
			std::cout << "ERROR" << std::endl;
		}
		BeamPairs.m_beamPairList.at(i).m_sinrPsd = sinr;
		uint8_t nbands = sinr.GetSpectrumModel ()->GetNumBands ();
		BeamPairs.m_beamPairList.at(i).m_avgSinr = Sum(sinr)/nbands;

		// Now add the experienced SINR to the beam management node (optional)
		m_beamManagement->AddEnbSinr(
				enb,
				BeamPairs.m_beamPairList.at(i).m_txBeamId,
				BeamPairs.m_beamPairList.at(i).m_rxBeamId,
				sinr);

//			// Get the index of the beam candidates vector to check if the best sinr is achieved with a different beam
//			if (BeamPairs.m_beamPairList.at(i).m_avgSinr > currentAvgSinr)
//			{
//				currentAvgSinr = BeamPairs.m_beamPairList.at(i).m_avgSinr;
//				bestCandidatePair = BeamPairs.m_beamPairList.at(i);
//			}

	} //End of loop for beam candidate info update

//		//Program change of beams if needed (depending on the architecture: one rx beam in the analog bf per symbol
	// MOVED TO PERIODIC CSI ROUTINE
//		BeamPairInfoStruct bestBeams = m_beamManagement->GetBestScannedBeamPair();
//		if(bestBeams.m_txBeamId != bestCandidatePair.m_txBeamId || bestBeams.m_rxBeamId != bestCandidatePair.m_rxBeamId)
//		{
//			m_bestTxBeamId = bestCandidatePair.m_txBeamId;
//			m_bestRxBeamId = bestCandidatePair.m_rxBeamId;
//			m_beamManagement->SetBestScannedEnb(bestCandidatePair);	//Needed before calling UpdateChannelMap
//
//			Architecture phyArch = GetPhyArchitecture();
//			if(phyArch == Digital)
//			{
//				UpdateChannelMap();
//			}
//			// If analog we introduce some delay in the update
//			else if (phyArch == Analog || phyArch == Analog_fast)
//			{
//				//FIXME: Not 2 times the symbol period
//				Simulator::Schedule(MicroSeconds(2*m_beamManagement->GetBeamReportingPeriod()),&MmWaveUePhy::UpdateChannelMap,this);
//			}
//			else
//			{
//				std::cout << "ERROR at MmWaveUePhy::GetBeamGainForCsi(): Unsupported architecture" << std::endl;
//			}
//		}
//
	// Update beam tracking information
	m_beamManagement->UpdateBeamTrackingInfoValues(enb,BeamPairs);

}



void
MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine ()
//MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine (Ptr<MmWaveBeamManagement> pExtManager)
{
	for(NetDeviceContainer::Iterator it = m_enbNetDevicesList.Begin(); it != m_enbNetDevicesList.End(); ++it)
	{
		Ptr<NetDevice> enb = *it;

//		BeamTrackingParams candidateBeamsInfoBefore = m_beamManagement->GetBeamsToTrackForEnb(enb);

		// Get the beam gains
		GetBeamGainForCsi(enb);

		BeamTrackingParams candidateBeamsInfoUpdated = m_beamManagement->GetBeamsToTrackForEnb(enb);

		/*
		 * Determine if there is a new best pair of beams. If so, update channel matrix. Analog bf has some delay
		 */
		BeamPairInfoStruct bestBeams = m_beamManagement->GetBestScannedBeamPair();
		BeamPairInfoStruct bestCandidatePair;
		double sinr = -1;
		Architecture phyArch = GetPhyArchitecture();
		Time analogDelayTime = MicroSeconds(2*m_beamManagement->GetBeamReportingPeriod());
//		std::cout << "CSI update: Time = " << Simulator::Now().GetSeconds() << std::endl;
//		NS_LOG_INFO("CSI update: Time = " << Simulator::Now().GetSeconds());
		bool found = false;	//This flag determines whether or not the active beam pair is in the list of candidate beams
		for (unsigned pos = 0; pos < candidateBeamsInfoUpdated.m_beamPairList.size(); pos++)
		{
			if(candidateBeamsInfoUpdated.m_beamPairList.at(pos).m_avgSinr > sinr)
			{
				bestCandidatePair = candidateBeamsInfoUpdated.m_beamPairList.at(pos);
				sinr = candidateBeamsInfoUpdated.m_beamPairList.at(pos).m_avgSinr;
			}
			if(bestBeams.m_txBeamId == bestCandidatePair.m_txBeamId && bestBeams.m_rxBeamId == bestCandidatePair.m_rxBeamId)
			{
				found = true;
			}
		}

		// Case of detecting a change in the best pair of beams
//		if(bestBeams.m_txBeamId != bestCandidatePair.m_txBeamId || bestBeams.m_rxBeamId != bestCandidatePair.m_rxBeamId)
		if(m_bestTxBeamId != bestCandidatePair.m_txBeamId || m_bestRxBeamId != bestCandidatePair.m_rxBeamId)
		{
			bestBeams.m_txBeamId = m_bestTxBeamId;
			bestBeams.m_rxBeamId = m_bestRxBeamId;
			BeamPairInfoStruct reportBeamPair = bestBeams;
			if (found == true)
			{
				m_bestTxBeamId = bestCandidatePair.m_txBeamId;
				m_bestRxBeamId = bestCandidatePair.m_rxBeamId;
				m_beamManagement->SetBestScannedEnb(bestCandidatePair);	//Needed before calling UpdateChannelMap
				reportBeamPair = bestCandidatePair;
				std::cout << "[" << Simulator::Now().GetSeconds() <<"]Best beam pair update: tx=" << m_bestTxBeamId <<
								" rx=" << m_bestRxBeamId << " avgSinr=" << bestCandidatePair.m_avgSinr << " (CSI)" <<
								std::endl;
				// Before concluding, since the best pair of beams has changed, the list of beams to monitor must change too
	//			BeamTrackingParams beforeCall = m_beamManagement->GetBeamsToTrack(); //TEST
				m_beamManagement->FindBeamPairCandidates();
	//			BeamTrackingParams afterCall = m_beamManagement->GetBeamsToTrack(); //TEST
			}
			// UpdateChannelMap calls for changing the transmit and receive beams
			if(phyArch == Digital)
			{
				UpdateChannelMapWithBeamPair(reportBeamPair);
			}
			// If analog we introduce some delay in the update
			else if (phyArch == Analog || phyArch == Analog_fast)
			{
				//FIXME: Not 2 times the symbol period
				Simulator::Schedule(analogDelayTime,&MmWaveUePhy::UpdateChannelMapWithBeamPair,this,reportBeamPair);
			}
			else
			{
				std::cout << "ERROR at MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine(): Unsupported architecture" << std::endl;
			}

		}

		// FIXME: update here is disabled because it is done before at GetBeamGainsForCsi method.
		// Not sure of the last line, GetBeamGainsForCsi only updates the SINR values of the list of beam pairs to track.
		// I need to update the list somewhere again, I think here is the best place, before the peer has been notified about the new list around the best beam pairs
		// Check that the list is really updated around the updated best pair of beams.
		// Testing creating a new list if best beam changed (line 1559) m_beamManagement->FindBeamPairCandidates();

		// Update and notify beam tracking information (even if the beam ids didn't change)
		if(phyArch == Digital)
		{
			m_beamManagement->NotifyBeamPairCandidatesToPeer(m_netDevice);
		}
		else if (phyArch == Analog || phyArch == Analog_fast)
		{
			Simulator::Schedule(analogDelayTime+NanoSeconds(1.0),&MmWaveBeamManagement::NotifyBeamPairCandidatesToPeer,m_beamManagement,m_netDevice);
			m_beamManagement->NotifyBeamPairCandidatesToPeer(m_netDevice);
		}
		else
		{
			std::cout << "ERROR at MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine(): Unsupported architecture" << std::endl;
		}

		// Program next call to the routine after Xp ms
		Simulator::Schedule(MilliSeconds(m_phyMacConfig->GetCsiReportPeriodicity()),&MmWaveUePhy::AcquaringPeriodicCsiValuesRoutine,this);
	}
}


}
