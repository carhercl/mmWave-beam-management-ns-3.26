/*
 * mmwave-beam-management.h
 *
 *  Created on: 12 oct. 2017
 *      Author: carlos
 */

#ifndef MMWAVE_BEAM_MANAGEMENT_H_
#define MMWAVE_BEAM_MANAGEMENT_H_


#include "ns3/object.h"
#include <ns3/spectrum-value.h>
#include <string.h>
#include "ns3/uinteger.h"
#include <complex>
#include <ns3/nstime.h>
#include <ns3/simple-ref-count.h>
#include <ns3/ptr.h>
#include <ns3/net-device-container.h>
#include <map>
#include <ns3/spectrum-signal-parameters.h>
#include <ns3/mobility-model.h>
#include <ns3/spectrum-propagation-loss-model.h>
#include <ns3/mmwave-phy-mac-common.h>
#include <ns3/random-variable-stream.h>
#include <ns3/mmwave-phy-mac-common.h>


namespace ns3
{

typedef std::vector< std::complex<double> > complexVector_t;
typedef std::vector<complexVector_t> complex2DVector_t;
typedef std::pair<uint16_t, uint16_t > sinrKey;		// <txBeamId,rxBeamId>

typedef std::pair<double, double> coordinate_t;	// UE coordinate in XY for finger printing

class MmWaveEnbPhy;
class MmWaveUePhy;

struct BeamSweepingParams
{
	uint16_t m_currentBeamId;		// The current (or last) beam id used for beam sweeping
	Time m_steerBeamInterval;		// The time period the beam is changed to the next one
	complex2DVector_t m_codebook;	// The codebook used for beam sweeping
};

struct BeamPairInfoStruct
{
	Ptr<NetDevice> m_targetNetDevice;
	uint16_t m_txBeamId;
	uint16_t m_rxBeamId;
	SpectrumValue m_sinrPsd;
	double m_avgSinr;

	BeamPairInfoStruct();
};

typedef struct BeamTrackingParams
{
	std::vector<BeamPairInfoStruct> m_beamPairList;
	uint16_t m_numBeamPairs;
	MmWavePhyMacCommon::CsiReportingPeriod csiReportPeriod;
	Time m_csiResourceLastAllocation;	// Determines the last time csi resources where allocated for beam tracking this user
} BeamTrackingParams;



class FingerprintingDatabase : public Object
{
public:
	FingerprintingDatabase ();
	virtual ~FingerprintingDatabase ();

	static TypeId GetTypeId (void);

	BeamTrackingParams FillBeamTrackingParamsFields(uint16_t num_pairs, std::vector<double> tx_beams_ids, std::vector<double> rx_beams_ids);

	/*
	 * Reads finger printing file and initializes internal finger printing map
	 */
	void LoadFingerPrinting();
	void LoadFingerPrinting (std::string inputFilename);

	std::string m_fingerprintingFilePath;

	inline void SetFingerprintingFilePath(std::string path)
	{
		m_fingerprintingFilePath = path;
	}

	void SetCurrentPathIndex (uint16_t id);

	uint16_t GetCurrentPathIndex();

	BeamTrackingParams GetBeamTrackingPairsCurrentIndex ();


private:

	std::map <coordinate_t,BeamTrackingParams> m_fingerPrintingMap;
	uint16_t m_current_ue_index;

};


class MmWaveBeamManagement : public Object
{

public:

	MmWaveBeamManagement ();
	virtual ~MmWaveBeamManagement ();

	static TypeId GetTypeId (void);

	void InitializeBeamManagerEnb (Ptr<MmWavePhyMacCommon> phyMacConfig, Ptr<MmWaveEnbPhy> phy);
	void InitializeBeamManagerUe (Ptr<MmWavePhyMacCommon> phyMacConfig, Ptr<MmWaveUePhy> phy);

	void InitializeBeamSweepingTx(Time beamChangeTime);
	void InitializeBeamSweepingRx(Time beamChangeTime);

	void SetBeamChangeInterval (Time beamChangePeriod);
	void SetBeamSweepCodebook (complex2DVector_t codebook);

	complexVector_t GetBeamSweepVector ();
	complexVector_t GetBeamSweepVector (uint16_t index);

	void BeamSweepStepTx ();
	void BeamSweepStepRx ();
	void BeamSweepStep ();

	void DisplayCurrentBeamId ();

	uint16_t GetCurrentBeamId ();

	uint16_t GetNumBlocksSinceLastBeamSweepUpdate ();

	uint16_t IncreaseNumBlocksSinceLastBeamSweepUpdate ();

	void ResetNumBlocksSinceLastBeamSweepUpdate ();

	void AddEnbSinr (Ptr<NetDevice> enbNetDevice, uint16_t enbBeamId, uint16_t ueBeamId, SpectrumValue sinr);

	/*
	 * @brief Alt0: Tracks all beam combinations.
	 */
	void Alt0BeamTrackingList();

	/*
	 * @brief Alt1: Determines the candidate beam pairs to track according to their avg SINR value.
	 */
	void FindBeamPairCandidatesSinr();

	/*
	 * @brief Alt2: Determines the best beam pair (SINR) and tracks the 4 closest beams at UE side (left, right, up and down).
	 */
	void Alt2BeamTrackingList();

	std::vector<uint16_t> GetSideImmediateNeighborBeams(uint16_t beam_id, uint16_t num_beams_h, uint16_t num_beams_v);

	std::vector<uint16_t> GetAlphaSpacedAzimuthBeamsFromOptimal(uint16_t opt_beam_id, uint16_t alpha, uint16_t num_beams_h);

	/*
	 * @brief Alt3: Alt2 and additional RX beams uniformly separated with alpha beams w.r.t best RX beam in the azimuth plane.
	 */
	void Alt3BeamTrackingList(uint16_t alpha);

	/*
	 * @brief Alt3: Alt2 and additional TX beams uniformly separated with alpha beams w.r.t best TX beam in the azimuth plane.
	 */
	void Alt4BeamTrackingList(uint16_t alpha);

	/*
	 * @brief Alt3: Alt2 and additional TX and RX beams uniformly separated with beta and alpha beams w.r.t best TX/RX beam in the azimuth plane.
	 */
	void Alt5BeamTrackingList(uint16_t beta, uint16_t alpha);

	/*
	 * @brief Beam tracking based on fingerprinting alternative 1. Only the optimal beam pair is tracked.
	 */
	void FingerPrinting_1();

	uint16_t GetCurrentNumBeamPairCandidates();

	uint16_t GetMaxNumBeamPairCandidates();

	void SetMaxNumBeamPairCandidates(uint16_t nBeamPairs);

	void FindBeamPairCandidates();

	BeamPairInfoStruct FindBestScannedBeamPairAndCreateBeamTrackingList ();

	BeamPairInfoStruct ExhaustiveBestScannedBeamPairSearch ();

	/*
	 * @brief Workaround to indicate the list of beam pairs for tracking to peer (from UE to gNB normally but could work the other way around).
	 * The Beam management class stores in the remote beam management class the beam pair vector
	 */
	void NotifyBeamPairCandidatesToPeer (Ptr<NetDevice>);

	void SetCandidateBeamPairSet (Ptr<NetDevice> NetDevice, std::vector<BeamPairInfoStruct> vector);

	BeamPairInfoStruct GetBestScannedBeamPair ();

	void UpdateBestScannedEnb();

	void ClearAllSinrMapEntries ();

	void ScheduleSsSlotSetStart(MmWavePhyMacCommon::SsBurstPeriods period);

	Time GetNextSsBlockTransmissionTime (Ptr<MmWavePhyMacCommon> mmWaveCommon, uint16_t currentSsBlock);

	// One CSI-RS resource per beam pair to monitor
	void SetNumPeriodicCsiResources(uint16_t numSimultaneousBeamsToMonitor)
	{
		m_maxNumBeamPairCandidates = numSimultaneousBeamsToMonitor;
	}

	uint16_t GetNumPeriodicCsiResources()
	{
		return m_maxNumBeamPairCandidates;
	}

	void SetBeamReportingPeriod (uint16_t period)
	{
		m_beamReportingPeriod = period;
	}

	uint16_t GetBeamReportingPeriod ()
	{
		return m_beamReportingPeriod;
	}

	bool GetBeamReportingEnabledCondition ()
	{
		return m_beamReportingEnabled;
	}

	void SetBeamReportingEnabledCondition (bool state)
	{
		m_beamReportingEnabled = state;
	}

	void EnableBeamReporting ()
	{
		m_beamReportingEnabled = true;
	}

	inline void SetTxCodebookFilePath(std::string path)
	{
		m_txFilePath = path;
	}

	inline void SetRxCodebookFilePath(std::string path)
	{
		m_rxFilePath = path;
	}

	std::map <Ptr<NetDevice>,BeamTrackingParams> GetDevicesMapToExpireTimer (Time margin);

	void IncreaseBeamReportingTimers (std::map <Ptr<NetDevice>,BeamTrackingParams> devicesToUpdate);

	BeamTrackingParams GetBeamsToTrackForEnb (Ptr<NetDevice> enb);

	BeamTrackingParams GetBeamsToTrack ();

	void UpdateBeamTrackingInfo (Ptr<NetDevice> peer, BeamTrackingParams beamTrackingInfo);

	void UpdateBeamTrackingInfoValues (Ptr<NetDevice> peer, BeamTrackingParams beamTrackingInfo);

	void SetBestScannedEnb(BeamPairInfoStruct bestEnbBeamInfo);

//	void SetCandidateBeamAlternative(uint16_t alt, uint16_t alpha);
	void SetCandidateBeamAlternative(uint16_t alt, uint16_t alpha, uint16_t beta, bool memory);

	void EnableSsbMeasMemory ();

	void DisableSsbMeasMemory ();

	void ConfigureBeamReporting ();

	void SetFingerprinting (Ptr<FingerprintingDatabase> database);

private:

	std::complex<double> ParseComplex (std::string strCmplx);

	/**
	* \brief Loads a generic beamforming matrix
	* \param Path to the input file to read
	* \return Array of complex values
	*/
	complex2DVector_t LoadCodebookFile (std::string inputFilename);

	std::string m_txFilePath;
	std::string m_rxFilePath;

	BeamSweepingParams m_beamSweepParams;

	Time m_lastBeamSweepUpdate;			// Timestamp of the last beam id modification

	uint16_t m_ssBlocksLastBeamSweepUpdate;	// Counter of ss blocks since the last beam update (when counter was reset)

	BeamPairInfoStruct m_bestScannedEnb;

	uint16_t m_maxNumBeamPairCandidates;	// Maximum number of beam pairs to monitor.
	uint16_t m_beamReportingPeriod;			// Beam reporting period in ms: Matches Xp parameter in PhyMacCommon class
	//std::vector<BeamPairInfoStruct> m_candidateBeams;
	std::map <Ptr<NetDevice>,BeamTrackingParams> m_candidateBeamsMap;
	uint16_t m_beamCandidateListStrategy;	// Selects the strategy to create the list of candidate beams
	uint16_t m_alpha;	// Alpha parameter in strategy to create the list of candidate beams number 3 and 5
	uint16_t m_beta;	// Alpha parameter in strategy to create the list of candidate beams number 4 and 5
	bool m_beamReportingEnabled;	// Determines if beam reporting is enabled at this time
	bool m_memorySs;	// Flag to determine if SS tracking decisions are to be made with the whole historical or only within the current SSB window

	std::map <Ptr<NetDevice>,std::map <sinrKey,SpectrumValue>> m_enbSinrMap;	//Map to all the eNBs
//	std::map <Ptr<NetDevice>,std::map <sinrKey,float>> m_ueSinrMap;	//Map to all the UEs

	Ptr<FingerprintingDatabase> m_fingerprinting;

};

}

#endif /* MMWAVE_BEAM_MANAGEMENT_H_ */
