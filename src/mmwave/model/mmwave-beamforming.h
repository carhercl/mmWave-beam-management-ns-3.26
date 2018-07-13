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
 */


#ifndef MMWAVE_BEAMFORMING_H_
#define MMWAVE_BEAMFORMING_H_

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
#include <ns3/mmwave-beam-management.h>
#include <ns3/multi-model-spectrum-channel.h>
#include <ns3/propagation-loss-model.h>


namespace ns3{

typedef std::vector<double> doubleVector_t;
typedef std::vector<doubleVector_t> double2DVector_t;

typedef std::vector< std::complex<double> > complexVector_t;
typedef std::vector<complexVector_t> complex2DVector_t;
typedef std::vector<complex2DVector_t> complex3DVector_t;
typedef std::vector<uint32_t> allocatedUeImsiVector_t;
typedef std::pair<Ptr<NetDevice>, Ptr<NetDevice> > key_t;



/**
* \store Spatial Signature and small scale fading matrices
*/
struct channelMatrix : public SimpleRefCount<channelMatrix>
{
	complex2DVector_t 	m_enbSpatialMatrix; // enb side spatial matrix
	complex2DVector_t 	m_ueSpatialMatrix; // ue side spatial matrix
	doubleVector_t 		m_powerFraction; // store power fraction vector of 20 paths
};
/**
* \store beamforming vectors to calculate beamforming gain and fading
*/
struct BeamformingParams : public SimpleRefCount<BeamformingParams>
{
	complexVector_t 	m_enbW; // enb beamforming vector
	complexVector_t 	m_ueW; // ue beamforming vector
	channelMatrix       m_channelMatrix;
	complexVector_t 	m_beam; // product of beamforming vectors and spatial matrices
};



/**
* \ingroup mmWave
* \MmWaveBeamforming models the beamforming gain and fading distortion in frequency and time for the mmWave channel
*/
class MmWaveBeamforming : public SpectrumPropagationLossModel
{
public:
	/**
	* \brief Set the pathNum and load files that store beamforming vector
	* \param enbAntenna antenna number of enb
	* \param ueAntenna antenna number of ue
	*/
	MmWaveBeamforming (uint32_t enbAntenna, uint32_t ueAntenna);
	virtual ~MmWaveBeamforming();

	static TypeId GetTypeId (void);
	void DoDispose();
	void SetConfigurationParameters (Ptr<MmWavePhyMacCommon> ptrConfig);
	Ptr<MmWavePhyMacCommon> GetConfigurationParameters (void) const;

	void LoadFile();
	/**
	* \brief Set the channel matrix for each link
	* \param ueDevices a pointer to ueNetDevice container
	* \param enbDevices a pointer to enbNetDevice container
	*/
	void Initial(NetDeviceContainer ueDevices, NetDeviceContainer enbDevices);
	void InitialModified(NetDeviceContainer ueDevices, NetDeviceContainer enbDevices);	//Carlos modification

	void UpdateMatrices (bool update);
	/**
	* \brief Set the beam sweeping vector of a pair of enb and ue (not necessarily connected)
	* \param ueDevice a pointer to ueNetDevice
	* \param enbDevice a pointer to enbNetDevice
	*/
	void SetBeamSweepingVector (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);

//	/**
//	* \brief Updates the bf vectors of the channel matrix. This is called once the best direction (and beams) is known
//	*/
	void UpdateBfChannelMatrix (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice, BeamPairInfoStruct bestBeams);

private:
	/**
	* \brief Get complex number from a string
	* \param strCmplx a string store complex number i.e. 3+2i,
	* \return a complex number of the string
	*/
	std::complex<double> ParseComplex (std::string strCmplx);
	/**
	* \brief Load file which store small scale fading sigma vector
	*/
	void LoadSmallScaleFading ();
	/**
	* \brief Load file which store antenna weights for enb
	*/
	void LoadEnbAntenna ();
	/**
	* \brief Load file which store antenna weights for ue
	*/
	void LoadUeAntenna ();
	/**
	* \brief Load file which store spatial signature matrix for  enb
	*/
	void LoadEnbSpatialSignature ();
	/**
	* \brief Load file which store spatial signature matrix for  ue
	*/
	void LoadUeSpatialSignature ();
	/*
	 * \brief Calculates the eNB and UE spatial signatures when spatial signatures depending on the eNB and UE locations are needed
	 */
	void UpdateChannelMatrixAntennaSpatialSignatures(Ptr<const MobilityModel> a,Ptr<const MobilityModel> b,Ptr<BeamformingParams> bfParams) const;
	/**
	* \brief Calculate beamforming gain and fading distortion in frequency and time
	* \param txPsd set of values vs frequency representing the
	*              transmission power. See SpectrumChannel for details.
	* \param a sender mobility
	* \param b receiver mobility
	* \return set of values vs frequency representing the received
	*         power in the same units used for the txPsd parameter.
	*/
	Ptr<SpectrumValue> DoCalcRxPowerSpectralDensity (Ptr<const SpectrumValue> txPsd,
	                                                   Ptr<const MobilityModel> a,
	                                                   Ptr<const MobilityModel> b) const;
	/**
	* \brief Set the beamforming vector of connected enbs and ues
	* \param ueDevice a pointer to ueNetDevice
	* \param enbDevice a pointer to enbNetDevice
	*/
	void SetBeamformingVector (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);



	/**
	* \brief beamsweeping. Picks the first pairs of beamforming vectors of not connected enbs and ues
	* \param ueDevice a pointer to ueNetDevice
	* \param enbDevice a pointer to enbNetDevice
	*/
	void SetInitialBeamformingVectors (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);

	/**
	* \brief beamsweeping. Picks the first pairs of beamforming vectors of not connected enbs and ues
	* \param ueDevice a pointer to ueNetDevice
	* \param enbDevice a pointer to enbNetDevice
	*/
	void InitializeBeamsweeping (Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);

	/**
	* \brief Store the channel matrix to channelMatrixMap
	* \param ueDevice a pointer to ueNetDevice
	* \param enbDevice a pointer to enbNetDevice
	*/
	void SetChannelMatrix(Ptr<NetDevice> ueDevice, Ptr<NetDevice> enbDevice);
	/**
	* \brief Calculate the system bandwidth using
	* 		 the user defined parameters
	* \return value of the system bandwidth
	*/
	double GetSystemBandwidth () const;
	/**
	* \brief Calculate long term fading
	* \param bfParas a pointer to beamforming vectors
	* \return complex vector of gain
	*/
	complexVector_t GetLongTermFading (Ptr<BeamformingParams> bfParams) const;
	/**
	* \brief calculate power spectrum density considering beamforming and fading
	* \param bfParas a pointer to beamforming vectors
	* \param Psd set of values vs frequency representing the
	*              transmission power. See SpectrumChannel for details
	* \param speed a double value to relative speed of tx and rx
	* \return cset of values vs frequency representing the received
	*         power in the same units used for the txPsd parameter.
	*/
	Ptr<SpectrumValue> GetChannelGainVector (Ptr<const SpectrumValue> txPsd, Ptr<BeamformingParams> bfParams, double speed) const;


	/**
	* \brief Performs a complete beam sweeping to find the pre-calculated pair of
	* 				codebooks vectors for tx and rx that provides maximum rx power
	* \param Psd set of values vs frequency representing the
	*              transmission power. See SpectrumChannel for details
	*/
//	void BeamSweepTest (Ptr<const SpectrumValue> txPsd,
//						Ptr<const MobilityModel> a,
//						Ptr<const MobilityModel> b);

	SpectrumValue
	BeamMagic (Ptr<SpectrumValue>  txPsd,
				Ptr<NetDevice> enbNetDevice,
				Ptr<NetDevice> ueNetDevice);

// Moved to MmWaveBeamManagement
//	/**
//	* \brief Evaluates the combination of tx and rx beams from the Kronecker matrix
//	* \param Psd set of values vs frequency representing the
//	*              transmission power. See SpectrumChannel for details
//	*/
//	void BeamSweepStep (Ptr<const SpectrumValue> txPsd,
//							Ptr<const MobilityModel> a,
//							Ptr<const MobilityModel> b);

	/**
	* \a map to store channel matrix
	* key pair<NetDevice,NetDevice> a pair of pointer to NetDevice present enb and ue for downlink
	*/
	mutable std::map< key_t, Ptr<BeamformingParams> > m_channelMatrixMap;

	/**
	* \a map to store beam sweeping channel matrixes
	* key pair<NetDevice,NetDevice> a pair of pointer to NetDevice present enb and ue for downlink
	*/
	mutable std::map< key_t, Ptr<BeamformingParams> > m_channelScanningMatrixMap;


	uint32_t m_pathNum;
	uint32_t m_enbAntennaSize;
	uint32_t m_ueAntennaSize;
//	double m_subbandWidth;
//	double m_centreFrequency;
//	uint32_t m_numResourceBlocks;
//	uint32_t m_numSubbbandPerRB;

private:
	Ptr<MmWavePhyMacCommon> m_phyMacConfig;
	Time m_longTermUpdatePeriod;
	bool m_smallScale;
	bool m_fixSpeed;  // used for SINR sweep test
	double m_ueSpeed;
	bool m_update;
	Ptr<UniformRandomVariable> m_uniformRV;

    //Ptr<ExponentialRandomVariable> m_nextLongTermUpdate;  // next update of long term statistics in microseconds

	// Carlos modification
//	std::map <Ptr<NetDevice>, complex2DVector_t> m_enbCodebooks;	// Codebooks of all the enBs
//	std::map <Ptr<NetDevice>, complex2DVector_t> m_ueCodebooks;		// Codebooks of all the UEs
	// End of modification

};

}  //namespace ns3


#endif /* MMWAVE_BEAMFORMING_H_ */
