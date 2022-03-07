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

#include "Common.hpp"
#include <iterator>
#include <iostream>
#include <fstream>
#include <bits/stdc++.h>
/**
 * Constructor initializes CCommon instance and retrieves common environment variables
 * @param None
 * @return None if successful;
 * 			In case of error or exception, application exits
 */
CCommon::CCommon() :
m_strExtMqttURL{""}, m_nQos{1}, m_strNodeConfPath{""},
m_strGroupId{""}, m_strNodeName{""}, m_bIsScadaTLS{true}
{
	setScadaRTUIds();

	if(false == EnvironmentInfo::getInstance().readCommonEnvVariables(m_vecEnv))
	{
		DO_LOG_ERROR("Error while reading the common environment variables");
		exit(1);
	}
}

/**
 * load config required for sparkplug-bridge container from sparkplug-bridge_config.yml file
 * @param None
 * @return true/false based on condition
 */
bool CCommon::loadYMLConfig()
{
	YAML::Node config;
	bool bRet = false;

	try
	{
		config = YAML::LoadFile(SPARKPLUG_BRIDGE_CONFIG_FILE_PATH);
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR("sparkplug-bridge_config.yml file not found :: " + std::string(e.what()));
		
		return false;
	}

	do
	{
		std::string sTemp{""};

		if((config["isTLS"])
				&& 0 == globalConfig::validateParam(config, "isTLS", globalConfig::eDataType::DT_BOOL))
		{
			if(config["isTLS"].as<bool>() == true)
			{
				sTemp = "ssl://";
				setScadaTLS(true);
				DO_LOG_INFO("Set the scada external broker to TLS");
			}
			else
			{
				sTemp = "tcp://";
				setScadaTLS(false);
				DO_LOG_INFO("Set the scada external broker to non-TLS");
			}
		}
		else
		{
			DO_LOG_ERROR("isTLS key is not present or type is invalid");
			bRet = false;
			break;
		}

		if((config["mqttServerAddrSCADA"])
				&& 0 == globalConfig::validateParam(config, "mqttServerAddrSCADA", globalConfig::eDataType::DT_STRING))
		{
			sTemp += config["mqttServerAddrSCADA"].as<std::string>();
		}
		else
		{
			DO_LOG_ERROR("mqttServerAddrSCADA key is not present or type is invalid");
			bRet = false;
			break;
		}

		if((config["mqttServerPortSCADA"])
				&& 0 == globalConfig::validateParam(config, "mqttServerPortSCADA", globalConfig::eDataType::DT_STRING))
		{
			sTemp += (":" + config["mqttServerPortSCADA"].as<std::string>());
			setExtMqttURL(sTemp);
			DO_LOG_INFO("EXTERNAL_MQTT_URL is set to :: " + getExtMqttURL())
			bRet = true;
		}
		else
		{
			DO_LOG_ERROR("mqttServerPortSCADA key is not present or type is invalid");
			bRet = false;
			break;
		}
	} while(0);

	if((config["qos"])
			&& 0 == globalConfig::validateParam(config, "qos", globalConfig::eDataType::DT_INTEGER))
	{
		int nQos = config["qos"].as<std::int32_t>();
		setMQTTQos(nQos);
		DO_LOG_INFO("QOS is set to :: " + std::to_string(getMQTTQos()));
	}
	else
	{
		setMQTTQos(1);	// default QOS value is 1
		DO_LOG_ERROR("QOS key is not present, set to default value " + std::to_string(getMQTTQos()));
	}

	return bRet;
}

/**
 * load global config file and set groupId and NodeName
 * @param None
 * @return None
 */
void CCommon::setScadaRTUIds()
{
	// load global configuration for container real-time setting
	bool bRetVal = globalConfig::loadGlobalConfigurations();
	if(!bRetVal)
	{
		DO_LOG_INFO("Global configuration is set with some default parameters");
	}
	else
	{
		DO_LOG_INFO("Global configuration is set successfully");
	}

	m_strGroupId = globalConfig::CGlobalConfig::getInstance().getSparkPlugInfo().getGroupId();
	if(m_strGroupId.empty())
	{
		DO_LOG_ERROR("Group id for scada-rtu is not set, exiting application");
		return;
	}

	m_strNodeName = globalConfig::CGlobalConfig::getInstance().getSparkPlugInfo().getNodeName();
	if(m_strNodeName.empty())
	{
		DO_LOG_ERROR("Node name for scada-rtu is not set, exiting application");
		return;
	}
}

/**
 * Destructor
 */
CCommon::~CCommon()
{
}

/**
 * Splits topic name with given delimeter 
 * @param a_sTopic :[in] topic to split
 * @param a_vsTopicParts :[out] stores all the split names in this vector
 * @param a_delimeter :[in] delimiter to be used for splitting string 
 * @return true/false based on true/false
 */
bool CCommon::getTopicParts(std::string a_sTopic,
		std::vector<std::string> &a_vsTopicParts, const std::string& a_delimeter)
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
* Get current time in nano seconds
* @param ts :[in] timestamp to get microsecond value
* @return time in micro seconds
*/
unsigned long CCommon::get_micros(struct timespec ts)
{
    return (unsigned long)ts.tv_sec * 1000000L + ts.tv_nsec/1000;
}
/**
 * To set the time stamp of the msg recived from SCADA 
 * @param :[in] None
 * @return none
 */
void CCommon::set_timestamp()
{
	struct timespec tsSPMsgReceived;
	timespec_get(&tsSPMsgReceived, TIME_UTC);	
	TS_SCADAtoMQTT = std::to_string(CCommon::get_micros(tsSPMsgReceived)); 
}
/**
 * To get the time stamp of the msg recived from SCADA 
 * @param :[in] None 
 * @return timestamp value 
 */
std::string CCommon::get_timestamp(){
	return TS_SCADAtoMQTT;
}
