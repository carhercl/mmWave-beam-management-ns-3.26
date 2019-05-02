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



#ifndef SRC_MMWAVE_MODEL_MMWAVE_PHY_MAC_COMMON_H
#define SRC_MMWAVE_MODEL_MMWAVE_PHY_MAC_COMMON_H

#include <vector>
#include <list>
#include <map>
#include <deque>
#include <ns3/object.h>
#include <ns3/packet.h>
#include <ns3/string.h>
#include <ns3/log.h>
//#include "mmwave-mac-pdu-header.h"
//#include "mmwave-mac-pdu-tag.h"


namespace ns3 {

// The slot indexes in which an SS burst block is allocated in
static const std::vector<uint16_t> ssBurstSetPattern_scs60Khz = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
//static const std::vector<uint16_t> ssBurstSetPattern_scs120Khz =
//{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38};
static const std::vector<uint16_t> ssBurstSetPattern_scs120Khz =
{4, 8, 16, 20, 32, 36, 44, 48, 60, 64, 72, 76, 88, 92, 100, 104, 144, 148, 156, 160, 172, 176, 184, 188, 200, 204, 212, 216, 228, 232, 240, 244, 284, 288, 296, 300, 312, 316, 324, 328, 340, 344, 352, 356, 368, 372, 380, 384, 424, 428, 436, 440, 452, 456, 464, 468, 480, 484, 492, 496, 508, 512, 520, 524};
//static const std::vector<uint16_t> ssBurstSetPattern_scs240Khz =
//{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34};
static const std::vector<uint16_t> ssBurstSetPattern_scs240Khz =
{8, 12, 16, 20, 32, 36, 40, 44, 64, 68, 72, 76, 88, 92, 96, 100, 120, 124, 128, 132, 144, 148, 152, 156, 176, 180, 184, 188, 200, 204, 208, 212, 288, 292, 296, 300, 312, 316, 320, 324, 344, 348, 352, 356, 368, 372, 376, 380, 400, 404, 408, 412, 424, 428, 432, 436, 456, 460, 464, 468, 480, 484, 488, 492};
//static const std::vector<uint16_t> csiReportPeriodicity = {5,10,20,40,80,160,320};


struct SfnSf
{
	SfnSf (): m_frameNum (0), m_sfNum (0), m_slotNum (0)
	{
	}

	SfnSf (uint16_t frameNum, uint8_t sfNum, uint8_t slotNum)
		: m_frameNum (frameNum), m_sfNum (sfNum), m_slotNum (slotNum)
	{
	}

	uint32_t
	Encode () const
	{
		return ((m_frameNum << 16) | ((m_sfNum & 0xFF) << 8) | (m_slotNum & 0xFF));
	}

	void
	Decode (uint32_t sfn)
	{
		m_frameNum = sfn >> 16;
		m_sfNum = ((sfn & 0xFF00) >> 8);
		m_slotNum = sfn & 0xFF;
	}

	uint16_t m_frameNum;
	uint8_t m_sfNum;
	uint8_t m_slotNum;			 // also used for symbol start index in some cases
};

/* Equivalent to the DCI in LTE*/

//struct RbAllocationInfo
//{
//	bool m_noAllocation;
//	uint64_t m_userImsi;
//	uint32_t m_tbSize;
//	int mcs;
//
//	uint8_t m_harqProcess;
//};

struct TbInfoElement
{
	TbInfoElement () : m_isUplink (0), m_slotIdx (0), m_rbBitmap (0), m_rbShift (0), m_rbStart (0), m_rbLen (0),
			m_symStart (0), m_numSym (0), m_resAlloc (0), m_mcs (0), m_tbSize (0), m_ndi (0), m_rv (0), m_harqProcess (0)
	{
	}

	bool 			m_isUplink;		// is uplink grant?
	uint8_t   m_slotIdx;		// slot index
	uint32_t  m_rbBitmap;		// Resource Block Group bitmap
	uint8_t   m_rbShift;		// shift for res alloc type 1
	uint8_t		m_rbStart;		// starting RB index for uplink res alloc type 0
	uint16_t	m_rbLen;
	uint8_t		m_symStart;		// starting symbol index for flexible TTI scheme
	uint8_t		m_numSym;			// number of symbols for flexible TTI scheme
	uint8_t   m_resAlloc;		// resource allocation type
	uint8_t		m_mcs;
	uint32_t	m_tbSize;
	uint8_t		m_ndi;
	uint8_t		m_rv;
	uint8_t   m_harqProcess;
};

struct DlDciInfoElementTdma
{
	DlDciInfoElementTdma () :
			m_symStart (0), m_numSym (0), m_mcs (0),
			m_tbSize (0), m_ndi (0), m_rv (0), m_harqProcess (0)
	{
	}

	uint8_t		m_symStart;		// starting symbol index for flexible TTI scheme
	uint8_t		m_numSym;			// number of symbols for flexible TTI scheme
	uint8_t		m_mcs;
	uint32_t	m_tbSize;
	uint8_t		m_ndi;
	uint8_t		m_rv;
	uint8_t   m_harqProcess;
};

struct DciInfoElementTdma
{
	enum DciFormat { DL = 0, UL = 1 };

	DciInfoElementTdma () :
			m_rnti (0), m_format (0), m_symStart (0), m_numSym (0), m_mcs (0),
			m_tbSize (0), m_ndi (0), m_rv (0), m_harqProcess (0)
	{
	}

	DciInfoElementTdma (uint16_t 	rnti, uint8_t format, uint8_t symStart, uint8_t numSym, uint8_t mcs,
	                    uint32_t tbs, uint8_t ndi, uint8_t rv, uint8_t harqProc) :
	                    	m_rnti (rnti), m_format (format), m_symStart (symStart), m_numSym (numSym), m_mcs (mcs),
	                    	m_tbSize (tbs), m_ndi (ndi), m_rv (rv), m_harqProcess (harqProc)
	{
	}

	uint16_t 	m_rnti;
	uint8_t		m_format;			// {DL assig. = 0, UL grant = 1}, only contiguous symbols supported
	uint16_t	m_symStart;		// starting symbol index for flexible TTI scheme
	uint16_t	m_numSym;			// number of symbols for flexible TTI scheme
	uint8_t		m_mcs;
	uint32_t	m_tbSize;
	uint8_t		m_ndi;
	uint8_t		m_rv;					// not used for UL DCI
	uint8_t   m_harqProcess;
};

struct TbAllocInfo
{
	TbAllocInfo () :
		m_rnti (0)
	{

	}
	struct SfnSf m_sfnSf;
	uint16_t m_rnti;
	std::vector<unsigned> m_rbMap;
	TbInfoElement m_tbInfo;
};

struct DciInfoElement
{
	DciInfoElement () :
		m_rnti (0), m_cceIndex (0), m_format (0), m_tddBitmap (0)
	{
	}

	uint16_t  m_rnti;
	uint8_t   m_cceIndex;
	uint8_t		m_format;			// to support different DCI types
	uint16_t	m_tddBitmap;	// 0 == DL, 1 == UL
	std::vector<TbInfoElement> m_tbInfoElements;
};

struct RlcPduInfo
{
	RlcPduInfo (): m_lcid (0), m_size (0)
	{
	}

	RlcPduInfo (uint8_t lcid, uint16_t size) :
		m_lcid (lcid), m_size (size)
	{
	}
  uint8_t   m_lcid;
  uint32_t  m_size;
};

struct SlotAllocInfo
{
	enum TddMode
	{
		NA = 0,
		DL = 1,
		UL = 2,
	};

	enum TddSlotType
	{
		CTRL_DATA = 0,
		DATA = 1,
		CTRL = 2
	};

	enum CtrlTxMode
	{
		ANALOG = 0,
		DIGITAL = 1,
		OMNI = 2
	};

	SlotAllocInfo () : m_tddMode(NA), m_isOmni (0), m_slotType (CTRL_DATA),
			m_numCtrlSym(0), m_slotIdx (0), m_ctrlTxMode (DIGITAL), m_rnti (0)
	{
	}

	SlotAllocInfo (uint8_t slotIdx, TddMode tddMode, TddSlotType slotType, CtrlTxMode ctrlTxMode, uint16_t rnti) :
		m_tddMode(tddMode), m_isOmni (0), m_slotType (slotType),
		m_numCtrlSym(0), m_slotIdx (slotIdx), m_ctrlTxMode (ctrlTxMode), m_rnti (rnti)
	{
	}

//	SlotAllocInfo& operator= (const SlotAllocInfo &src)
//	{
//		m_tddMode = src.m_tddMode;
//		m_isOmni = src.m_isOmni;
//		m_slotType = src.m_slotType;
//		m_numCtrlSym = src.m_numCtrlSym;
//		m_slotIdx = src.m_slotIdx;
//		m_ctrlTxMode = src.m_ctrlTxMode;
//		m_rnti = src.m_rnti;
//		m_dci = src.m_dci;
//		m_rlcPduInfo = src.m_rlcPduInfo;
//	}

	//std::vector<TbAllocInfo> m_tbInfo;
	//std::vector<unsigned> m_rntiCtrlSymMap;		// RNTI to which ctrl data is TXed for each ctrl symbol index (analog bf only)
	//std::map<uint16_t, std::vector<unsigned> > m_ueRbMap; // for FDMA

	//struct DciInfoElement m_dci;
	//std::vector<TbAllocInfo> m_tbInfo;
	//std::vector<unsigned> m_rntiCtrlSymMap;		// RNTI to which ctrl data is TXed for each ctrl symbol index (analog bf only)
  //std::vector<struct RlcPduInfo >  m_rlcPduList; // RLC PDU elems for MAC TB

	TddMode m_tddMode;
	bool 		m_isOmni; 					// Beamforming disabled, true if omnidirectional
	TddSlotType m_slotType;
	uint8_t m_numCtrlSym;				// number of DL ctrl (or ctrl+data) symbols at beginning of slot
	uint8_t m_slotIdx;
	CtrlTxMode m_ctrlTxMode;
	uint16_t m_rnti;
	struct DciInfoElementTdma m_dci;
	std::vector<RlcPduInfo> m_rlcPduInfo;
	//std::list<MmWaveControlMessage> m_controlMessages;  // ctrl messages for this user
};


struct SfAllocInfo
{
	SfAllocInfo () : m_sfnSf (SfnSf()),  m_numSymAlloc (0), m_ulSymStart (0)
	{
		//m_tddPattern.resize (8);
	}

	SfAllocInfo (SfnSf sfn) : m_sfnSf (sfn), m_numSymAlloc (0), m_ulSymStart (0)
	{
	}

//	SfAllocInfo& operator= (const SfAllocInfo &src)
//	{
//		m_sfnSf = src.m_sfnSf;
//		m_numSymAlloc = src.m_numSymAlloc;
//		m_ulSymStart = src.m_ulSymStart;
//		m_dlSlotAllocInfo = src.m_dlSlotAllocInfo;
//		m_ulSlotAllocInfo = src.m_ulSlotAllocInfo;
//		return *this;
//	}

	SfnSf m_sfnSf;
	uint32_t m_numSymAlloc;  // number of allocated slots
	uint32_t m_ulSymStart;		 // start of UL region
	//std::vector <SlotAllocInfo::TddMode> m_tddPattern;
	std::deque <SlotAllocInfo> m_dlSlotAllocInfo;
	std::deque <SlotAllocInfo> m_ulSlotAllocInfo;
	std::deque <SlotAllocInfo> m_slotAllocInfo;
};


typedef std::vector<SlotAllocInfo::TddSlotType> TddSlotTypeList;


struct DlCqiInfo
{

	DlCqiInfo()
	{
		this->m_cqiType = WB;
		//this->m_rbCqi = [];
		this->m_ri = 0;
		this->m_rnti = 65000;
		this->m_wbCqi = 0;
		this->m_wbPmi = 0;
	}

  uint16_t  m_rnti;
  uint8_t   m_ri;
  enum DlCqiType
  {
    WB,SB
  } m_cqiType;
	std::vector<uint8_t> m_rbCqi; // CQI for each Rsc Block, set to -1 if SINR < Threshold
	uint8_t m_wbCqi; // Wide band CQI
	uint8_t m_wbPmi;
};


struct UlCqiInfo
{
  //std::vector <uint16_t> m_sinr;
  std::vector <double> m_sinr;
  enum UlCqiType
  {
    SRS,
    PUSCH,
    PUCCH_1,
    PUCCH_2,
    PRACH
  } m_type;
};

struct MacCeValue
{
	MacCeValue() : 
	m_phr(0), m_crnti(0)
	{
	}
  uint8_t   m_phr;
  uint8_t   m_crnti;
  std::vector <uint8_t> m_bufferStatus;
};

/**
 * \brief See section 4.3.14 macCEListElement
 */
struct MacCeElement
{
	MacCeElement() : 
	m_rnti(0)
	{
	}
  uint16_t  m_rnti;
  enum MacCeType
  {
    BSR, PHR, CRNTI
  } m_macCeType;
  struct MacCeValue m_macCeValue;
};

struct RlcListElement
{
	std::vector <struct RlcPduInfo> m_rlcPduElements;
};

struct SchedInfo
{
	SchedInfo () :
		m_frameNum (0), m_sfNum (0), m_rnti (0)
	{
	}
	SchedInfo (unsigned int numSlots) :
		m_frameNum (0), m_sfNum (0), m_rnti (0)
	{
	}

	uint16_t	m_frameNum;
	uint8_t	  m_sfNum;
  uint16_t  m_rnti;

  SfAllocInfo m_sfAllocInfo;
  struct DciInfoElement m_dci;
  std::map<uint8_t, std::vector<struct RlcPduInfo > > m_rlcPduMap; // RLC PDU elems for each MAC TB
};

struct UePhyPacketCountParameter
{
	uint64_t m_imsi;
	uint32_t m_noBytes;
	bool m_isTx; //Set to false if Rx and true if tx
	uint32_t m_subframeno;
};


struct EnbPhyPacketCountParameter
{
	uint64_t m_cellId;
	uint32_t m_noBytes;
	bool m_isTx; //Set to false if Rx and true if tx
	uint32_t m_subframeno;
};

struct RxPacketTraceParams
{
	uint64_t m_cellId;
	uint16_t m_rnti;
	uint32_t m_frameNum;
	uint8_t  m_sfNum;
	uint8_t  m_slotNum;
	uint8_t  m_symStart;
	uint8_t  m_numSym;
	uint32_t m_tbSize;
	uint8_t  m_mcs;
	uint8_t  m_rv;
	double   m_sinr;
	double   m_sinrMin;
	double	 m_tbler;
	bool		 m_corrupt;
};

struct DlHarqInfo
{
  uint16_t  m_rnti;
  uint8_t   m_harqProcessId;
  enum HarqStatus
  {
    ACK, NACK
  };
  enum HarqStatus m_harqStatus;
  uint8_t	m_numRetx;
};

struct UlHarqInfo
{
  uint16_t  m_rnti;
  uint8_t   m_harqProcessId;
  std::vector <uint16_t> m_ulReception;
  enum ReceptionStatus
  {
    Ok, NotOk, NotValid
  } m_receptionStatus;
  uint8_t   m_tpc;
  uint8_t	m_numRetx;
};


class MmWavePhyMacCommon : public Object
{
public:
	MmWavePhyMacCommon (void);

	~MmWavePhyMacCommon (void)
	{
	}

	static TypeId GetTypeId (void);

	inline uint32_t
	GetSymbPerSlot (void)
	{
		return m_symbolsPerSlot;
	}

	inline double
	GetSymbolPeriod (void)
	{
		return m_symbolPeriod;
	}

	inline double
	GetSlotPeriod (void)
	{
		//return (double)(m_symbolsPerSlot * m_symbolPeriod);	// In microseconds
		return m_slotPeriod;	// In microseconds
	}

	inline uint32_t
	GetCtrlSymbols (void)
	{
		return m_ctrlSymbols;
	}

	inline uint32_t
	GetDlCtrlSymbols (void)
	{
		return m_dlCtrlSymbols;
	}

	inline uint32_t
	GetDlCtrlSymbols (uint16_t frameNum, uint8_t subframeNum)
	{
		return GetDlBlockSize(frameNum, subframeNum);
	}

	inline uint32_t
	GetUlCtrlSymbols (void)
	{
		return m_ulCtrlSymbols;
	}

	inline uint32_t
	GetSymbolsPerSubframe (void)
	{
		return m_symbolsPerSubframe;
	}

	inline uint32_t
	GetSymbolsPerSlot (void)
	{
		return m_symbolsPerSlot;
	}

	inline double
	GetSubframePeriod (void)
	{
		return m_subframePeriod;
	}

	inline double
	GetTti (void)
	{
		return (m_symbolsPerSlot*m_symbolPeriod*1e-6);
	}

	inline uint32_t
	GetSlotsPerSubframe (void)
	{
		return m_slotsPerSubframe;
	}

	inline uint32_t
	GetSubframesPerFrame (void)
	{
		return m_subframesPerFrame;
	}

	inline uint32_t
	GetNumReferenceSymbols (void)
	{
		return m_numRefSymbols;
	}

	inline double
	GetGuardPeriod (void)
	{
		return m_guardPeriod;
	}

	inline uint8_t
	GetUlSchedDelay (void)
	{
		return m_ulSchedDelay;
	}

	inline uint32_t
	GetNumSCperChunk (void)
	{
		return m_numSubCarriersPerChunk;
	}

	inline double
	GetChunkWidth (void)
	{
		return m_chunkWidth;
	}

	inline uint32_t
	GetNumChunkPerRb (void)
	{
		return m_chunksPerRb;
	}

	inline uint32_t
	GetNumRefScPerRb (void)
	{
		return m_numRefScPerRb;
	}

	inline uint32_t
	GetNumRefScPerSym (void)
	{
		return m_numRefScPerSym;
	}

	inline uint32_t
	GetNumRb (void)
	{
		return m_numRb;
	}

	inline uint32_t
	GetNumRbPerRbg (void)
	{
		return m_numRbPerRbg;
	}

	inline double
	GetRBWidth (void)
	{
		return (m_chunksPerRb*m_chunkWidth);
	}

	inline double
	GetSystemBandwidth (void)
	{
		return (GetRBWidth ()*m_numRb);
	}

	inline uint32_t
	GetTotalNumChunk ()
	{
		return (m_chunksPerRb*m_numRb);
	}

	inline double
	GetCentreFrequency (void)
	{
		return m_centerFrequency;
	}

	inline uint32_t
	GetL1L2CtrlLatency (void)
	{
		return m_l1L2CtrlLatency;
	}

	inline uint32_t
	GetL1L2DataLatency (void)
	{
		return m_l1L2DataLatency;
	}

	inline double
	GetWbCqiPeriodUs (void)
	{
		return m_wbCqiPeriodUs;
	}

	inline std::string
	GetStaticTDDPattern ()
	{
		return m_staticTddPattern;
	}

	inline uint32_t
	GetNumHarqProcess (void)
	{
		return m_numHarqProcess;
	}

	inline uint8_t
	GetHarqTimeout (void)
	{
		return m_harqTimeout;
	}

	inline uint32_t
	GetTbDecodeLatency (void)
	{
		return m_tbDecodeLatencyUs;
	}

	inline uint32_t
	GetMaxTbSize (void)
	{
		return m_maxTbSizeBytes;
	}

	void
	SetSymbPerSlot (uint32_t numSym)
	{
		m_symbolsPerSlot = numSym;
	}

	void
	SetSymbolPeriod (double prdSym)
	{
		m_symbolPeriod = prdSym;
	}

	void
	SetSymbolsPerSubframe (uint32_t numSym)
	{
		m_symbolsPerSubframe = numSym;
	}

	void
	SetSubframePeriod (double period)
	{
		m_subframePeriod = period;
	}

	void
	SetCtrlSymbols (uint32_t ctrlSymbols)
	{
		m_ctrlSymbols = ctrlSymbols;
	}

	void
	SetDlCtrlSymbols (uint32_t ctrlSymbols)
	{
		m_dlCtrlSymbols = ctrlSymbols;
	}

	void
	SetUlCtrlSymbols (uint32_t ctrlSymbols)
	{
		m_ulCtrlSymbols = ctrlSymbols;
	}

	void
	SetSlotPerSubframe (uint32_t numSlot)
	{
		m_slotsPerSubframe = numSlot;
	}

	void
	SetSubframePerFrame (uint32_t numSf)
	{
		m_subframesPerFrame = numSf;
	}

	void
	SetNumReferenceSymbols (uint32_t refSym)
	{
		m_numRefSymbols = refSym;
	}

	void
	SetGuardPeriod (double usec)
	{
		m_guardPeriod = usec;
	}

	void
	SetUlSchedDelay (uint32_t tti)
	{
		m_ulSchedDelay = tti;
	}

	void
	SetNumSCperChunk (uint32_t numSC)
	{
		m_numSubCarriersPerChunk = numSC;
	}

	void
	SetNumChunkPerRB (uint32_t numChunk)
	{
		m_chunksPerRb = numChunk;
	}

	void
	SetNumRefScPerRb (uint32_t numRefSc)
	{
		m_numRefScPerRb = numRefSc;
	}

	void
	SetNumRefScPerSym (uint32_t numRefSc)
	{
		m_numRefScPerSym = numRefSc;
	}

	void
	SetChunkWidth (double chumkWidth)
	{
		m_chunkWidth = chumkWidth;
	}
	void
	SetNumRb (uint32_t numRB)
	{
		m_numRb = numRB;
	}

	void
	SetNumRbPerRbg (uint32_t numRB)
	{
		m_numRbPerRbg = numRB;
	}

	void
	SetCentreFrequency (double fc)
	{
		m_centerFrequency = fc;
	}

	void
	SetL1L2CtrlLatency (uint32_t delaySfs)
	{
		m_l1L2CtrlLatency = delaySfs;
	}

	void
	SetL1L2DataLatency (uint32_t delaySlots)
	{
		m_l1L2DataLatency = delaySlots;
	}

	void
	SetWbCqiPeriodUs (double us)
	{
		m_wbCqiPeriodUs = us;
	}

	void
	SetStaticTDDPattern (std::string p)
	{
		if (p.length () != m_slotsPerSubframe)
		{
			NS_FATAL_ERROR ("TDD pattern length should be equal to the number of slots per SF");
		}
		m_staticTddPattern = p;
	}

	void
	SetNumHarqProcess (uint32_t numProcess)
	{
		m_numHarqProcess = numProcess;
	}

	void
	SetHarqDlTimeout (uint8_t harqDlTimeout)
	{
		m_harqTimeout = harqDlTimeout;
	}

	void
	SetTbDecodeLatency (uint32_t us)
	{
		m_tbDecodeLatencyUs = us;
	}

	void
	SetMaxTbSize (uint32_t bytes)
	{
		m_maxTbSizeBytes = bytes;
	}

	// Carlos modification
	enum SubCarrierSpacing
	{
		SCS15KHz = 15000,
		SCS30KHz = 30000,
		SCS60KHz = 60000,
		SCS120KHz = 120000,
		SCS240KHz = 240000,
		SCS480KHz = 480000,
		SCS960KHz = 960000
	};

	// SS burst and SS burst set period in ms
	typedef enum SsBurstPeriods
	{
		ms0 = 0,
		ms1 = 1,
		ms5 = 5,
		ms10 = 10,
		ms20 = 20,
		ms40 = 40,
		ms80 = 80,
		ms160 = 160
	}SsBurstPeriods, CsiReportingPeriod;

	// CSI-RS resource allocation periodicity in ms
//	typedef enum CsiReportingPeriod
//	{
//		ms10 = 10,
//		ms20 = 20,
//		ms40 = 40,
//		ms80 = 80
//	} CsiReportingPeriod;

	// CSI-RS resource allocation periodicity in slots
	typedef enum CsiReportingPeriodSlots
	{
		slot4 = 4,
		slot5 = 5,
		slot10 = 10,
		slot20 = 20,
		slot40 = 40,
		slot80 = 80,
		slot160 = 160,
		slot320 = 320,
		slot640 = 640,
		slot1280 = 1280
	}CsiReportingPeriodSlots;

	/*
	 * \brief Populates the SCS-related information.
	 * \param scs: determines the SCS for the whole system bandwidth
	 * \param paternFlag: If set to false, it disables the SS block mapping according to 3GPP symbol starting indexes in TS 38.213 and SS blocks are transmitted in every slot
	 */
	void
	SetScs (SubCarrierSpacing scs, bool paternFlag);

	void
	SetSsBurstSetPeriod(SsBurstPeriods ssBurstSetPeriod);

	inline SsBurstPeriods
	GetSsBurstSetPeriod ()
	{
		return m_ssBurstSetPeriod;
	}

	inline SsBurstPeriods
	GetSsBurstSetLength ()
	{
		return m_ssBurstSetLength;
	}

	bool
	GetSsBlockSlotStatus ();

	bool
	CheckSsBlockSlotStatus(uint16_t slotId);

	inline void
	IncreaseCurrentSsSlotId()
	{
		m_currentSsBlockSlotId = (m_currentSsBlockSlotId + 1)%m_maxSsBlockSlotId;
	}

	inline uint16_t
	GetCurrentSsSlotId()
	{
		return m_currentSsBlockSlotId;
	}

	inline uint16_t
	GetCsiReportPeriodicity()
	{
		return m_Xp;
	}


//	void
//	setTxBeamId (uint16_t id)
//	{
//		m_txBeamId = id;
//	}
//
//	void
//	setRxBeamId (uint16_t id)
//	{
//		m_rxBeamId = id;
//	}
//
//	uint16_t
//	getTxBeamId ()
//	{
//		return m_txBeamId;
//	}
//	uint16_t
//	getRxBeamId ()
//	{
//		return m_rxBeamId;
//	}
	// End of Carlos modification

	uint16_t GetSsBurstOfdmIndex(uint16_t index);

	inline uint16_t
	GetSsBlockPatternLength()
	{
		return m_ssBurstPattern.size();
	}

	uint32_t GetDlBlockSize(uint16_t frameNum, uint8_t subframeNum);

	void SetPeriodicCsiResourcePeriodicity(CsiReportingPeriod Xp);

	void SetNumBlocksForCsiResources(uint16_t size);

	void SetPeriodicCsiResourceAllocationCondition(bool condition)
	{
		m_periodicCsiResources = condition;
	}

	bool GetPeriodicCsiResourceAllocationCondition ()
	{
		return m_periodicCsiResources;
	}

private:
	uint32_t m_symbolsPerSlot;
	double   m_symbolPeriod; // in micro seconds
	uint32_t m_symbolsPerSubframe;
	double	 m_subframePeriod;
	uint32_t m_ctrlSymbols;
	uint32_t m_dlCtrlSymbols;  // num OFDM symbols for downlink control at beginning of subframe
	uint32_t m_ulCtrlSymbols;  // num OFDM symbols for uplink control at end of subframe
	uint32_t m_slotsPerSubframe;
	uint32_t m_subframesPerFrame;
	uint32_t m_numRefSymbols;
  uint32_t m_numRbPerRbg;

	uint32_t m_numSubCarriersPerChunk;
	uint32_t m_chunksPerRb;
	uint32_t m_numRefScPerRb;
	uint32_t m_numRefScPerSym;							 // for TDMA, number of reference subcarriers across entire bandwidth (default to 1/4th of SCs)
	double m_chunkWidth; //enter in Hz.
	uint32_t m_numRb;
	uint8_t m_numHarqProcess;
	uint8_t m_harqTimeout;

	double m_centerFrequency;
	double m_guardPeriod; 		// UL to DL switching time in micro seconds

	uint32_t m_l1L2CtrlLatency; // In no. of sub-frames
	uint32_t m_l1L2DataLatency; // In no. of slots
	uint32_t m_ulSchedDelay;	// delay between transmission of UL-DCI and corresponding subframe in TTIs
	uint32_t m_wbCqiPeriodUs;	// WB CQI periodicity in microseconds

	uint32_t m_tbDecodeLatencyUs;
	uint32_t m_maxTbSizeBytes;

	std::string m_staticTddPattern;		//This is really not used

	// Carlos modification
	SubCarrierSpacing m_scs;
	double	 m_slotPeriod;	// in micro seconds
	uint16_t m_csiPeriodicReportPeriodicity;	// In slots

	std::vector<uint16_t> m_ssBurstPattern;		// Slot IDs with SS blocks (at least one) within a half frame (5 ms)
	//TODO: the m_ssBurstPattern only supports one SS block per slot, and the TS 38.213 supports one, two or three.
	uint16_t m_currentSsBlockSlotId;		// Current slot ID within a half frame (5 ms)
	uint16_t m_maxSsBlockSlotId;			// Maximum number of slot ID, determined by the SCS: 5 times the number of slots per subframe
	// The following 2 variables determine the SS Burst Set structure
	SsBurstPeriods m_ssBurstSetLength;	// SS burst set duration (5 ms)
	SsBurstPeriods m_ssBurstSetPeriod;	// SS burst set period

	// CSI-RS resource periodic allocation for tracking (see TR 38.214 - 5.1.6.1.1 CSI-RS for tracking)
	bool m_periodicCsiResources;	// Flag to enable (true) or disable (false) periodic CSI resource allocation
	CsiReportingPeriod m_Xp;	// in milliseconds
	CsiReportingPeriodSlots m_csiPeriodicResourceAllocationSlots;	// in slots
	uint16_t m_numRbsForPeriodicCsiResource;	// Size in RBs of a periodic CSI-RS resource
};

}


#endif /* SRC_MMWAVE_MODEL_MMWAVE_PHY_MAC_COMMON_H_ */
