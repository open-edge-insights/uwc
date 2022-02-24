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

#include "KPIAppConfigMgr.hpp"
#include "YamlUtil.hpp"
#include "ConfigManager.hpp"


/**
 * Parses configuration YML file for KPI App
 * @param a_sFileName	:[in] YML file name
 * @return true/false based on success/failure
 */
bool CKPIAppConfig::parseYMLFile(const std::string &a_sFileName)
{
	try
	{
		YAML::Node node = CommonUtils::loadYamlFile(a_sFileName);

		if(0 != globalConfig::validateParam(node, "timeToRun_Minutes", globalConfig::DT_UNSIGNED_INT))
		{
			m_uiExecTimeMin = 0; // default value. Allow infinite execution
		}
		else
		{
			m_uiExecTimeMin = node["timeToRun_Minutes"].as<std::uint32_t>();
		}

		if(0 != globalConfig::validateParam(node, "isMQTTModeApp", globalConfig::DT_BOOL))
		{
			m_bIsMQTTModeApp = false;
		}
		else
		{
			m_bIsMQTTModeApp = node["isMQTTModeApp"].as<bool>();
		}

		if(0 != globalConfig::validateParam(node, "isRTModeForPolledPoints", globalConfig::DT_BOOL))
		{
			m_bIsRTModeForPolledPoints = true;
		}
		else
		{
			m_bIsRTModeForPolledPoints = node["isRTModeForPolledPoints"].as<bool>();
		}

		if(0 != globalConfig::validateParam(node, "isRTModeForWriteOp", globalConfig::DT_BOOL))
		{
			m_bIsRTModeForWriteOp = true;
		}
		else
		{
			m_bIsRTModeForWriteOp = node["isRTModeForWriteOp"].as<bool>();
		}

		for (auto it : node)
		{
			if(it.second.IsSequence() && it.first.as<std::string>() == "controlLoopDataPointMapping")
			{
				const YAML::Node& points =  it.second;
				for (auto it1 : points)
				{
					std::string strPolledTopic{""}, strWriteTopic{""}, strVal{""};
					uint32_t u32iDelayMs = 5;

					/// Polled Point
					if(0 == globalConfig::validateParam(it1, "polled_point", globalConfig::DT_STRING))
					{
						strPolledTopic = it1["polled_point"].as<std::string>();
					}

					/// Delay in msec
					if(0 == globalConfig::validateParam(it1, "delay_msec", globalConfig::DT_UNSIGNED_INT))
					{
						u32iDelayMs = it1["delay_msec"].as<std::uint32_t>();
					}

					/// Write data point
					if(0 == globalConfig::validateParam(it1["write_operation"], "datapoint", globalConfig::DT_STRING))
					{
						strWriteTopic = it1["write_operation"]["datapoint"].as<std::string>();
					}

					/// Write value
					if(0 == globalConfig::validateParam(it1["write_operation"], "dataval", globalConfig::DT_STRING))
					{
						strVal = it1["write_operation"]["dataval"].as<std::string>();
					}

					if(strPolledTopic.empty() || strWriteTopic.empty() || strVal.empty())
					{
						DO_LOG_ERROR("polled_point or write_operation/datapoint or write_operation/dataval is not present as a string. Ignoring this loop.");
						std::cout << "polled_point or write_operation/datapoint or write_operation/dataval is not present as a string. Ignoring this loop.\n";
						continue;
					}
					else
					{
						m_oCtrlLoopMap.insertControlLoopData(strPolledTopic,
							strWriteTopic,
							u32iDelayMs,
							strVal);
					}
				}
			}
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}

	return true;
}

