/*************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation. Title to the
* Material remains with Intel Corporation.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
*************************************************************************************/

#ifndef INCLUDE_COMMON_HPP_
#define INCLUDE_COMMON_HPP_

#include <string>
#include <map>
#include <algorithm>
#include "Logger.hpp"
#include "ConfigManager.hpp"

using namespace std;
using namespace globalConfig;

#define SPARKPLUG_TOPIC "spBv1.0/"

class CCommon
{
private:
	// Private constructor so that no objects can be created.
	CCommon();
	CCommon(const CCommon & obj){}
	CCommon& operator=(CCommon const&);

	string m_strAppName;
	string m_strMqttURL;
	string m_siteListFileName;
	string m_strNodeConfPath;
	string m_strNetworkType;
	string m_strGroupId;
	string m_strEdgeNodeID;
	bool m_devMode;

	void setScadaRTUIds();
	string getMACAddress(const string& a_strInterfaceName);

public:
	~CCommon();
	bool readCommonEnvVariables();
	bool readEnvVariable(const char *pEnvVarName, std::string &storeVal);

	/**
	 * Set application name
	 * @param strAppName :[in] Application name to set
	 * @return None
	 */
	void setStrAppName(const std::string &strAppName)
	{
		m_strAppName = strAppName;
	}

	/**
	 * Get application name
	 * @param None
	 * @return application name in string
	 */
	const std::string& getStrAppName() const
	{
		return m_strAppName;
	}

	/**
	 * Set MQTT Export URL to connect with MQTT broker
	 * @param strMqttExportURL
	 * @return None
	 */
	void setStrMqttURL(const std::string &strMqttURL)
	{
		m_strMqttURL = strMqttURL;
	}

	/**
	 * Get MQTT-Export broker connection URL
	 * @param None
	 * @return connection URL in string
	 */
	const std::string& getStrMqttURL() const
	{
		return m_strMqttURL;
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
	 * Check if application set for dev mode or not
	 * @param None
	 * @return true if set to devMode
	 * 			false if not set to devMode
	 */
	bool isDevMode() const
	{
		return m_devMode;
	}

	/**
	 * Set dev mode
	 * @param devMode :[in] value to set for dev_mode
	 * @return None
	 */
	void setDevMode(bool devMode)
	{
		m_devMode = devMode;
	}

	/**
	 * get site list from file name
	 * @return site list
	 */
	const std::string& getSiteListFileName() const
	{
		return m_siteListFileName;
	}

	/**
	 * set site list from file name
	 * @param siteListFileName	:[in] site list to set
	 */
	void setSiteListFileName(const std::string &siteListFileName)
	{
		m_siteListFileName = siteListFileName;
	}

	/**
	 * Return topic in sparkplug format to set in will message
	 * in mqtt subscriber
	 * @return death topic in string
	 */
	std::string getDeathTopic()
	{
		std::string topic(SPARKPLUG_TOPIC);
		topic.append(m_strGroupId);
		topic.append("/NDEATH/" + getEdgeNodeID());

		return topic;
	}

	/**
	 * Return topic in sparkplug format to send as node birth
	 * to SCADA
	 * @return nbirth topic in string
	 */
	std::string getNBirthTopic()
	{
		std::string topic(SPARKPLUG_TOPIC);
		topic.append(m_strGroupId);
		topic.append("/NBIRTH/" + getEdgeNodeID());

		return topic;
	}

	/**
	 * Return topic in sparkplug format to send as device birth
	 * to SCADA
	 * @return dbirth topic in string
	 */
	std::string getDBirthTopic()
	{
		std::string topic(SPARKPLUG_TOPIC);
		topic.append(m_strGroupId);
		topic.append("/DBIRTH/" + getEdgeNodeID() + "/");

		return topic;
	}

	/**
	 * Get edge node id of scada-rtu
	 * @return edge node id of scada-rtu in string
	 */
	std::string getEdgeNodeID()
	{
		return m_strEdgeNodeID;
	}

	/**
	 * get network type
	 * @return network type in string (ALL, TCP or RTU)
	 */
	const std::string& getNetworkType() const
	{
		return m_strNetworkType;
	}

	/**
	 * Set network type
	 * @param strNodeConfPath	:[in] node configuration file path to set
	 */
	void setNetworkType(const std::string &strNetworkType)
	{
		m_strNetworkType = strNetworkType;
	}
};
#endif
