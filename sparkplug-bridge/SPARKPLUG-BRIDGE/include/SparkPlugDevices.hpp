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

/*** SparkPlugDevices.hpp Maintains the spark plug device operations */

#ifndef SPARKPLUG_DEVICES_HPP_
#define SPARKPLUG_DEVICES_HPP_

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <typeinfo>
#include <map>
#include <functional>
#include <mutex>

#include "Metric.hpp"
#include "NetworkInfo.hpp"
#include "Common.hpp"

extern "C"
{
#include "cjson/cJSON.h"
}

#define WIDTH_ONE		1
#define WIDTH_TWO		2
#define WIDTH_FOUR		4
#define WIDTH_EIGHT 		8

class CSparkPlugDev;
class CVendorApp;

using devSparkplugMap_t = std::map<std::string, CSparkPlugDev>; /** map with key as string and value as CSparkPlugDev*/
using var_dev_ref_t = std::variant<std::monostate, std::reference_wrapper<const network_info::CUniqueDataDevice>>; /**Type alias for metric value*/

/** Enumerator specifying device status*/
enum eDevStatus
{
	enDEVSTATUS_NONE, enDEVSTATUS_UP, enDEVSTATUS_DOWN
};

enum eYMlDataType
{
	enBOOLEAN = 0, enUINT, enINT, enFLOAT, enDOUBLE, enSTRING, enUNKNOWN
};

struct stRefForSparkPlugAction;

/** class holding spark plug device information*/
class CSparkPlugDev
{
	std::string m_sSubDev;/** subscriber device*/
	std::string m_sSparkPlugName;/**spark plug name*/
	bool m_bIsVendorApp;/** vendor app or not(true or false)*/
	metricMapIf_t m_mapMetrics; /** reference for metricMapIf_t*/
	std::atomic<eDevStatus> m_enLastStatetPublishedToSCADA;/** last state published to scada*/
	std::atomic<eDevStatus> m_enLastKnownStateFromDev; /** last state known from device*/
	uint64_t m_deathTimestamp; /** value for death timestamp*/
	var_dev_ref_t m_rDirectDevRef; /** direct device reference*/
	std::mutex m_mutexMetricList; /** mutex for metric list*/

	CSparkPlugDev& operator=(const CSparkPlugDev&) = delete;	/// assignmnet operator

	bool parseRealDeviceUpdateMsg(const std::string &a_sPayLoad, 
		std::string &a_sMetric, std::string &a_sValue, std::string &a_sStatus,
		uint64_t &a_usec, uint64_t &a_lastGoodUsec, uint32_t &a_error_code);

	bool parseScaledValueRealDevices(const std::string &a_sPayLoad, std::string &a_sMetric,
			 CValObj &a_rValobj);

	bool validateRealDeviceUpdateData(
		const std::string &a_sValue, std::string a_sStatus,
		uint32_t a_error_code,
		bool &a_bIsGood, bool &a_bIsDeathCode);

	bool prepareModbusMessage(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload, 
		const metricMapIf_t &a_mapMetrics, bool a_bIsBirth);
public:
	/** constructor*/
	CSparkPlugDev(std::string a_sSubDev, std::string a_sSparkPluName,
			bool a_bIsVendorApp = false) :
			m_sSubDev{ a_sSubDev }, m_sSparkPlugName{ a_sSparkPluName },
			m_bIsVendorApp{ a_bIsVendorApp }, 
			m_enLastStatetPublishedToSCADA{enDEVSTATUS_NONE},
			m_enLastKnownStateFromDev{enDEVSTATUS_NONE},
			m_deathTimestamp {0}, m_mutexMetricList{}
	{
	}
	
	CSparkPlugDev(const network_info::CUniqueDataDevice &a_rUniqueDev, std::string a_sSparkPlugName) :
			m_sSubDev{ a_rUniqueDev.getWellSiteDev().getID() }, 
			m_sSparkPlugName{ a_sSparkPlugName },
			m_bIsVendorApp{ false },
			m_enLastStatetPublishedToSCADA{enDEVSTATUS_NONE},
			m_enLastKnownStateFromDev{enDEVSTATUS_NONE},
			m_deathTimestamp {0}, m_rDirectDevRef {a_rUniqueDev}, m_mutexMetricList{}
	{
	}

	CSparkPlugDev(const CSparkPlugDev &a_refObj) :
			m_sSubDev{ a_refObj.m_sSubDev }, m_sSparkPlugName{ a_refObj.m_sSparkPlugName },
			m_bIsVendorApp{ a_refObj.m_bIsVendorApp }, m_mapMetrics{a_refObj.m_mapMetrics},
			m_enLastStatetPublishedToSCADA{enDEVSTATUS_NONE},
			m_enLastKnownStateFromDev{enDEVSTATUS_NONE},
			m_deathTimestamp {0}, 
			m_rDirectDevRef{a_refObj.m_rDirectDevRef}, m_mutexMetricList{}
	{
	}

	void addMetric(const network_info::CUniqueDataPoint &a_rUniqueDataPoint);

	/** function to set death time*/
	void setDeathTime(uint64_t a_deviceDeathTimestamp)
	{
		m_deathTimestamp = a_deviceDeathTimestamp;
	}

	/** function to get death time*/
	uint64_t getDeathTime()
	{
		return m_deathTimestamp;
	}

	/** function to get subscriber device name*/
	std::string getSubDevName()
	{
		return m_sSubDev;
	}

	/**function to get spark plug name*/
	std::string getSparkPlugName()
	{
		return m_sSparkPlugName;
	}

	/**function to check metric*/
	bool checkMetric(org_eclipse_tahu_protobuf_Payload_Metric& a_sparkplugMetric)
	{
		bool flag = false;
		try
		{
			//
			if(NULL != a_sparkplugMetric.name)
			{
				std::string sName{a_sparkplugMetric.name};
				auto itr = m_mapMetrics.find(sName);
				if(m_mapMetrics.end() != itr)
				{
					if(nullptr == itr->second)
					{
						DO_LOG_ERROR("Metric data not found: " + sName);
						return false;
					}
					// metric name is found
					// Check if datatypes match				
					switch(((itr->second)->getValue()).getDataType())
					{
					case METRIC_DATA_TYPE_INT8:
					case METRIC_DATA_TYPE_INT16:
					case METRIC_DATA_TYPE_INT32:
					case METRIC_DATA_TYPE_INT64:
					case METRIC_DATA_TYPE_BOOLEAN:
					case METRIC_DATA_TYPE_FLOAT:
					case METRIC_DATA_TYPE_DOUBLE:
					case METRIC_DATA_TYPE_STRING:					
						if(((itr->second)->getValue()).getDataType() == a_sparkplugMetric.datatype)
						{
							flag = true;
						}
						break;

					// Unsigned datatypes are sometimes just treated as integers
					// Accordingly a datatype is received
					case METRIC_DATA_TYPE_UINT8:
					case METRIC_DATA_TYPE_UINT16:
					case METRIC_DATA_TYPE_UINT32:
					case METRIC_DATA_TYPE_UINT64:
						if(
							(METRIC_DATA_TYPE_UINT8 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_UINT16 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_INT8 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_INT16 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_INT32 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_UINT32 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_UINT64 == a_sparkplugMetric.datatype) ||
							(METRIC_DATA_TYPE_INT64 == a_sparkplugMetric.datatype)
						)
						{
							flag = true;
						}
						break;
					}

					/*
					*  Check for template datatype. Here it calls getDataType() method of CUDT class for
					*  template datatype validation wheras for all other primitive datatypes, we call 
					*  getDataType() of CValObj which is a friend function of CMetric class. 	
					*
					*  While preparing the m_mapMetric map in BIRTH message, the data type for template is METRIC_DATA_TYPE_TEMPLATE.
					*  This METRIC_DATA_TYPE_TEMPLATE datatype is saved in map m_mapMetric during BIRTH messsage in case of UDT datatype.
					*  Now when we receive the sparkplug payload, we compare the datatype received in payload with that stored in our previously 
				    *  created map m_mapMetric. They both should match with METRIC_DATA_TYPE_TEMPLATE datatype.					   
					*/

					if((itr->second)->getDataType() == METRIC_DATA_TYPE_TEMPLATE &&
						(itr->second)->getDataType() == a_sparkplugMetric.datatype)
					{
							flag = true;
					}

					if(false == flag)
					{
						DO_LOG_ERROR("Datatypes do not match for given metric: " + sName);
					}
				}
				else
				{
					DO_LOG_ERROR("Metric not found: " + sName);
				}
			}
		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("Unable to check whether metric is correct. Exception : " + std::string(e.what()));
		}
		return flag;
	}

	/** function to set spark plug name*/
	void setSparkPlugName(std::string a_sVal)
	{
		m_sSparkPlugName = a_sVal;
	}

	/** to set vendor app reference */
	void setVendorAppRef(CVendorApp &a_refVendorApp)
	{
	}

	metricMapIf_t processNewData(metricMapIf_t &a_MetricList);
	metricMapIf_t processNewBirthData(metricMapIf_t &a_MetricList, bool &a_bIsOnlyValChange);
	
	/**To set published status */
	void setPublishedStatus(eDevStatus a_enStatus)
	{
		m_enLastStatetPublishedToSCADA.store(a_enStatus);
	}	

	/** To get last published device status*/
	eDevStatus getLastPublishedDevStatus()
	{
		return m_enLastStatetPublishedToSCADA.load();
	}

	/** function to set known device status*/
	void setKnownDevStatus(eDevStatus a_enStatus)
	{
		m_enLastKnownStateFromDev.store(a_enStatus);
	}	

	/** function to get last known device  status*/
	eDevStatus getLastKnownDevStatus()
	{
		return m_enLastKnownStateFromDev.load();
	}

	/** To check vendor app*/
	bool isVendorApp()
	{
		return m_bIsVendorApp;
	}
	
	bool prepareDBirthMessage(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload, bool a_bIsNBIRTHProcess);
	bool processRealDeviceUpdateMsg(const std::string a_sPayLoad, std::vector<stRefForSparkPlugAction> &a_stRefActionVec);

	void print()
	{
		/*std::cout << "Device: " << m_sSparkPlugName << ": Metric List:\n";
		for (auto &itr : m_mapMetrics)
		{
			itr.second.print();
		}*/
	}

	bool getWriteMsg(std::string& a_sTopic, cJSON *a_root, std::pair<const std::string, std::shared_ptr<CIfMetric>>& a_metric, const int& a_appSeqNo);
	bool getCMDMsg(std::string& a_sTopic, metricMapIf_t& m_metrics, cJSON *metricArray);

	bool prepareDdataMsg(org_eclipse_tahu_protobuf_Payload &a_payload, const metricMapIf_t &a_mapChangedMetrics);
};


/** Class for vendor App*/
class CVendorApp
{
	std::string m_sName; /** site name*/
	bool m_bIsDead; /** Is dead or not(true or false)*/
	std::map<std::string, std::reference_wrapper<CSparkPlugDev>> m_mapDevList; /** Map for device list*/
	std::mutex m_mutexDevList; /** mutex for device lst*/

	CVendorApp& operator=(const CVendorApp&) = delete;	/// assignmnet operator

public:
	/** constructor*/
	CVendorApp(std::string a_sName) :
			m_sName(a_sName), m_bIsDead{ false }, m_mutexDevList{}
	{
	}
	CVendorApp(const CVendorApp &a_refObj) :
			m_sName(a_refObj.m_sName), m_bIsDead{a_refObj.m_bIsDead},
			m_mapDevList{a_refObj.m_mapDevList}, m_mutexDevList{}
	{
	}

	/** Function to add device*/
	uint8_t addDevice(CSparkPlugDev &a_oDev)
	{
		std::lock_guard<std::mutex> lck(m_mutexDevList);
		std::string sDevName = a_oDev.getSparkPlugName();
		if (m_mapDevList.end() == m_mapDevList.find(sDevName))
		{
			m_mapDevList.emplace(sDevName, a_oDev);
			a_oDev.setVendorAppRef(*this);
			return 0;
		}
		return 1;
	}

	/** Function to get device*/
	std::map<std::string, std::reference_wrapper<CSparkPlugDev>>& getDevices()
	{
		std::lock_guard<std::mutex> lck(m_mutexDevList);
		return m_mapDevList;
	}

	/** Function to set death status*/
	void setDeathStatus(bool a_bIsDead)
	{
		m_bIsDead = a_bIsDead;
	}

	/**Function to check dead or not*/
	bool isDead()
	{
		return m_bIsDead;
	}
};


/** Class maintaining vendor app list*/
class CVendorAppList
{
	std::map<std::string, CVendorApp> m_mapVendorApps;/** map for vendor app*/
	std::mutex m_mutexAppList;/** mutex for app list*/

public:
	/**function to add device */
	void addDevice(std::string a_sVendorApp, CSparkPlugDev &a_oDev)
	{
		std::lock_guard<std::mutex> lck(m_mutexAppList);
		if (m_mapVendorApps.end() == m_mapVendorApps.find(a_sVendorApp))
		{
			CVendorApp temp
			{ a_sVendorApp };
			m_mapVendorApps.emplace(a_sVendorApp, temp);
		}

		auto &oVendorApp = m_mapVendorApps.at(a_sVendorApp);
		oVendorApp.addDevice(a_oDev);
	}

	/** function to get vendor app*/
	CVendorApp* getVendorApp(std::string a_sVendorApp)
	{
		std::lock_guard<std::mutex> lck(m_mutexAppList);
		if (m_mapVendorApps.end() == m_mapVendorApps.find(a_sVendorApp))
		{
			return NULL;
		}
		auto &oVendorApp = m_mapVendorApps.at(a_sVendorApp);
		return &oVendorApp;
	}

};

/** Enumerator specifying msg action*/
enum eMsgAction
{
	enMSG_NONE, enMSG_BIRTH, enMSG_DEATH, enMSG_DATA, enMSG_DCMD_CMD, enMSG_DCMD_WRITE, 
	enMSG_UDTDEF_TO_SCADA
};

/** Structure maintaining reference for sparkplug actions*/
struct stRefForSparkPlugAction
{
	std::reference_wrapper<CSparkPlugDev> m_refSparkPlugDev; /** wrapper for sparkplug device*/
	eMsgAction m_enAction; /** Action to be taken*/
	metricMapIf_t m_mapChangedMetrics; /** metrics to be used for taking action*/

	stRefForSparkPlugAction(std::reference_wrapper<CSparkPlugDev> a_ref,
			eMsgAction a_enAction, metricMapIf_t a_mapMetrics) :
			m_refSparkPlugDev{a_ref}, m_enAction{a_enAction}
			, m_mapChangedMetrics{a_mapMetrics}
	{
	}

	~stRefForSparkPlugAction()
	{
		m_mapChangedMetrics.clear();
	}
};
#endif
