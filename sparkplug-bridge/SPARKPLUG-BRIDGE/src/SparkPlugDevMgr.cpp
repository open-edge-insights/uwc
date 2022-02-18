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
#include <string.h>
#include "SparkPlugDevMgr.hpp"
#include "SparkPlugUDTMgr.hpp"
#include "SCADAHandler.hpp"

/**
 * Splits topic name with given delimeter
 * @param a_sTopic :[in] topic to split
 * @param a_vsTopicParts :[out] stores all the split names in this vector
 * @param a_delimeter :[in] delimiter to be used with splitting string
 * @return true/false based on true/false
 */
bool CSparkPlugDevManager::getTopicParts(std::string a_sTopic,
		std::vector<std::string> &a_vsTopicParts, const string& a_delimeter)
{
	try
	{
		std::string delimiter = "/";
		if(! a_delimeter.empty())
		{
			delimiter = a_delimeter;
		}
		size_t last = 0;
		size_t next = 0;
		while ((next = a_sTopic.find(delimiter, last)) != std::string::npos)
		{
			a_vsTopicParts.push_back(a_sTopic.substr(last, next - last));
			last = next + 1;
		}
		a_vsTopicParts.push_back(a_sTopic.substr(last));
	}
	catch (std::exception &e)
	{
		DO_LOG_ERROR(std::string("Error:") + e.what());
		a_vsTopicParts.clear();
		return false;
	}
	return true;
}

/**
 * Processes messages received on internal MQTT broker and returns metrics along with
 * action BIRTH, DATA and DEATH
 * @param a_sTopic : [in] topic of message received on internal MQTT
 * @param a_sPayLoad :[in] payload of message received on internal MQTT
 * @param a_stRefActionVec :[out] stores all the metrics in this structure
 * @return true/false based on success/failure
 */
bool CSparkPlugDevManager::processExternalMQTTMsg(std::string a_sTopic,
		org_eclipse_tahu_protobuf_Payload& a_payload,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	metricMapIf_t oMetricMap;

	try
	{
		//parse topic to get site name and device name
		vector<string> vSplitTopic;

		if(false == getTopicParts(a_sTopic, vSplitTopic, "/"))
		{
			DO_LOG_ERROR("Could not split topic name");
			return false;
		}

		if(vSplitTopic.size() != 5)
		{
			DO_LOG_ERROR("Invalid topic while checking for device validity");
			return false;
		}

		//split device-site name into device and site
		vector<string> mDevName;
		//device-site
		string strDevName = vSplitTopic[4];
		getTopicParts(strDevName, mDevName, "-");
		bool bIsFound = false;
		auto itr = [&] () mutable -> devSparkplugMap_t::iterator
		{
			bIsFound = false;
			std::lock_guard<std::mutex> lck(m_mutexDevList);
			auto i = m_mapSparkPlugDev.find(strDevName);
			if (m_mapSparkPlugDev.end() != i)
			{
				bIsFound = true;
			}
			return i;
		}();

		if(false == bIsFound)
		{
			DO_LOG_ERROR("Device is not present in list : " + strDevName);
			return false;
		}

		for (pb_size_t i = 0; i < a_payload.metrics_count; i++)
		{
			if(NULL == a_payload.metrics[i].name)
			{
				DO_LOG_DEBUG("Metric name is not present in DCMD message. Ignored.");
				return false;
			}
			
			if((false == itr->second.isVendorApp()) &&
				(METRIC_DATA_TYPE_TEMPLATE == a_payload.metrics[i].datatype))
			{
				org_eclipse_tahu_protobuf_Payload_Template &udt_template = a_payload.metrics[i].value.template_value;
				for(uint iLoop = 0; iLoop < udt_template.metrics_count; iLoop++)
				{
					if(NULL == udt_template.metrics[iLoop].name)
					{
						DO_LOG_DEBUG("Metric name is not present in DCMD message. Ignored.");
						return false;
					}
					std::shared_ptr<CIfMetric> ptrCIfMetric = metricFactoryMethod(udt_template.metrics[iLoop].name, udt_template.metrics[iLoop].datatype);
					if(nullptr == ptrCIfMetric)
					{
						DO_LOG_ERROR("Unable to build metric. Not processing this message.");
						return false;
					}
					if (false == processDCMDMetric(itr->second, *ptrCIfMetric, udt_template.metrics[iLoop]))
					{
						DO_LOG_DEBUG("Could not process metric");
						return false;
					}
					else
					{
						oMetricMap.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));
					}
				}
			}
			else 
			{
				std::shared_ptr<CIfMetric> ptrCIfMetric = metricFactoryMethod(a_payload.metrics[i].name, a_payload.metrics[i].datatype);
				if(nullptr == ptrCIfMetric)
				{
					DO_LOG_ERROR("Unable to build metric. Not processing this message.");
					return false;
				}
				if (false == processDCMDMetric(itr->second, *ptrCIfMetric, a_payload.metrics[i]))
				{
					DO_LOG_DEBUG("Could not process metric");
					return false;
				}
				else
				{
					oMetricMap.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));
				}
			}
		}

		auto &mapSparkPlugDev = itr->second;
		stRefForSparkPlugAction stCMDAction
		{ std::ref(mapSparkPlugDev), enMSG_DCMD_WRITE, oMetricMap };
		a_stRefActionVec.push_back(stCMDAction);
	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		return false;
	}
	return true;
}

/**
 * Processes messages received on internal MQTT broker and returns metrics along with
 * action BIRTH, DATA and DEATH
 * @param a_sTopic : [in] topic of message received on internal MQTT
 * @param a_sPayLoad :[in] payload of message received on internal MQTT
 * @param a_stRefActionVec :[out] stores all the metrics in this structure
 * @return true/false based on success/failure
 */
bool CSparkPlugDevManager::processInternalMQTTMsg(std::string a_sTopic,
		std::string a_sPayLoad,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	bool bRet = true;
	do
	{
		try
		{
			std::vector<std::string> vsTopicParts;
			getTopicParts(a_sTopic, vsTopicParts, "/");
			switch (vsTopicParts.size())
			{
			// Template defintion
			// TemplateDef
			case 1:
			{
				std::string sSubTopic{ vsTopicParts[0] };
				std::transform(sSubTopic.begin(), sSubTopic.end(),
						sSubTopic.begin(), ::tolower);

				if (0 == sSubTopic.compare("templatedef"))
				{
					CSparkPlugUDTManager::getInstance().processTemplateDef(a_sPayLoad, a_stRefActionVec);
				}
				else
				{
					DO_LOG_ERROR("Ignoring the message. Incorrect topic: " + a_sTopic);
					bRet = false;
				}
			}
			break;

			// DEATH message
			// DEATH/{NAON_UWCP_ID}
			case 2:
			{
				std::string sSubTopic
				{ vsTopicParts[0] };
				std::transform(sSubTopic.begin(), sSubTopic.end(),
						sSubTopic.begin(), ::tolower);

				if (0 == sSubTopic.compare("death"))
				{
					processDeathMsg(vsTopicParts[1], a_sPayLoad, a_stRefActionVec);
				}
				else
				{
					DO_LOG_ERROR(
							"Ignoring the message. Incorrect topic: "
							+ a_sTopic);
					bRet = false;
				}
			}
			break;

			// BIRTH and DATA message:
			// BIRTH/{NAON_UWCP_ID}/{WellheadID}
			// DATA/{NAON_UWCP_ID}/{WellheadID}
			case 3:
			{
				std::string sSubTopic
				{ vsTopicParts[0] };
				std::transform(sSubTopic.begin(), sSubTopic.end(),
						sSubTopic.begin(), ::tolower);

				if (0 == sSubTopic.compare("birth"))
				{
					processBirthMsg(vsTopicParts[1], vsTopicParts[2],
							a_sPayLoad, a_stRefActionVec);
				}
				else if (0 == sSubTopic.compare("data"))
				{
					processDataMsg(vsTopicParts[1], vsTopicParts[2], a_sPayLoad,
							a_stRefActionVec);
				}
				else
				{
					DO_LOG_ERROR(
							"Ignoring the message. Incorrect topic: "
							+ a_sTopic);
					bRet = false;
				}
			}
			break;

			// update message:
			// /device/wellhead/point/update
			case 5:
			{
				std::string sSubTopic{ vsTopicParts[4] };
				std::transform(sSubTopic.begin(), sSubTopic.end(), sSubTopic.begin(), ::tolower);

				if (0 == sSubTopic.compare("update"))
				{
					processUpdateMsg(vsTopicParts[1], vsTopicParts[2], a_sPayLoad,
							a_stRefActionVec);
				}
			}
			break;

			default:
				DO_LOG_ERROR(
						"Ignoring the message. Incorrect topic: " + a_sTopic)
						;
				bRet = false;
				break;
			}
		} catch (std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
			bRet = false;
		}
	} while (0);
	return bRet;
}

/**
 * Processes metric to parse its data-type and value; sets in CValueObj corresponding to the metric
 * @param a_SPDev :[in] device to be used for checking for metric presence
 * @param a_oMetric :[in] metric to be parsed
 * @param a_sparkplugMetric :[in] input Sparkplug DCMD metric
 * @return true/false based on success/failure
 */
bool CSparkPlugDevManager::processDCMDMetric(CSparkPlugDev& a_SPDev, CIfMetric &a_oMetric, org_eclipse_tahu_protobuf_Payload_Metric& a_sparkplugMetric)
{
		try
		{
		//to check if given metric is a part of the device
		if(false == a_SPDev.checkMetric(a_sparkplugMetric))
			{
				return false;
			}

		//map data-type with Sparkplug data-type
		if (false == a_oMetric.setValObj(a_sparkplugMetric))
			{
			DO_LOG_ERROR("Cannot parse value.");
				return false;
			}
	}
	catch (std::exception &e)
			{
		DO_LOG_ERROR(std::string("Error:") + e.what());
				return false;
			}
	return true;
}

/**
 * Gets instance of CSparkPlugDevManager
 * @return instance of CSparkPlugDevManager
 */
CSparkPlugDevManager& CSparkPlugDevManager::getInstance()
{
	static CSparkPlugDevManager _self;
	return _self;
}

/**
 * Creates metric of correct type based on input
 * @param a_sName :[in] metric name
 * @param a_uiDataType :[in] datatype to be used for metric creation
 * @return metric reference
 */
std::shared_ptr<CIfMetric> CSparkPlugDevManager::metricFactoryMethod(const std::string &a_sName,
										uint32_t a_uiDataType)
{
	std::shared_ptr<CIfMetric>cIfMetric = nullptr;
	do
			{
		try
		{
			// Metric type is not template i.e. metric is not UDT
			if(METRIC_DATA_TYPE_TEMPLATE != a_uiDataType)
			{
				// Create a metric data object
				cIfMetric = std::make_shared<CMetric>(a_sName, a_uiDataType);
			}
			else
			{
				// Create a UDT object
				cIfMetric = std::make_shared<CUDT>(a_sName, a_uiDataType);
			}
		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
			return nullptr;
		}
	} while (0);
	return cIfMetric;
}

/**
 * Processes metric to parse its data-type and value; creates metric of correct type
 * @param a_cjArrayElemMetric :[in] cJSON array element containing details about the metric
 * @return metric reference
 */
std::shared_ptr<CIfMetric> CSparkPlugDevManager::metricFactoryMethod(cJSON *a_cjArrayElemMetric)
{
	std::shared_ptr<CIfMetric>cIfMetric = nullptr;
	do
	{
		try
		{
			if (a_cjArrayElemMetric == NULL)
			{
				DO_LOG_ERROR("Invalid payload: Name or datatype or value is missing.");
				return cIfMetric;
			}

			cJSON *cjDataType = cJSON_GetObjectItem(a_cjArrayElemMetric, "dataType");
			if (cjDataType == NULL)
			{
				DO_LOG_ERROR("Invalid payload: dataType is not found.");
				return cIfMetric;
			}

			if (cjDataType->valuestring == NULL)
			{
				DO_LOG_ERROR("Datatype is NULL. Cannot parse payload.");
				return cIfMetric;
			}

			std::string sDataType{ cjDataType->valuestring };

			std::transform(sDataType.begin(), sDataType.end(),
					sDataType.begin(), ::tolower);

			uint32_t uiDataType = 0;

			// bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double, std::string
			if ("boolean" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_BOOLEAN;
			}
			else if ("uint8" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_UINT8;
			}
			else if ("uint16" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_UINT16;
			}
			else if ("uint32" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_UINT32;
			}
			else if ("uint64" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_UINT64;
			}
			else if ("int8" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_INT8;
			}
			else if ("int16" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_INT16;
			}
			else if ("int32" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_INT32;
			}
			else if ("int64" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_INT64;
			}
			else if ("float" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_FLOAT;
			}
			else if ("double" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_DOUBLE;
			}
			else if ("string" == sDataType)
			{	
				uiDataType = METRIC_DATA_TYPE_STRING;
			}
			else if ("udt" == sDataType)
			{
				uiDataType = METRIC_DATA_TYPE_TEMPLATE;
			}
			else
			{
				DO_LOG_ERROR("Invalid data type found. Mentioned type: " + sDataType);
				return cIfMetric;
			}

			cJSON *cjName = cJSON_GetObjectItem(a_cjArrayElemMetric, "name");
			if (cjName == NULL)
			{
				DO_LOG_ERROR("Invalid payload: Name is not found.");
				return cIfMetric;
			}
			if (NULL == cjName->valuestring)
			{
				DO_LOG_ERROR("Invalid payload: Name is not found.");
				return cIfMetric;
			}
			std::string s_tempName {cjName->valuestring};
			if (s_tempName.empty())
			{
				DO_LOG_ERROR("Name is empty.");
				return cIfMetric;
			}

			// Creat metric
			cIfMetric = metricFactoryMethod(s_tempName, uiDataType);
	}
	catch (std::exception &e)
	{
		DO_LOG_ERROR(std::string("Error:") + e.what());
			return nullptr;
	}
	} while (0);
	return cIfMetric;
}

/**
 * Parses birth message of vendor app; stores metrics and corresponding values
 * @param a_oMetricMap:[out] conatiner to store parsed metrics 
 * @param a_cjRoot :[in] json payload containing metrics
 * @param a_sKey :[in] json key to look for 
 * @param a_bIsBirthMsg :[in] indicates that this is a birth message
 * @return nothing
 */
void CSparkPlugDevManager::parseVendorAppMericData(metricMapIf_t &a_oMetricMap,
										cJSON *a_cjRoot, const std::string &a_sKey,
										bool a_bIsBirthMsg)
{
	do
	{
		try
		{
			if (NULL == a_cjRoot)
			{
				DO_LOG_ERROR("Json payload is not received");
				break;
			}

			//start parsing the cjson payload
			cJSON *cjMetrics = cJSON_GetObjectItem(a_cjRoot, a_sKey.c_str());
			if (NULL == cjMetrics)
			{
				DO_LOG_ERROR(a_sKey + ": Key not found in input message");
				break;
			}
			int iTotalMetrics = cJSON_GetArraySize(cjMetrics);
			for (int iLoop = 0; iLoop < iTotalMetrics; ++iLoop)
			{
				cJSON *cjArrayElemMetric = cJSON_GetArrayItem(cjMetrics, iLoop);
				if (NULL != cjArrayElemMetric)
				{
					std::shared_ptr<CIfMetric>cIfMetric = metricFactoryMethod(cjArrayElemMetric);
					if(nullptr == cIfMetric)
					{
						DO_LOG_ERROR("Cannot create metric. Ignored.");
					}
					else 
					{
						if(false == cIfMetric->processMetric(cjArrayElemMetric))
					{
						DO_LOG_ERROR("Cannot parse metric. Ignored.");
					}
					else
					{
							bool bIsOK = true;
							// If it is a birth messsage processing, validate the metric
							// This is particularly applicable for UDTs
							if(a_bIsBirthMsg)
							{
								// Now validate the metric
								if(false == cIfMetric->validate())
								{
									DO_LOG_ERROR(cIfMetric->getName() + ": Metric validate failed. Ignored.");
									bIsOK = false;
								}
							}
							if(bIsOK)
							{
								a_oMetricMap.insert({cIfMetric->getName(), std::move(cIfMetric)});
					}
				}
			}
				}
			}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Parses birth message of vendor app; stores metrics and corresponding values
 * @param a_sPayLoad :[iin] payload containing metrics
 * @param a_bIsBirthMsg :[in] indicates that this is a birth message
 * @return map containing metric and corresponding values
 */
metricMapIf_t CSparkPlugDevManager::parseVendorAppBirthDataMessage(
							std::string a_sPayLoad, bool a_bIsBirthMsg)
{
	metricMapIf_t oMetricMap;
	cJSON *cjRoot = NULL;
	do
	{
		try
		{
			cjRoot = cJSON_Parse(a_sPayLoad.c_str());
			if (NULL == cjRoot)
			{
				DO_LOG_ERROR(
						"Message received from MQTT could not be parsed in json format");
				break;
			}

			parseVendorAppMericData(oMetricMap, cjRoot, "metrics", a_bIsBirthMsg);
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);

	if (NULL != cjRoot)
	{
		cJSON_Delete(cjRoot);
	}

	return oMetricMap;
}

/**
 * Parses birth message of vendor app death message
 * @param a_sPayLoad :[iin] payload containing metrics
 * @return timestamp for death message
 */
uint64_t CSparkPlugDevManager::parseVendorAppDeathMessage(std::string& a_sPayLoad)
{
	uint64_t uiDeathTime = 0;
	cJSON *root = NULL;

	try
	{
		root = cJSON_Parse(a_sPayLoad.c_str());
		if (NULL == root)
		{
			DO_LOG_ERROR("Message received from MQTT could not be parsed in json format");
			return get_current_timestamp();
		}

		if (!cJSON_HasObjectItem(root, "timestamp"))
		{
			uiDeathTime = get_current_timestamp();
		}
		else
		{
			uiDeathTime = cJSON_GetObjectItem(root, "timestamp")->valuedouble;
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
	}

	if (NULL != root)
	{
		cJSON_Delete(root);
	}

	return uiDeathTime;
}

/**
 * Processes DEATH message from vendor app
 * @param a_sAppName :[in] vendor app name for which DEATH has received
 * @param a_sPayLoad :[in] payload for DEATH message
 * @param a_stRefActionVec :[out] structure containing device information to be marked dead
 */
void CSparkPlugDevManager::processDeathMsg(std::string a_sAppName,
		std::string a_sPayLoad,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	do
	{
		try
		{
			if (a_sAppName.empty())
			{
				DO_LOG_ERROR("App-name is empty");
				break;
			}

			CVendorApp *pVendorApp = NULL;
			{
				std::lock_guard<std::mutex> lck(m_mutexDevList);
				pVendorApp = m_objVendorAppList.getVendorApp(a_sAppName);
			}
			if (NULL == pVendorApp)
			{
				DO_LOG_ERROR(
						a_sAppName
						+ ": Vendor app not found to process DEATH message. Ignoring.");
				break;
			}

			uint64_t uiDeathTime = parseVendorAppDeathMessage(a_sPayLoad);

			metricMapIf_t mapChangedMetrics;
			// mark all subdevices under this vendor app for DEATH
			auto &mapDev = pVendorApp->getDevices();
			for (auto &itr : mapDev)
			{
				itr.second.get().setDeathTime(uiDeathTime);

				stRefForSparkPlugAction stDummyAction
				{ std::ref(itr.second.get()), enMSG_DEATH, mapChangedMetrics };
				a_stRefActionVec.push_back(stDummyAction);
				itr.second.get().setKnownDevStatus(enDEVSTATUS_DOWN);
			}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Processes device birth message and updates storage accordingly
 * @param a_sAppName :[in] vendor app name
 * @param a_sSubDev :[in] sub device name
 * @param a_sPayLoad :[in] payload of birth message from vendor app
 * @param a_stRefActionVec : [out] map containing metric and corresponding values
 */
void CSparkPlugDevManager::processBirthMsg(std::string a_sAppName,
		std::string a_sSubDev, std::string a_sPayLoad,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	// There could be following cases:
	// Case 1: All are new metrics => This results in a DBIRTH message
	// Case 2: Datatype of few metrics are changed => This results in a DBIRTH message
	// Case 3: Only values of few metrics are changed. There are no other changes => This results in a DDATA message
	// Case 4: There are no changes. 
	// Case 5: Last published status was DDEATH. Then a next message should be DBIRTH

	do
	{
		try
		{
			if (a_sAppName.empty())
			{
				DO_LOG_ERROR("App-name is empty");
				break;
			}
			if (a_sSubDev.empty())
			{
				DO_LOG_ERROR("Subdevice is empty");
				break;
			}

			std::string sDevName(a_sAppName + SUBDEV_SEPARATOR_CHAR + a_sSubDev);
			DO_LOG_INFO("Device name is: " + sDevName);

			// Parse message
			metricMapIf_t mapMetricsInMsg = parseVendorAppBirthDataMessage(a_sPayLoad, true);

			if (0 == mapMetricsInMsg.size())
			{
				DO_LOG_ERROR("Metric List is empty");
				break;
			}

			// Find the device in list
			bool bIsNew = false;

			auto& oDev = [&] () mutable -> CSparkPlugDev&
					{
				std::lock_guard<std::mutex> lck(m_mutexDevList);
				auto itr = m_mapSparkPlugDev.find(sDevName);
				if (m_mapSparkPlugDev.end() == itr)
				{
					bIsNew = true;
					DO_LOG_INFO("New device found: " + sDevName);
					CSparkPlugDev oDev{ a_sSubDev, sDevName, true };
					m_mapSparkPlugDev.emplace(sDevName, oDev);
					itr = m_mapSparkPlugDev.find(sDevName);

					m_objVendorAppList.addDevice(a_sAppName, itr->second);
				}

				return itr->second;
					}();
					bool bIsOnlyValChange = false;
			metricMapIf_t mapChangedMetricsFromBirth = oDev.processNewBirthData(
							mapMetricsInMsg, bIsOnlyValChange);

					// Check if last published status was DDEATH
					if(oDev.getLastPublishedDevStatus() == enDEVSTATUS_DOWN)
					{
						// If last status was DDEATH, then a DBIRTH should be published
						stRefForSparkPlugAction stDummyAction
						{ std::ref(oDev), enMSG_BIRTH, mapChangedMetricsFromBirth };
						a_stRefActionVec.push_back(stDummyAction);
						oDev.setKnownDevStatus(enDEVSTATUS_UP);

						break;
					}
					// Check if any changes have occurred
					else if (0 != mapChangedMetricsFromBirth.size())
					{
						// Check if it is BIRTH message
						if (false == bIsOnlyValChange)
						{
							// Check if Device is new or not
							if (false == bIsNew)
							{
								// Not a new device. First send a DDEATH message
								// Changes have occurred. 1st publish a DDEATH message.
						metricMapIf_t mapChangedMetrics;
								stRefForSparkPlugAction stDummyAction
								{ std::ref(oDev), enMSG_DEATH, mapChangedMetrics };
								a_stRefActionVec.push_back(stDummyAction);
							}

							// Action for DBIRTH message
							stRefForSparkPlugAction stDummyAction
							{ std::ref(oDev), enMSG_BIRTH, mapChangedMetricsFromBirth };
							a_stRefActionVec.push_back(stDummyAction);
							oDev.setKnownDevStatus(enDEVSTATUS_UP);
						}
						else
						{
							// Action for DDATA message
							stRefForSparkPlugAction stDummyAction
							{ std::ref(oDev), enMSG_DATA, mapChangedMetricsFromBirth };
							a_stRefActionVec.push_back(stDummyAction);
							oDev.setKnownDevStatus(enDEVSTATUS_UP);
						}
					}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Processes real device update message and updates storage accordingly
 * @param a_sDeviceName :[in] real device name
 * @param a_sSubDev :[in] sub device name
 * @param a_sPayLoad :[in] payload of update message from real device
 * @param a_stRefActionVec : [out] map containing metric and corresponding values
 */
void CSparkPlugDevManager::processUpdateMsg(std::string a_sDeviceName,
		std::string a_sSubDev, std::string a_sPayLoad,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	// There could be following cases:
	// Case 1: Only values of few metrics are changed => This results in no DDATA message
	// Case 2: There are no changes => No DDATA message

	do
	{
		try
		{
			if (a_sDeviceName.empty())
			{
				DO_LOG_ERROR("App-name is empty. Ignoring.");
				break;
			}
			if (a_sSubDev.empty())
			{
				DO_LOG_ERROR("Subdevice is empty. Ignoring.");
				break;
			}

			std::string sDevName(a_sDeviceName + SUBDEV_SEPARATOR_CHAR + a_sSubDev);
			DO_LOG_DEBUG(sDevName + ":Device. Received message: " + a_sPayLoad);

			// Find the device in list
			bool bIsFound = false;			
			auto itr = [&] () mutable -> devSparkplugMap_t::iterator
			{
				bIsFound = false;
				std::lock_guard<std::mutex> lck(m_mutexDevList);
				auto i = m_mapSparkPlugDev.find(sDevName);
				if (m_mapSparkPlugDev.end() != i)
				{
					bIsFound = true;
				}
				return i;
			}();

			if (false == bIsFound)
			{
				DO_LOG_ERROR("Invalid real device. Ignoring.");
				break;
			}

			auto &oDev = itr->second;

			// Parse message and get metric info
			bool bRet = oDev.processRealDeviceUpdateMsg(a_sPayLoad, a_stRefActionVec);

			if(false == bRet)
			{
				DO_LOG_ERROR("Message processing failed. Ignored message: " + a_sPayLoad);
				break;
			}
		}
		catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Processes DATA message received from vendor app and updates storage accordingly
 * @param a_sAppName :[in] vendor app name
 * @param a_sSubDev :[in] sub device name
 * @param a_sPayLoad :[in] payload of DATA message
 * @param a_stRefActionVec :[out] map containing metric and corresponding values
 */
void CSparkPlugDevManager::processDataMsg(std::string a_sAppName,
		std::string a_sSubDev, std::string a_sPayLoad,
		std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	do
	{
		try
		{
			if (a_sAppName.empty())
			{
				DO_LOG_ERROR("App-name is empty");
				break;
			}
			if (a_sSubDev.empty())
			{
				DO_LOG_ERROR("Subdevice is empty");
				break;
			}
			std::string sDevName(a_sAppName + "-" + a_sSubDev);
			DO_LOG_INFO("Device name is: " + sDevName);

			// Find the device in list
			bool bIsFound = false;
			auto itr = [&] () mutable -> devSparkplugMap_t::iterator
					{
				bIsFound = false;
				std::lock_guard<std::mutex> lck(m_mutexDevList);
				auto i = m_mapSparkPlugDev.find(sDevName);
				if (m_mapSparkPlugDev.end() != i)
				{
					bIsFound = true;
				}
				return i;
					}();
					if (false == bIsFound)
					{
				DO_LOG_ERROR(sDevName
								+ ": Not found in dev-ist. Ignoring DATA message: "
								+ a_sPayLoad);
					}
					else
					{
						// Parse message
				metricMapIf_t mapMetricsInMsg = parseVendorAppBirthDataMessage(
						a_sPayLoad, false);

						if (mapMetricsInMsg.size() > 0)
						{
							auto &oDev = itr->second;
					metricMapIf_t mapChangedMetricsFromData = oDev.processNewData(
									mapMetricsInMsg);

							// Check if last published status was DDEATH
							if(oDev.getLastPublishedDevStatus() == enDEVSTATUS_DOWN)
							{
								// If last status was DDEATH, then a DBIRTH should be published
								stRefForSparkPlugAction stDummyAction
								{ std::ref(oDev), enMSG_BIRTH, mapChangedMetricsFromData };
								a_stRefActionVec.push_back(stDummyAction);
								oDev.setKnownDevStatus(enDEVSTATUS_UP);
							}
							else if (0 != mapChangedMetricsFromData.size())
							{
								// Action for DDATA message
								stRefForSparkPlugAction stDummyAction
								{ std::ref(oDev), enMSG_DATA, mapChangedMetricsFromData };
								a_stRefActionVec.push_back(stDummyAction);
								oDev.setKnownDevStatus(enDEVSTATUS_UP);
							}
						}
					}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Function to add a real Modbus devices as SparkPlu devices
 * @return true/false depending on the success/failure
 */
bool CSparkPlugDevManager::addRealDevices()
{
	do
	{
		try
		{
			using network_info::CUniqueDataPoint;

			std::lock_guard<std::mutex> lck(m_mutexDevList);
			auto &mapUniqueDevice =	network_info::getUniqueDeviceList();
			for (auto &rUniqueDev : mapUniqueDevice)
			{
				std::string sUniqueDev{rUniqueDev.second.getWellSiteDev().getID() + SUBDEV_SEPARATOR_CHAR + 
					rUniqueDev.second.getWellSite().getID()};

				auto itr = m_mapSparkPlugDev.find(sUniqueDev);
				// Create a new device, if not present
				if (m_mapSparkPlugDev.end() == itr)
				{
					DO_LOG_INFO("New device found: " + sUniqueDev);
					CSparkPlugDev oDev{ rUniqueDev.second, sUniqueDev };
					m_mapSparkPlugDev.emplace(sUniqueDev, oDev);

					// Now add datapoints to SparkPlug device as a metric
					auto& rSparkPlugDev = m_mapSparkPlugDev.at(sUniqueDev);
					auto& rPointList = rUniqueDev.second.getPoints();
					for (auto &rPoint : rPointList)
					{
						rSparkPlugDev.addMetric(rPoint.get());
					}
				}
				else
				{
					DO_LOG_ERROR(sUniqueDev + ": Repeat device found. Ignoring recent instance.");
				}
			}
		} 
		catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
	return true;
}

/**
 * Prepare device birth messages to be published on SCADA system
 * @param a_rTahuPayload :[out] reference of spark plug message payload in which to store birth messages
 * @param a_sDevName:[in] device name for which birth message to be generated
 * @param a_bIsNBIRTHProcess: [in] indicates whether DBIRTH is needed as a part of NBIRTH process
 * @return true/false depending on the success/failure
 */
bool CSparkPlugDevManager::prepareDBirthMessage(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload, std::string a_sDevName, bool a_bIsNBIRTHProcess)
{
	try
	{
		std::lock_guard<std::mutex> lck(m_mutexDevList);
		auto itr = m_mapSparkPlugDev.find(a_sDevName);
		// Check if device is found
		if (m_mapSparkPlugDev.end() != itr)
		{
			return itr->second.prepareDBirthMessage(a_rTahuPayload, a_bIsNBIRTHProcess);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return false;
}

/**
 * Returns list of device names
 * @return List of device names
 */
std::vector<std::string> CSparkPlugDevManager::getDeviceList()
{
	std::vector<std::string> sDevNameVector;
	try
	{
		std::lock_guard<std::mutex> lck(m_mutexDevList);
		for (auto const& itr : m_mapSparkPlugDev) 
		{
			sDevNameVector.push_back(itr.first);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
	}
	return sDevNameVector;
}

/**
 * Sets the status of last published message for this device
 * @param a_enStatus :[in] Publish status for this device for last message
 * @param a_sDevName :[in] device name for which birth message to be generated
 * @return true/false depending on the success/failure
 */
bool CSparkPlugDevManager::setMsgPublishedStatus(eDevStatus a_enStatus, std::string a_sDevName)
{
	try
	{
		std::lock_guard<std::mutex> lck(m_mutexDevList);
		auto itr = m_mapSparkPlugDev.find(a_sDevName);
		// Check if device is found
		if (m_mapSparkPlugDev.end() != itr)
		{
			itr->second.setPublishedStatus(a_enStatus);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}
