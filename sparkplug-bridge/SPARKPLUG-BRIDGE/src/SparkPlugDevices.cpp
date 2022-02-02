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
#include <chrono>
#include "SparkPlugDevices.hpp"
#include "SCADAHandler.hpp"

#include <ctime>

/**
 * Process new device birth message from vendor app and extract all the information about metric
 * @param a_MetricList :[in] list of metrics present in a message payload received on internal MQTT broker
 * @param a_bIsOnlyValChange: [out] a flag indicates whether changes in BIRTH message are only changes in value which result in DDATA message rather than a DBIRTH message 
 * @return structured map of metrics
 */
metricMapIf_t CSparkPlugDev::processNewBirthData(metricMapIf_t &a_MetricList, bool &a_bIsOnlyValChange)
{
	// There could be following cases:
	// Case 1: All are new metrics => This results in a DBIRTH message
	// Case 2: Datatype of few metrics are changed => This results in a DBIRTH message
	// Case 3: Only values of few metrics are changed. There are no other changes => This results in a DDATA message
	// Case 4: There are no changes. 
	
	metricMapIf_t oMetricMap;
	// This is a special case: In BIRTH message when we just recceive a change in 
	metricMapIf_t oMetricMapData;
	// Default value: false: This means output is a BIRTH message
	a_bIsOnlyValChange = false;
	do
	{
		try
		{
			std::lock_guard<std::mutex> lck(m_mutexMetricList);
			for (auto &itrInputMetric : a_MetricList)
			{
				if(nullptr == itrInputMetric.second)
				{
					DO_LOG_ERROR(itrInputMetric.first + ": Metric data is not built.");
					continue;
				}
				auto itrMyMetric = m_mapMetrics.find(itrInputMetric.first);
				if (m_mapMetrics.end() == itrMyMetric)
				{
					m_mapMetrics.emplace(itrInputMetric.first,
							itrInputMetric.second);
					oMetricMap.emplace(itrInputMetric.first,
							itrInputMetric.second);
				}
				else
				{
					if(nullptr == itrMyMetric->second)
					{
						DO_LOG_ERROR(itrMyMetric->first + ": Metric data is not found.");
						continue;
					}
					// Compare data type
					switch ((itrMyMetric->second)->compareValue(*(itrInputMetric.second)))
					{
					case SAMEVALUE_OR_DTATYPE:
						// No action
						break;

					case VALUES_DIFFERENT:
						// Assign new value 
						(itrMyMetric->second)->assignNewValue(*(itrInputMetric.second));
						// Add data to a separate metric map for maintaining data
						oMetricMapData.emplace(itrInputMetric.first,
								itrInputMetric.second);
						break;

					case DATATYPE_DIFFERENT:
						// Datatype is different
						itrMyMetric->second = itrInputMetric.second;
						// Add data to a separate metric map for maintaining changes in BIRTH
						oMetricMap.emplace(itrInputMetric.first,
								itrInputMetric.second);
						break;
					}
				}
			}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);

	// Check if any changes occurred for BIRTH message
	if (0 != oMetricMap.size())
	{
		a_bIsOnlyValChange = false;
		return oMetricMap;
	}
	// No changes occurred for BIRTH
	// Check if any changes occurred for DATA change i.e. only change in value
	if (0 != oMetricMapData.size())
	{
		a_bIsOnlyValChange = true;
		return oMetricMapData;
	}
	// No changes occurred for BIRTH or DATA
	a_bIsOnlyValChange = false;
	return oMetricMap;
}

/**
 * Process new device data message from vendor app and extract all the information about metric
 * @param a_MetricList :[in] list of metrics present in a message payload received on internal MQTT broker
 * @return structured map of metrics
 */
metricMapIf_t CSparkPlugDev::processNewData(metricMapIf_t &a_MetricList)
{
	metricMapIf_t oMetricMap;
	do
	{
		try
		{
			std::lock_guard<std::mutex> lck(m_mutexMetricList);
			for (auto &itrInputMetric : a_MetricList)
			{
				if(nullptr == itrInputMetric.second)
				{
					DO_LOG_ERROR(itrInputMetric.first + ": Metric data is not built.");
					continue;
				}
				auto itrMyMetric = m_mapMetrics.find(itrInputMetric.first);
				if (m_mapMetrics.end() == itrMyMetric)
				{
					DO_LOG_ERROR(itrInputMetric.first + ": Metric not found in device: "
							+ m_sSparkPlugName	+ ". Ignoring this metric data");
				}
				else
				{
					if(nullptr == itrMyMetric->second)
					{
						DO_LOG_ERROR(itrMyMetric->first + ": Metric data is not found.");
						continue;
					}
					// Compare data type
					switch ((itrMyMetric->second)->compareValue(*(itrInputMetric.second)))
					{
					case SAMEVALUE_OR_DTATYPE:
						// No action
						break;

					case VALUES_DIFFERENT:
						// Assign value here
						(itrMyMetric->second)->assignNewValue(*(itrInputMetric.second));
						oMetricMap.emplace(itrInputMetric.first,
								itrInputMetric.second);
						break;

					case DATATYPE_DIFFERENT:
						// Datatype is different
						DO_LOG_ERROR(
								itrInputMetric.first
										+ ": Metric data-types differ in device: "
										+ m_sSparkPlugName
										+ ". Expected datatype:"
								+ std::to_string((itrMyMetric->second)->getDataType())
										+ ", Received datatype:"
								+ std::to_string((itrInputMetric.second)->getDataType())
								+ ". Ignoring this data");
						break;
					}
				}
			}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);

	return oMetricMap;
}

/**
 * Add metric to SprakPlugDev for Modbus device
 * @param a_rUniqueDataPoint :[in] reference of unique data point
 * @return none
 */
void CSparkPlugDev::addMetric(const network_info::CUniqueDataPoint &a_rUniqueDataPoint)
{
	do
	{
		try
		{
			std::lock_guard<std::mutex> lck(m_mutexMetricList);
			auto itrMyMetric = m_mapMetrics.find(a_rUniqueDataPoint.getID());

			if (m_mapMetrics.end() == itrMyMetric)
			{
				/** data type of datapoint specified in yml files*/
				std::string ymlDataType =  a_rUniqueDataPoint.getDataPoint().getAddress().m_sDataType;
				std::transform(ymlDataType.begin(), ymlDataType.end(), ymlDataType.begin(), ::tolower);

				auto getDataType = [](std::string a_sDataType) -> eYMlDataType
				{
				    if (! a_sDataType.compare("boolean"))
				    {
				        return enBOOLEAN;
				    }
				    else if ((! a_sDataType.compare("uint"))   ||
					         (! a_sDataType.compare("uint16")) ||
					         (! a_sDataType.compare("uint32")) ||
					         (! a_sDataType.compare("uint64")))
				    {
				        return enUINT;
				    }
				    else if ((! a_sDataType.compare("int"))   ||
					         (! a_sDataType.compare("int16")) ||
					         (! a_sDataType.compare("int32")) ||
					         (! a_sDataType.compare("int64")))
				    {
				        return enINT;
				    }
				    else if (! a_sDataType.compare("float"))
				    {
				        return enFLOAT;
				    }
				    else if (! a_sDataType.compare("double"))
				    {
				        return enDOUBLE;
				    }
				    else if (! a_sDataType.compare("string"))
					{
						return enSTRING;
					}
				    else
				    {
				     return enUNKNOWN;
				    }
				};

				eYMlDataType oYMlDataType = getDataType(ymlDataType);

				if (enUNKNOWN == oYMlDataType)
				{
					DO_LOG_ERROR("Invalid Yml Data Type. Ignored");
					return;
				}

				/** width value*/
				int ymlWidth = a_rUniqueDataPoint.getDataPoint().getAddress().m_iWidth;
				int metricDataType = METRIC_DATA_TYPE_UNKNOWN;
				int defaultIntVal = 0;
				float defaultFloatVal = 0.0;
				double defaultDoubleVal = 0.0;
				std::string defaultStringVal =  "";
				std::shared_ptr<CIfMetric> ptrIfMetric = std::make_shared<CMetric>(a_rUniqueDataPoint);			
				if(nullptr == ptrIfMetric)
				{
					DO_LOG_ERROR(a_rUniqueDataPoint.getID() + ": Unable to create shared metric.");
					return;
				}
				
				if (enSTRING == oYMlDataType)
				{
					metricDataType = METRIC_DATA_TYPE_STRING;
					CValObj objVal(metricDataType, defaultStringVal);
					ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);					
				}
				else 
				{
				switch(ymlWidth)
				{
					case WIDTH_ONE:

						if (enINT == oYMlDataType)
						{							 
							metricDataType = METRIC_DATA_TYPE_INT16;
							CValObj objVal(metricDataType, (int16_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);								 				 
						}
						else if (enUINT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_UINT16;
							CValObj objVal(metricDataType, (uint16_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else if (enBOOLEAN == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_BOOLEAN;
							CValObj objVal(metricDataType, true);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else
						{
							DO_LOG_ERROR("Invalid datatype for width 1. Ignored " + ymlDataType);
							return;
						}
							 
						break;

					case WIDTH_TWO:

						if (enINT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_INT32;
							CValObj objVal(metricDataType, (int32_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);								 
						}
						else if (enUINT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_UINT32;
							CValObj objVal(metricDataType, (uint32_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else if (enFLOAT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_FLOAT;
							CValObj objVal(metricDataType, (float)defaultFloatVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else
						{
							DO_LOG_ERROR("Invalid datatype for width 2. Ignored " + ymlDataType);
							return;
						}
						break;

					case WIDTH_FOUR:

						if (enINT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_INT64;
							CValObj objVal(metricDataType, (int64_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else if (enUINT == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_UINT64;
							CValObj objVal(metricDataType, (uint64_t)defaultIntVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}
						else if (enDOUBLE == oYMlDataType)
						{
							metricDataType = METRIC_DATA_TYPE_DOUBLE;
							CValObj objVal(metricDataType, (double)defaultDoubleVal);
							ptrIfMetric->getValue().assignNewDataTypeValue(metricDataType, objVal);
						}						
						else
						{
							DO_LOG_ERROR("Invalid datatype for width 4. Ignored " + ymlDataType);
							return;
						}

						break;					

					default:
						DO_LOG_ERROR("Unknown width. Ignored");
						break;

				};				
             }	
				m_mapMetrics.emplace(ptrIfMetric->getSparkPlugName(), ptrIfMetric);				
			}
			else
			{
				// Ignore this datapoint. It is already present.
				DO_LOG_ERROR(a_rUniqueDataPoint.getID() + " : Datapoint is already present in SparkPlug device: " 
								+ m_sSparkPlugName + ". Ignoring most recent instances.");
			}
		} catch (const std::exception &e)
		{
			DO_LOG_ERROR(std::string("Error:") + e.what());
		}
	} while (0);
}

/**
 * Prepare device birth messages to be published on SCADA system
 * @param a_rTahuPayload :[out] reference of spark plug message payload in which to store birth messages
 * @param a_mapMetrics: [in] list of metrics to be added in message
 * @param a_bIsBirth: [in] indicates whether DBIRTH is needed as a part of NBIRTH process
 * @return true/false depending on the success/failure
 */
bool CSparkPlugDev::prepareModbusMessage(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload, 
		const metricMapIf_t &a_mapMetrics, bool a_bIsBirth)
{
	try
	{
		// Check if it is Modbus device to process data
		if (true != std::holds_alternative<std::reference_wrapper<const network_info::CUniqueDataDevice>>(m_rDirectDevRef))
		{
			return false;
		}
		
		auto &orUniqueDev = std::get<std::reference_wrapper<const network_info::CUniqueDataDevice>>(m_rDirectDevRef);
		auto &rDev = orUniqueDev.get().getWellSiteDev().getDevInfo();
		
		org_eclipse_tahu_protobuf_Payload_Template udt_template = org_eclipse_tahu_protobuf_Payload_Template_init_default;
		udt_template.version = strndup(rDev.getDataPointsRef().getVersion().c_str(), rDev.getDataPointsRef().getVersion().length());
		udt_template.metrics_count = a_mapMetrics.size();
		udt_template.metrics = (org_eclipse_tahu_protobuf_Payload_Metric *) calloc(a_mapMetrics.size(), sizeof(org_eclipse_tahu_protobuf_Payload_Metric));
		std::string sYMLFilename{rDev.getDataPointsRef().getYMLFileName()};
		{
			std::size_t found = rDev.getDataPointsRef().getYMLFileName().rfind(".");
			if(found!=std::string::npos)
			{
				sYMLFilename.assign( rDev.getDataPointsRef().getYMLFileName().substr(0, found) );
			}
		}
		udt_template.template_ref = strndup(sYMLFilename.c_str(), sYMLFilename.length());
		udt_template.has_is_definition = true;
		udt_template.is_definition = false;
		int iLoop = 0;

		if(udt_template.metrics != NULL)
		{
			for(auto &itr: a_mapMetrics)
			{
				if(itr.second)
				{
					if(true !=  (itr.second)->addModbusMetric(udt_template.metrics[iLoop], a_bIsBirth))
					{
						DO_LOG_ERROR((itr.second)->getSparkPlugName() + ":Could not add metric to device. Trying to add other metrics.");
				}
					udt_template.metrics[iLoop].timestamp = (itr.second)->getTimestamp();
				udt_template.metrics[iLoop].has_timestamp = true;
				}
				++iLoop;
			}
		}
		if(true == a_bIsBirth)
		{
			std::string sProtocol{""};
			switch(orUniqueDev.get().getWellSiteDev().getAddressInfo().m_NwType)
			{
				case network_info::eNetworkType::eTCP:
				sProtocol.assign("Modbus TCP");				
				break;

				case network_info::eNetworkType::eRTU:
				sProtocol.assign("Modbus RTU");
				break;

				default:
				break;
			}

			CSCADAHandler::instance().addModbusPropForBirth(udt_template, sProtocol);
		}

		// Create the root UDT definition and add the UDT definition value which includes the UDT members and parameters
		org_eclipse_tahu_protobuf_Payload_Metric metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
		init_metric(&metric, orUniqueDev.get().getWellSiteDev().getID().c_str(), false, 0, METRIC_DATA_TYPE_TEMPLATE, false, false, &udt_template, sizeof(udt_template));
		metric.timestamp = get_current_timestamp();
		metric.has_timestamp = true;

		// Add the UDT to the payload
		add_metric_to_payload(&a_rTahuPayload, &metric);
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

/**
 * Prepare device birth messages to be published on SCADA system
 * @param a_rTahuPayload :[out] reference of spark plug message payload in which to store birth messages
 * @param a_bIsNBIRTHProcess: [in] indicates whether DBIRTH is needed as a part of NBIRTH process
 * @return true/false depending on the success/failure
 */
bool CSparkPlugDev::prepareDBirthMessage(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload, bool a_bIsNBIRTHProcess)
{
	try
	{
		std::lock_guard<std::mutex> lck(m_mutexMetricList);
		
		// Check if last known status of device is DOWN or not
		if(enDEVSTATUS_DOWN == getLastKnownDevStatus())
		{
			DO_LOG_INFO(m_sSparkPlugName + ": Last known dev status is DOWN. DBIRTH message is not prepared.");
			// Last know status is DOWN. Do NOT prepare a birth message
			return false;
		}
		// Check whether this is node (re)birth scenario
		if(false == a_bIsNBIRTHProcess)
		{
			// Following check should be done when DBIRTH is done as a part of device status and
			// and not when node (re)birth is happening
			// Check if last published status of device is UP or not
			if(enDEVSTATUS_UP == getLastPublishedDevStatus())
			{
				DO_LOG_INFO(m_sSparkPlugName + ": Last published dev status is UP. DBIRTH message is not prepared.");
				// Last know status is not UP. Do NOT prepare a birth message
				return false;
			}
		}
		if (true == std::holds_alternative<std::reference_wrapper<const network_info::CUniqueDataDevice>>(m_rDirectDevRef))
		{
			// For Modbus device
			prepareModbusMessage(a_rTahuPayload, m_mapMetrics, true);
		}
		else // For vendor app 
		{
			for(auto &itr: m_mapMetrics)
			{
				if(nullptr == itr.second)
				{
					continue;
				}
				uint64_t current_time = (itr.second)->getTimestamp();

				// org_eclipse_tahu_protobuf_Payload_Metric : Fields
				// char *name: NULL, 
				// bool has_alias: false, uint64_t alias: 0
				// bool has_timestamp: true, uint64_t timestamp: current_time
				// bool has_datatype: true, uint32_t datatype: itr.second.getDataType()
				// bool has_is_historical: false, bool is_historical: 0
				// bool has_is_transient: false, bool is_transient: 0
				// bool has_is_null: true, bool is_null: false
				// bool has_metadata: false, org_eclipse_tahu_protobuf_Payload_MetaData metadata: default
				// bool has_properties: false, org_eclipse_tahu_protobuf_Payload_PropertySet properties: default
				// pb_size_t which_value: 0, value: {0}
				org_eclipse_tahu_protobuf_Payload_Metric metric = {NULL, false, 0, true, current_time , true,
						(itr.second)->getDataType(), false, 0, false, 0, false, true, false,
						org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
						false, org_eclipse_tahu_protobuf_Payload_PropertySet_init_default, 0, {0}};

				if(true == (itr.second)->addMetricForBirth(metric))
				{
					add_metric_to_payload(&a_rTahuPayload, &metric);
				}
				else
				{
					DO_LOG_ERROR((itr.second)->getSparkPlugName() + ":Could not add metric to device. Trying to add other metrics.");
				}
			}
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

/**
 * Parses real device update message and stores metrics and corresponding values
 * @param a_sPayLoad :[in] payload containing metrics
 * @param a_sMetric :[out] stores value of "metric" key
 * @param a_sValue :[out] stores value of "value" key
 * @param a_sStatus :[out] stores value of "status" key
 * @param a_usec :[out] stores value of "usec" key
 * @param a_lastGoodUsec :[out] stores value of "lastGoodUsec" key
 * @param a_error_code :[out] stores value of "error_code" key
 * @return true or false based on success
 */
bool CSparkPlugDev::parseRealDeviceUpdateMsg(const std::string &a_sPayLoad, 
		std::string &a_sMetric, std::string &a_sValue, std::string &a_sStatus,
		uint64_t &a_usec, uint64_t &a_lastGoodUsec, uint32_t &a_error_code)
{
	cJSON *pjRoot = NULL;
	try
	{
		// Set initial values
		a_sMetric.clear();
		a_sValue.clear();
		a_sStatus.clear();
		a_usec = a_lastGoodUsec = a_error_code = 0;

		// Parse JSON file
		pjRoot = cJSON_Parse(a_sPayLoad.c_str());
		if (NULL == pjRoot)
		{
			DO_LOG_ERROR("Message received from MQTT could not be parsed in json format");
			return false;
		}

		// Lambda to read a parameter from JSON
		auto readFieldFromJSON = [](std::string &a_sFieldValue, std::string a_sFieldName, cJSON *a_cjRoot) -> bool
		{
			a_sFieldValue.clear();
			if(NULL != a_cjRoot)
			{
				cJSON *cjValue = cJSON_GetObjectItem(a_cjRoot, a_sFieldName.c_str());
				if(cjValue == NULL)
				{
					DO_LOG_DEBUG(a_sFieldName + ": Field not found in input JSON");
					return false;
				}
				else
				{
					char *pcValue = cJSON_GetStringValue(cjValue);
					if (pcValue == NULL)
					{
						DO_LOG_ERROR("Invalid payload: No value found for field: " + a_sFieldName);
						return false;
					}
					a_sFieldValue.assign(pcValue);
				}
			}
			return true;
		};

		// Read metric
		if(false == readFieldFromJSON(a_sMetric, "metric", pjRoot))
		{
			DO_LOG_ERROR("metric key not found in message: " + a_sPayLoad);
			return false;
		}

		// Read status
		if(false == readFieldFromJSON(a_sStatus, "status", pjRoot))
		{
			DO_LOG_ERROR("status key not found in message" + a_sPayLoad);
			return false;
		}
	
		// Read value
		if(false == readFieldFromJSON(a_sValue, "value", pjRoot))
		{
			DO_LOG_ERROR("value key not found in message" + a_sPayLoad);
			return false;
		}

		std::string sTemp{""};
		//timestamp is optional parameter
		if(false == readFieldFromJSON(sTemp, "usec", pjRoot))
		{
			a_usec = get_current_timestamp();
		}
		else
		{
			if(false == sTemp.empty())
			{
				std::chrono::microseconds dur_micro(std::stoul(sTemp));
				std::chrono::milliseconds dur_milli = std::chrono::duration_cast<std::chrono::milliseconds>(dur_micro);
				a_usec = dur_milli.count();
			}
			else
			{
				a_usec = get_current_timestamp();
			}
		}

		//lastGoodUsec is optional parameter
		if(true == readFieldFromJSON(sTemp, "lastGoodUsec", pjRoot))
		{
			if(false == sTemp.empty())
			{
				std::chrono::microseconds dur_micro(std::stoul(sTemp));
				std::chrono::milliseconds dur_milli = std::chrono::duration_cast<std::chrono::milliseconds>(dur_micro);
				a_lastGoodUsec = dur_milli.count();
			}			
		}

		//error_code is optional parameter
		if(true == readFieldFromJSON(sTemp, "error_code", pjRoot))
		{
			if(false == sTemp.empty())
			{
				a_error_code = std::stoul(sTemp);
			}
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
	}

	if (NULL != pjRoot)
	{
		cJSON_Delete(pjRoot);
	}

	return true;
}

/**
 * parseScaledValueRealDevices function will first parse scaled value field from JSON payload and compare the datatype of the scaledValue 
 * before assigning it to CValObj instance. This datatype and scaled value that is stored in CValObj instance is 
 * later used when metric parameters of tahu payload is initialized. Therafter, tahu payload is published to external mqtt.
 * @param a_sPayLoad :[in]  payload containing metrics
 * @param a_sMetric  :[in]  std::string &a_sMetric
 * @param a_rValobj  :[out] CValObj &a_rValobj
 * @return true or false based on success
 */
bool CSparkPlugDev::parseScaledValueRealDevices(const std::string &a_sPayLoad, std::string &a_sMetric, CValObj &a_rValobj)
{
	cJSON *pjRoot = NULL;
	//getDatatype of the datapoint
	pjRoot = cJSON_Parse(a_sPayLoad.c_str());

	// Below retVal flag is used to identify success/failure scenarios.
	bool retVal = false;
	if (NULL == pjRoot)
	{
		DO_LOG_ERROR("Message received from MQTT could not be parsed in json format");
		return retVal;
	}

	auto iter = m_mapMetrics.find(a_sMetric);
	uint32_t tempYmlDataType = METRIC_DATA_TYPE_UNKNOWN;

	if (m_mapMetrics.end() != iter)
	{
		if(nullptr == (iter->second))
		{
			DO_LOG_ERROR("Null value in Metric Data: " + a_sMetric);
			retVal = false;
		}
		else 
		{
			tempYmlDataType = (iter->second)->getValue().getDataType();
			retVal = true;
		}
	}
	else
	{
		DO_LOG_ERROR("Metric Not found : " + a_sMetric);
		retVal = false;
	}
        
	// Check if metric data is null then delete allocated pjRoot and return.
	// If metric data is not null then proceed for further actions.
	if (retVal == false)
	{
		if (NULL != pjRoot)
		{
			cJSON_Delete(pjRoot);
		}
		
		return retVal;
	}

	//readScaledValueFieldFromJSON
	cJSON *cjValue = cJSON_GetObjectItem(pjRoot, "scaledValue");
	if (NULL == cjValue)
	{
		DO_LOG_ERROR("scaledValue field not found in input json");
		retVal = false;
	}

	// Check if scaledValue is missing in received json then delete allocated pjRoot and return.
	// If scaledValue is present then proceed for further actions.
	if (retVal == false)
	{
		if (NULL != pjRoot)
		{
			cJSON_Delete(pjRoot);
		}
		
		return retVal;
	}
     
	if ((METRIC_DATA_TYPE_BOOLEAN == tempYmlDataType) && (1 == cJSON_IsBool(cjValue)))
	{
		if (cJSON_IsTrue(cjValue))
		{
			CValObj oValtemp(METRIC_DATA_TYPE_BOOLEAN, true);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_BOOLEAN, oValtemp);
		}
		else
		{
			CValObj oValtemp(METRIC_DATA_TYPE_BOOLEAN, false);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_BOOLEAN, oValtemp);
		}
		retVal = true;
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_UINT16 && (1 == cJSON_IsNumber(cjValue)))
	{
		if (cjValue->valuedouble < 0.0) {
			DO_LOG_ERROR("Negative value received for unsigned datatype uint16");
			retVal = false;
		}	
		else
		{
		
			CValObj oValtemp(METRIC_DATA_TYPE_UINT16, (uint16_t)cjValue->valuedouble);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT16, oValtemp);
			retVal = true;
		}
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_UINT32 && (1 == cJSON_IsNumber(cjValue)))
	{
		if (cjValue->valuedouble < 0.0) {
			DO_LOG_ERROR("Negative value received for unsigned datatype uint32");
			retVal = false;
		}
		else 
		{
			CValObj oValtemp(METRIC_DATA_TYPE_UINT32, (uint32_t)cjValue->valuedouble);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT32, oValtemp);
			retVal = true;
		}
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_UINT64 && (1 == cJSON_IsNumber(cjValue)))
	{
		if (cjValue->valuedouble < 0.0) {
			DO_LOG_ERROR("Negative value received for unsigned datatype uint64");
			retVal = false;
		}
		else
		{
			CValObj oValtemp(METRIC_DATA_TYPE_UINT64, (uint64_t)cjValue->valuedouble);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT64, oValtemp);
			retVal = true;
		}
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_INT16 && (1 == cJSON_IsNumber(cjValue)))
	{
		CValObj oValtemp(METRIC_DATA_TYPE_INT16, (int16_t)cjValue->valuedouble);
		a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_INT16, oValtemp);
		retVal = true;
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_INT32 && (1 == cJSON_IsNumber(cjValue)))
	{
		CValObj oValtemp(METRIC_DATA_TYPE_INT32, (int32_t)cjValue->valuedouble);
		a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_INT32, oValtemp);
		retVal = true;
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_INT64 && (1 == cJSON_IsNumber(cjValue)))
	{
		int64_t i64 = static_cast<std::int64_t>(cjValue->valuedouble);
		// Handle corner scenario of max value
		if((i64 < 0) && (cjValue->valuedouble > 0.0))
		{
			i64 = std::numeric_limits<int64_t>::max();
		}
		CValObj oValtemp(METRIC_DATA_TYPE_INT64, (int64_t)i64);
		a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_INT64, oValtemp);
		retVal = true;
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_STRING && (1 == cJSON_IsString(cjValue)))
	{
		char *val = cJSON_GetStringValue(cjValue);
		if (NULL == val) {
			DO_LOG_ERROR("Cannot get String value from Json payload");
			retVal = false;
		}
		else
		{
			std::string sTemp(val);
			CValObj oValtemp(METRIC_DATA_TYPE_STRING, sTemp);
			a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_STRING, oValtemp);
			retVal = true;
		}
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_FLOAT && (1 == cJSON_IsNumber(cjValue)))
	{
		float fval = static_cast<float>(cjValue->valuedouble);
		CValObj oValtemp(METRIC_DATA_TYPE_FLOAT, fval);
		a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_FLOAT, oValtemp);
		retVal = true;
	}
	else if (tempYmlDataType == METRIC_DATA_TYPE_DOUBLE && (1 == cJSON_IsNumber(cjValue)))
	{
		double dval = static_cast<double>(cjValue->valuedouble);
		CValObj oValtemp(METRIC_DATA_TYPE_DOUBLE, dval);
		a_rValobj.assignNewDataTypeValue(METRIC_DATA_TYPE_DOUBLE, oValtemp);
		retVal = true;
	}
	else
	{
		DO_LOG_ERROR(
				"Invalid data type or mismatch found. Mentioned type in Yml file : "
						+ std::to_string(tempYmlDataType) + ", Value datatype :"
						+ std::to_string(cjValue->type));
		retVal = false;
	}

	// Deallocate pjRoot
	if (NULL != pjRoot)
	{
		cJSON_Delete(pjRoot);
	}

	return retVal;
}

/**
 * Validates data parsed from update message of a real device
 * @param a_sValue :[in] value of "value" key
 * @param a_sStatus :[in] value of "status" key
 * @param a_error_code :[in] value of "error_code" key
 * @param a_bIsGood :[out] Indicates whether the status is good or bad
 * @param a_bIsDeathCode :[out] Indicates error code is for a device being unreachable
 * @return true or false based on success
 */
bool CSparkPlugDev::validateRealDeviceUpdateData(
		const std::string &a_sValue, std::string a_sStatus,
		uint32_t a_error_code,
		bool &a_bIsGood, bool &a_bIsDeathCode)
{
	try
	{
		// Check status
		a_bIsGood = true;
		a_bIsDeathCode = false;
		std::transform(a_sStatus.begin(), a_sStatus.end(), a_sStatus.begin(), ::tolower);
		if("good" != a_sStatus)
		{
			if("bad" != a_sStatus)
			{
				DO_LOG_ERROR("Unknown status. Ignoring the message");
				return false;
			}
			// Read error_code
			if(0 == a_error_code)
			{
				DO_LOG_ERROR("Either error_code key not found or value is 0 in message with bad status. Ignoring the message");
				return false;
			}

			// status is bad. Check error code
			a_bIsGood = false;
			
			// Following error codes are considered under device connection being lost
			// 2002	STACK_ERROR_SOCKET_FAILED
			// 2003	STACK_ERROR_CONNECT_FAILED
			// 2021	STACK_ERROR_SERIAL_PORT_ERROR

			// As of now, following are not considered under device connection being lost
			// 2004	STACK_ERROR_SEND_FAILED
			// 2005	STACK_ERROR_RECV_FAILED
			// 2006	STACK_ERROR_RECV_TIMEOUT => This does not necessarily mean that device is not reachable.
			switch(a_error_code)
			{
				case 2002:
				case 2003:
				case 2021:
				a_bIsDeathCode = true;
				break;

				default:
				a_bIsDeathCode = false;
				break;
			}
		}
		else 
		{
			// Check if value is present. 
			if(a_sValue.empty())
			{
				DO_LOG_ERROR("Value in string format is not present. Ignoring the message");
				return false;
			}
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}

	return true;
}

/**
 * Parses real device update message and stores metrics and corresponding values
 * @param a_sPayLoad :[in] payload containing metrics
 * @param a_stRefActionVec :[out] action vector
 * @return map containing metric and corresponding values
 */
bool CSparkPlugDev::processRealDeviceUpdateMsg(const std::string a_sPayLoad, std::vector<stRefForSparkPlugAction> &a_stRefActionVec)
{
	try
	{
		std::string sMetric{""}, sValue{""}, sStatus{""};
		uint64_t usec{0}, lastGoodUsec{0};
		uint32_t error_code{0};
		CValObj oValObj;
		bool bRet = parseRealDeviceUpdateMsg(a_sPayLoad, 
			sMetric, sValue, sStatus, usec, lastGoodUsec, error_code);
		if(false == bRet)
		{
			DO_LOG_ERROR("Unable to parse message: " + a_sPayLoad);
			return false;
		}

		// At this point, all required fields are retrieved to make decisions
		// metric-name, status, error-code (if any), value, usec, lastGoodUsec (if present)
		// Now validate these fields values
		bool bIsGood = true, bDeathErrorCode = false;
		bRet = validateRealDeviceUpdateData(sValue, sStatus, error_code, bIsGood, bDeathErrorCode);
		if(false == bRet)
		{
			DO_LOG_ERROR("Message validation failed: " + a_sPayLoad);
			return false;
		}
		
		std::lock_guard<std::mutex> lck(m_mutexMetricList);
		// Check if metric is a part of this device
		auto itrMyMetric = m_mapMetrics.find(sMetric);
		if (m_mapMetrics.end() == itrMyMetric)
		{
			DO_LOG_ERROR(sMetric + ": Metric not found in device: "
						+ m_sSparkPlugName + ". Ignoring this metric data");
			return false;
		}
		CMetric *pOtherMetric = dynamic_cast<CMetric*>(itrMyMetric->second.get());
		if(nullptr == pOtherMetric)
		{
			DO_LOG_ERROR(sMetric + ": Metric types do not match! : "
						+ m_sSparkPlugName + ". Ignoring this metric data");
			return false;
		}
		CMetric &refMyMetric = *pOtherMetric;
		if ( false == parseScaledValueRealDevices(a_sPayLoad, sMetric, oValObj))
		{
			DO_LOG_ERROR("Error in parseScaledValueRealDevices. ");
			return false;
		}

		// All validations are done
		// Now, action can be determined based on values in this message and 
		// last known status informed to SCADA Master
		//CValObj oValObj{sValue};
		//CValObj oValObj{(int16_t)ReadValFromJson};

		// Scenarios: last status to SCADA = DEVICE UP
		//		1. Now Good status and 
		//			a. change in value => DDATA
		//			b. no change in value => no action
		//		2. Now Bad status and 
		//			a. error code other than death scenario => no action
		//			b. error code meaning death scenario => DDEATH

		// Scenarios: last status to SCADA = DEVICE DOWN
		//		1. Now Good status => DBIRTH
		//		2. Now Bad status => NO action
		
		auto uiValCompareResult = refMyMetric.getValue().compareValue(oValObj);

		// Lambda to read a parameter from JSON
		auto addToActionVector = [&](eMsgAction a_eMsgAction) -> bool
		{
			metricMapIf_t mapChangedMetricsFromUpdate;
			if((enMSG_DATA == a_eMsgAction) || (enMSG_BIRTH == a_eMsgAction))
			{
				refMyMetric.setValue(oValObj);
				refMyMetric.setTimestamp(usec);

				if(enMSG_DATA == a_eMsgAction)
				{
					mapChangedMetricsFromUpdate.emplace((itrMyMetric->second)->getSparkPlugName(), (itrMyMetric->second));
				}
			}
			
			stRefForSparkPlugAction stDummyAction
				{ std::ref(*this), a_eMsgAction, mapChangedMetricsFromUpdate};
				a_stRefActionVec.push_back(stDummyAction);
			return true;
		};
		
		// Now to prepare action vector, check all scenarios
		switch(getLastPublishedDevStatus())
		{
			case enDEVSTATUS_NONE:
			// No action as of now. Wait for SCADA-RTU to send a 1st dbirth message
			break;

			case enDEVSTATUS_DOWN:
			if(true == bIsGood)
			{
				// Now it is a good data
				// case for dbirth
				addToActionVector(enMSG_BIRTH);
				setKnownDevStatus(enDEVSTATUS_UP);
			}
			else
			{
				// All other cases are ignored. No action

				// Check lastgoodsec and value. Set it to be used for next DBIRTH message
				if((0 != lastGoodUsec) && (VALUES_DIFFERENT == uiValCompareResult)
						&& (false == sValue.empty()))
				{
					refMyMetric.setTimestamp(lastGoodUsec);
					refMyMetric.setValue(oValObj);
				}
			}
			
			break;

			case enDEVSTATUS_UP:
			// Last report was: device is up
			if(true == bIsGood)
			{
				refMyMetric.setTimestamp(usec);

				if(VALUES_DIFFERENT == uiValCompareResult)
				{
					addToActionVector(enMSG_DATA);
					setKnownDevStatus(enDEVSTATUS_UP);
				}
			}
			else if((false == bIsGood) && (true == bDeathErrorCode))
			{
				// Check lastgoodsec and value. 
				if((0 != lastGoodUsec) && (VALUES_DIFFERENT == uiValCompareResult)
					&& (false == sValue.empty()))
				{
					// This case means there is some value which we did not have earlier.
					// Set it to be used for next DBIRTH message
					refMyMetric.setTimestamp(lastGoodUsec);
					refMyMetric.setValue(oValObj);
				}
				addToActionVector(enMSG_DEATH);
				setKnownDevStatus(enDEVSTATUS_DOWN);
			}
			else if(false == bIsGood)
			{
				// All other cases are ignored. No action
				// Check lastgoodsec and value. 
				if((0 != lastGoodUsec) && (VALUES_DIFFERENT == uiValCompareResult)
					&& (false == sValue.empty()))
				{
					addToActionVector(enMSG_DATA);
					setKnownDevStatus(enDEVSTATUS_UP);
				}
			}
			break;

			default:
			break;
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
	}

	return true;
}

/**
 * Prepare cJSON message for write-on-demand to publish on internal MQTT
 * @param a_sTopic :[in] topic on which to publish the message
 * @param a_root :[out] cJSON reference containing message payload
 * @param a_metrics :[in] changed metrics for which to publish WOD message
 * @param a_appSeqNo :[in] app seq number to be used
 * @return true/false based on success/failure
 */
bool CSparkPlugDev::getWriteMsg(std::string& a_sTopic, cJSON *a_root,
		std::pair<const std::string, std::shared_ptr<CIfMetric>> &a_metric,
		const int& a_appSeqNo)
{
	bool isRealDevice = true;
	try
	{
		if(nullptr == a_metric.second)
		{
			DO_LOG_ERROR("Metric data is not available.");
			return false;
		}

		std::string pubTopic = "";
		std::string pubMsg = "";

		vector<string> vParsedTopic = { };
		CCommon::getInstance().getTopicParts(getSparkPlugName(), vParsedTopic, "-");

		if(vParsedTopic.size() != 2)
		{
			DO_LOG_ERROR("Invalid device name to prepare WOD request");
			return false;
		}
		if(vParsedTopic[0].empty() || vParsedTopic[1].empty())
		{
			DO_LOG_ERROR("Invalid device name or site name to prepare WOD request");
			return false;
		}
		string strDevice = vParsedTopic[0];
		string strSite = vParsedTopic[1];
		string strMetricName = "";
		strMetricName.assign(a_metric.first);

		a_sTopic = "/" + strDevice + "/" + strSite + "/" + strMetricName + "/write";

		{
			string a_strDataPoint = (a_metric.second)->getName().c_str();

			cJSON_AddItemToObject(a_root, "wellhead", cJSON_CreateString(vParsedTopic[1].c_str()));
			cJSON_AddItemToObject(a_root, "command", cJSON_CreateString((a_metric.second)->getName().c_str()));

			if(false == (a_metric.second)->assignToCJSON(a_root, isRealDevice))
			{
				return false;
			}

			time_t t = (a_metric.second)->getTimestamp() / 1000;

			char cTimestamp[100];
			auto ptrTime = std::localtime(&t);
			if(NULL != ptrTime)
			{
				if (std::strftime(cTimestamp, sizeof(cTimestamp), "%F %T", ptrTime))
				{
					cJSON_AddItemToObject(a_root, "timestamp", cJSON_CreateString(cTimestamp));
				}
				else
				{
					DO_LOG_ERROR("Cannot assign timestamp in write-on-demand request");
					return false;
				}
			}

			cJSON_AddItemToObject(a_root, "usec", cJSON_CreateString(std::to_string((a_metric.second)->getTimestamp()).c_str()));

			cJSON_AddItemToObject(a_root, "version", cJSON_CreateString(CCommon::getInstance().getVersion().c_str()));
			std::string sAppSeq{"SCADA_RTU_" + std::to_string(a_appSeqNo)};
			cJSON_AddItemToObject(a_root, "app_seq", cJSON_CreateString(sAppSeq.c_str()));
		}
	}
	catch(std::exception& ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

/**
 * Prepare cJSON message for vendor app CMD msg to publish on internal MQTT
 * @param a_sTopic :[out] topic on which to publish the message
 * @param a_root :[out] cJSON reference containing message payload
 * @param a_metrics :[in] changed metrics for which to publish CMD message
 * @return true/false based on success/failure
 */
bool CSparkPlugDev::getCMDMsg(string& a_sTopic, metricMapIf_t& a_metrics, cJSON *metricArray)
{
	bool bRet = false;
	bool isRealDevice = false;
	try
	{
		vector<string> vParsedTopic = { };
		CCommon::getInstance().getTopicParts(getSparkPlugName(), vParsedTopic, "-");

		if(vParsedTopic.size() != 2)
		{
			DO_LOG_ERROR("Invalid device name to prepare CMD request");
			return false;
		}
		if(vParsedTopic[0].empty() || vParsedTopic[1].empty())
		{
			DO_LOG_ERROR("Invalid device name or site name to prepare CMD request");
			return false;
		}

		//assuming site name exists after the last occurrence of -
		string device = vParsedTopic[0];
		string siteName = vParsedTopic[1];
		a_sTopic = "CMD/" + device + "/" + siteName;

		//these shall be part of a single sparkplug msg
		for (auto &itrMetric : a_metrics)
		{
			if(nullptr == itrMetric.second)
			{
				DO_LOG_ERROR(itrMetric.first + ": Metric data not found.");
				continue;
			}
			cJSON *cjMetric = cJSON_CreateObject();
			if(cjMetric == NULL)
			{
				DO_LOG_ERROR("Error while creating CJSON message");
				return false;
			}

			cJSON_AddItemToObject(cjMetric, "name", cJSON_CreateString((itrMetric.second)->getName().c_str()));

			if(false == (itrMetric.second)->assignToCJSON(cjMetric, isRealDevice))
			{
				DO_LOG_ERROR("Failed to assign value to CJSON");
				return false;
			}

			cJSON_AddItemToObject(cjMetric, "timestamp", cJSON_CreateNumber((itrMetric.second)->getTimestamp()));

		    cJSON_AddItemToArray(metricArray, cjMetric);

		}//metric ends
		bRet = true;
	}
	catch(std::exception& ex)
	{
		DO_LOG_FATAL(ex.what());
		bRet = false;
	}
	return bRet;
}

/**
 * Prepare and publish a DDATA message in sparkplug format for a device 
 * @param a_payload :[out] sparkplug payload being created
 * @param a_mapChangedMetrics :[in] changed metrics for which ddata message to be created
 * @return true/false based on success/failure
 */
bool CSparkPlugDev::prepareDdataMsg(org_eclipse_tahu_protobuf_Payload &a_payload, const metricMapIf_t &a_mapChangedMetrics)
{
	bool bRet = false;

	try
	{

		if (true == std::holds_alternative<std::reference_wrapper<const network_info::CUniqueDataDevice>>(m_rDirectDevRef))
		{
			// For Modbus device
			bRet = prepareModbusMessage(a_payload, a_mapChangedMetrics, false);
		}
		else // For vendor app 
		{
			for(auto &itrMetric: a_mapChangedMetrics)
			{
				if(nullptr == itrMetric.second)
				{
					DO_LOG_ERROR(itrMetric.first + ": Metric data not found.");
					continue;
				}
				uint64_t timestamp = (itrMetric.second)->getTimestamp();
				string strMetricName = (itrMetric.second)->getName();

				org_eclipse_tahu_protobuf_Payload_Metric metric =
					{ NULL, false, 0, true, timestamp, true,
					(itrMetric.second)->getDataType(), false, 0, false, 0, false,
					true, false, org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
					false,	org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
					0, { 0 } };

				if(false == (itrMetric.second)->addMetricNameValue(metric))
				{
					DO_LOG_ERROR((itrMetric.second)->getName() + ":Failed to add metric name and value");
				}
				else
				{
					add_metric_to_payload(&a_payload, &metric);
					bRet = true;
				}
			}
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return bRet;
}
