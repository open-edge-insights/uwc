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

/*** Common.hpp for common operations of scada-RTU*/

#ifndef INCLUDE_COMMON_HPP_
#define INCLUDE_COMMON_HPP_

#include <string>
#include <map>
#include <algorithm>
#include "Constants.hpp"
#include "Logger.hpp"
#include "ConfigManager.hpp"
#include "EnvironmentVarHandler.hpp"
using namespace globalConfig;

#define SPARKPLUG_BRIDGE_CONFIG_FILE_PATH "/opt/intel/eii/uwc_data/sparkplug-bridge/sparkplug-bridge_config.yml"
#define SCADA_SPARKPLUG_VERSION "2.0"

/** class handling common operations*/
class CCommon
{
private:
	/** vector of env variables*/
	std::vector<std::string> m_vecEnv{"AppName", "INTERNAL_MQTT_URL", "DEV_MODE", "NETWORK_TYPE", "DEVICES_GROUP_LIST_FILE_NAME"};
	// a variable to store the timestamp values of msgs coming from SCADA
	std::string TS_SCADAtoMQTT;
	// Private constructor so that no objects can be created.
	CCommon();
	CCommon(const CCommon & obj) = delete;
	CCommon& operator=(CCommon const&) = delete;

	std::string m_strExtMqttURL; /** external mqtt url*/
	int m_nQos; /** qos value*/
	std::string m_strNodeConfPath; /** node configuration path*/
	std::string m_strGroupId; /** value of group ID*/
	std::string m_strNodeName; /** node name*/
	bool m_bIsScadaTLS; /** scada TLS(true or false)*/

	void setScadaRTUIds();

	/**
	 * Return topic in sparkplug format to send as device data
	 * to SCADA
	 * @return data topic in string
	 */
	std::string getSparkPlugTopic(const std::string &a_sTopic)
	{
		std::string sTopic{SPARKPLUG_TOPIC + m_strGroupId + "/" + a_sTopic + "/" + m_strNodeName};
		return sTopic;
	}

public:
	~CCommon();//destructor

	bool loadYMLConfig();

	/**
	 * Set MQTT Export URL to connect with MQTT broker
	 * @param strMqttExportURL
	 * @return None
	 */
	void setExtMqttURL(const std::string &strMqttURL)
	{
		m_strExtMqttURL = strMqttURL;
	}

	/**
	 * Get MQTT-Export broker connection URL
	 * @param None
	 * @return connection URL in string
	 */
	const std::string& getExtMqttURL() const
	{
		return m_strExtMqttURL;
	}

	/**
	 * Set MQTT QOS to connect with MQTT broker
	 * @param nQos
	 * @return None
	 */
	void setMQTTQos(const int nQos)
	{
		switch(nQos)
		{
		case 0:
		case 1:
		case 2:
			m_nQos = nQos;
			break;
		default:
			m_nQos = 1;
			break;
		}
	}

	/**
	 * Get MQTT-Export broker connection QOS parameter
	 * @param None
	 * @return connection QOS in int
	 */
	const int getMQTTQos() const
	{
		return m_nQos;
	}

	/**
	 * Get single instance of this class
	 * @param None
	 * @return this instance of CCommon class
	 */
	static CCommon& getInstance()
	{
		static CCommon _self;
			return _self;
	}

	/**
	 * Return topic in sparkplug format to set in will message
	 * in mqtt subscriber
	 * @return death topic in string
	 */
	std::string getDeathTopic()
	{
		return getSparkPlugTopic(NDEATH);
	}

	/**
	 * Return topic in sparkplug format to send as node birth
	 * to SCADA
	 * @return nbirth topic in string
	 */
	std::string getNBirthTopic()
	{
		return getSparkPlugTopic(NBIRTH);
	}

	/**
	 * Return topic in sparkplug format to send as device birth
	 * to SCADA
	 * @return dbirth topic in string
	 */
	std::string getDBirthTopic()
	{
		return getSparkPlugTopic(DBIRTH);
	}

	/**
	 * Return topic in sparkplug format to send as device data
	 * to SCADA
	 * @return data topic in string
	 */
	std::string getDDataTopic()
	{
		return getSparkPlugTopic(DDATA);
	}

	/**
	 * Return topic in sparkplug format to send as device death
	 * to SCADA
	 * @return death topic in string
	 */
	std::string getDDeathTopic()
	{
		return getSparkPlugTopic(DDEATH);
	}

	/**
	 * Return topic in sparkplug format to receive as device cmd
	 * from SCADA
	 * @return dcmd topic in string
	 */
	std::string getDCmdTopic()
	{
		return getSparkPlugTopic(DCMD);
	}

	/**
	 * Return topic in sparkplug format to receive as node cmd
	 * from SCADA
	 * @return ncmd topic in string
	 */
	std::string getNCmdTopic()
	{
		return getSparkPlugTopic(NCMD);
	}

	/**
	 * Get group name of scada-rtu
	 * @return group name of scada-rtu in string
	 */
	std::string getGroupName()
	{
		return m_strGroupId;
	}

	/**
	 * Get edge node id of scada-rtu
	 * @return edge node id of scada-rtu in string
	 */
	std::string getNodeName()
	{
		return m_strNodeName;
	}

	/**
	 * Returns sparkplug version to be sent in message
	 * @return version string
	 */
	const std::string getVersion()
	{
		return std::string(SCADA_SPARKPLUG_VERSION);
	}

	/**
	 * Check if application set for TLS mode or not
	 * @param None
	 * @return true if set to TLS
	 * 			false if set to non-TLS
	 */
	bool isScadaTLS() const
	{
		return m_bIsScadaTLS;
	}

	/**
	 * Set TLS mode for scada
	 * @param a_bIsTLS :[in] value to set for TLS/Non-TLS
	 * @return None
	 */
	void setScadaTLS(bool a_bIsTLS)
	{
		m_bIsScadaTLS = a_bIsTLS;
	}

	bool getTopicParts(std::string a_sTopic, std::vector<std::string> &a_vsTopicParts, const std::string& a_delimeter);
	std::string get_timestamp();
	void set_timestamp();
	unsigned long get_micros(struct timespec ts);
};
#endif
