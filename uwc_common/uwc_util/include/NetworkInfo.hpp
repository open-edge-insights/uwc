/********************************************************************************
* Copyright (c) 2021 Intel Corporation.

* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*********************************************************************************/

/*** NetworkInfo.hpp is for building network info based on network type*/

#ifndef INCLUDE_INC_NETWORKINFO_HPP_
#define INCLUDE_INC_NETWORKINFO_HPP_

#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include "YamlUtil.hpp"

#define SEPARATOR_CHAR "/"
#define PERIODIC_GENERIC_TOPIC "update"

using std::string;
using std::vector;

/** network_info holds information for structures and classes for building network */

namespace network_info 
{

    /** enumerator class holding the network type */
	enum class eNetworkType
	{
		eTCP,
		eRTU,
		eALL
	};

	/**enumerator class holding the endpoint type */
	enum class eEndPointType
	{
		eCoil,
		eHolding_Register,
		eInput_Register,
		eDiscrete_Input
	};

	/**structure for information of data point address */
	struct stDataPointAddress
	{
		int m_iAddress; /** Address value*/
		int m_iWidth; /** width value*/
		eEndPointType m_eType; /** type of end point*/
		bool m_bIsByteSwap; /** byte swap or not(true or false)*/
		bool m_bIsWordSwap; /** word swap or not(true or false)*/
		std::string m_sDataType; /** data type*/
		double m_dScaleFactor;
	};

	/** structure for polling data information*/
	struct stPollingData
	{
		unsigned int m_uiPollFreq; /** polling frequency*/
		bool m_bIsRealTime; /** RT or non-RT(true or false)*/
	};

	/** class holds the information for data points*/
	class CDataPoint
	{
		std::string m_sId; /** ID value*/
		struct stDataPointAddress m_stAddress; /** data point address*/
		struct stPollingData m_stPollingConfig; /** polling configuration */
		static eEndPointType getPointType(const std::string&);
		// dataPersist flag for each datapoint
		bool m_bIsDataPersist;
		
		public:
		const std::string& getID() const {return m_sId;}
		// get and set methods of Data Persistence for each Data Point
		const bool& getDataPersist() const { return m_bIsDataPersist;}
		void setDataPersist(const bool& a_bIsDataPersist) {  m_bIsDataPersist = a_bIsDataPersist; }

		const struct stDataPointAddress& getAddress() const { return m_stAddress;}
		const struct stPollingData& getPollingConfig() const { return m_stPollingConfig;}

		static void build(const YAML::Node& a_oData, CDataPoint &a_oCDataPoint, bool a_bDefaultRealTime);
	};
	
	/** class holds data for data points from YML file*/
	class CDataPointsYML
	{
		std::string m_sYMLName; /** YML file name*/
		std::string m_sVersion; /** YML file version*/
		std::vector<CDataPoint> m_DataPointList; /** vector for data pont list*/
		
		public:
		CDataPointsYML(const std::string &a_sYMLName)
		: m_sYMLName{a_sYMLName}, m_sVersion{}, m_DataPointList{}
		{};
		
		int addDataPoint(CDataPoint &a_oDataPoint); 
		const std::vector<CDataPoint>& getDataPoints() const {return m_DataPointList;}
		const std::string& getYMLFileName() const {return m_sYMLName;}; 
		void setVersion(const std::string&a_sVersion) {m_sVersion.assign(a_sVersion);};
		const std::string& getVersion() const {return m_sVersion;};
	};
	
	/** class holds data for device information*/
	class CDeviceInfo
	{
		std::string m_sYMLName; /** YML file name*/
		std::string m_sDevName; /** Name*/
		CDataPointsYML &m_rDataPointsYML; /** vector for data pont list*/
		
		public:
		CDeviceInfo(const std::string &a_sYMLName, const std::string& a_sDevName, CDataPointsYML &a_rDataPointsYML)
		: m_sYMLName{a_sYMLName}, m_sDevName{a_sDevName}, m_rDataPointsYML{a_rDataPointsYML}
		{}
		const std::vector<CDataPoint>& getDataPoints() const {return m_rDataPointsYML.getDataPoints();}
		const std::string& getYMLFileName() const {return m_sYMLName;};
		const std::string& getDevName() const {return m_sYMLName;};
		const CDataPointsYML& getDataPointsRef() const {return m_rDataPointsYML;};
	};
	
	/** structure for TCP address information*/
	struct stTCPAddrInfo
	{
		std::string m_sIPAddress; /** IP address value*/
		uint16_t m_ui16PortNumber; /**POrt number*/
		unsigned int m_uiUnitID; /** Unit ID value*/
	};

	/** Structure for TCP master information */
	struct stTCPMasterInfo
	{
		long m_lInterframeDelay; /** Interframe delay value*/
		long m_lResTimeout; /** Response time out*/
	};

	/** structure for RTU Address information*/
	struct stRTUAddrInfo
	{
		unsigned int m_uiSlaveId; /** slave ID*/
	};

	/** structure for modbus address information*/
	struct stModbusAddrInfo
	{
		eNetworkType m_NwType; /** network type*/
		struct stTCPAddrInfo m_stTCP; /** reference of struct stTCPAddrInfo*/
		struct stRTUAddrInfo m_stRTU; /** reference of struct stRTUAddrInfo*/
	};
	
	/**
	 *
	 * Class to maintain RTU network information
	 */
	class CRTUNetworkInfo
	{
		std::string m_sPortName; /** port name*/
		std::string m_sParity; /** parity value*/
		int m_iBaudRate; /** Baud rate value*/
		long m_lInterframeDelay; /** Internal frame delay value*/
		long m_lResTimeout; /** Response time out value*/

	public:
		/** constructor*/
		CRTUNetworkInfo()
		{
			m_sPortName = "";
			m_sParity = "";
			m_iBaudRate = 0;
			m_lInterframeDelay = 0;
			m_lResTimeout = 80;
		}

		/** Function to get baud rate*/
		int getBaudRate() const
		{
			return m_iBaudRate;
		}

		/** Function to get parity*/
		const std::string& getParity() const
		{
			return m_sParity;
		}

		/** function to get port name*/
		const std::string& getPortName() const
		{
			return m_sPortName;
		}

		static void buildRTUNwInfo(CRTUNetworkInfo &a_oNwInfo,
				std::string a_fileName);

		/** function to get interframe delay*/
		long getInterframeDelay() const
		{
			return m_lInterframeDelay;
		}

		/** function to get response time out*/
		long getResTimeout() const
		{
			return m_lResTimeout;
		}
	};

	/** class holding information for wellsite device information */
	class CWellSiteDevInfo
	{
		mutable int32_t m_iCtx; /** context value*/
		std::string m_sId; /** site ID value*/
		struct stModbusAddrInfo m_stAddress; /** reference of struct stModbusAddrInfo*/
		struct stTCPMasterInfo m_stTCPMasterInfo; /** reference of struct stTCPMasterInfo*/
		const CDeviceInfo &m_rDev; /** object of class CDeviceInfo*/
		class CRTUNetworkInfo m_rtuNwInfo; /** object of class CRTUNetworkInfo*/
		
		public:
		CWellSiteDevInfo(CDeviceInfo &a_rDev)
		: m_iCtx{-1}, m_sId{""}, m_stAddress{}, m_stTCPMasterInfo{}, m_rDev{a_rDev}, m_rtuNwInfo{}
		{}
		
		std::string getID() const {return m_sId;}

		const struct stModbusAddrInfo& getAddressInfo() const {return m_stAddress;}
		const CDeviceInfo& getDevInfo() const {return m_rDev;}

		const CRTUNetworkInfo& getRTUNwInfo() const {return m_rtuNwInfo;}
		const int32_t getCtxInfo() const {return m_iCtx;}
		void setCtxInfo(int a_iCtx) const {m_iCtx = a_iCtx;}

		static void build(const YAML::Node& a_oData, CWellSiteDevInfo &a_oWellSiteDevInfo);

		const struct stTCPMasterInfo& getTcpMasterInfo() const {
			return m_stTCPMasterInfo;
		}
	};
	
	/** class holding information of well sites.*/
	class CWellSiteInfo
	{
		std::string m_sId;/** site ID value*/
		std::vector<CWellSiteDevInfo> m_DevList; /** vector for device lists*/
		
		public:
		/** constructor*/
		CWellSiteInfo()
		{
		}
		std::string getID() const {return m_sId;}
		int addDevice(CWellSiteDevInfo a_oDevice);
		const std::vector<CWellSiteDevInfo>& getDevices() {return m_DevList;}
		const std::vector<CWellSiteDevInfo>& getDevices() const {return m_DevList;}

		static void build(const YAML::Node& a_oData, CWellSiteInfo &a_oWellSite);
	};
	
	/** Forward declaration*/
	class CUniqueDataPoint;

	/** class holding information for unique device data*/
	class CUniqueDataDevice
	{
	private:
		const CWellSiteInfo &m_rWellSite; /** object of class CWellSiteInfo*/
		const CWellSiteDevInfo &m_rWellSiteDev; /** object of class CWellSiteDevInfo*/
		std::vector<std::reference_wrapper<const CUniqueDataPoint>> m_rPointList; /** vector for point list*/

	public:
		/**constructor */
		CUniqueDataDevice(const CWellSiteInfo &a_rWellSite, const CWellSiteDevInfo &a_rWellSiteDev) :
				m_rWellSite{a_rWellSite}, m_rWellSiteDev { a_rWellSiteDev } {
		}
		const CWellSiteInfo& getWellSite() const {return m_rWellSite;}
		const CWellSiteDevInfo& getWellSiteDev() const {
			return m_rWellSiteDev;
		}
		void addPoint(const CUniqueDataPoint &a_rPoint);
		
		const std::vector<std::reference_wrapper<const CUniqueDataPoint>>& getPoints() const {return m_rPointList;}
	};

	/** class for maintaining Unique data point*/
	class CUniqueDataPoint
	{
		const unsigned int m_uiMyRollID; /** ID value*/
		const std::string m_sId; /** site ID value*/
		const CWellSiteInfo &m_rWellSite; /** reference of wellsite*/
		const CWellSiteDevInfo &m_rWellSiteDev; /** reference of wellsite device*/
		const CDataPoint &m_rPoint; /** reference of datapoint*/
		mutable std::atomic<bool> m_bIsAwaitResp;/**(true or false) */
		mutable std::atomic<bool> m_bIsRT; /** RT or non-RT(true or false)*/

	public:
		/** constructor*/
		CUniqueDataPoint(std::string a_sId, const CWellSiteInfo &a_rWellSite,
				const CWellSiteDevInfo &a_rWellSiteDev, const CDataPoint &a_rPoint);

		CUniqueDataPoint(const CUniqueDataPoint&);

		CUniqueDataPoint& operator=(const CUniqueDataPoint&) = delete;	/** Copy assign*/

		std::string getID() const {return m_sId;}
		const CWellSiteInfo& getWellSite() const {return m_rWellSite;}
		const CWellSiteDevInfo& getWellSiteDev() const {return m_rWellSiteDev;}
		const CDataPoint& getDataPoint() const {return m_rPoint;}

		unsigned int getMyRollID() const {return m_uiMyRollID;}

		bool isIsAwaitResp() const;

		void setIsAwaitResp(bool isAwaitResp) const;

		bool getRTFlag() const { return m_bIsRT; }
};

	void buildNetworkInfo(string a_strNetworkType, string DeviceListFile, string a_strAppId);
	const std::map<std::string, CWellSiteInfo>& getWellSiteList();
	const std::map<std::string, CUniqueDataPoint>& getUniquePointList();
	/**
	 * Get unique device list
	 * @return map of unique device
	 */
	const std::map<std::string, CUniqueDataDevice>& getUniqueDeviceList();
	/**
	 * Get data points YML file listing objects
	 * @return map of data points YML file listing objects
	 */
	const std::map<std::string, CDataPointsYML>& getDataPointsYMLList();

	bool validateIpAddress(const string &ipAddress);
	/** Returns true if s is a number else false */
	bool isNumber(string s);
}

#endif /* INCLUDE_INC_NETWORKINFO_HPP_ */
