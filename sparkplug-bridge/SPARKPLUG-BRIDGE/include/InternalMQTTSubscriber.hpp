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

/*** InternalMQTTSubsciber.hpp is for internal mqtt */

#ifndef MQTT_HANDLER_HPP_
#define MQTT_HANDLER_HPP_

#include <semaphore.h>
#include "mqtt/async_client.h"
#include "MQTTPubSubClient.hpp"
#include "Logger.hpp"
#include "QueueMgr.hpp"
#include "SparkPlugDevMgr.hpp"
#include "ZmqHandler.hpp"
/** enumerator specifying mqtt connection status*/
enum eIntMQTTConStatus
{
	enCON_NONE, enCON_UP, enCON_DOWN
};

/** Handler class for internal mqtt operation*/
class CIntMqttHandler : public CMQTTBaseHandler
{
	int m_appSeqNo; /**app sequence number*/

	sem_t m_semConnSuccess; /** semaphore for connection success*/
	sem_t m_semConnLost; /** semphore for connection lost*/
	sem_t m_semConnSuccessToTimeOut; /** semaphore for connection success to timeout*/

	std::atomic<eIntMQTTConStatus> m_enLastConStatus; /** last connection status*/
	std::atomic<bool> m_bIsInTimeoutState; /** Timeout status*/

	/** constructor*/
	CIntMqttHandler(const std::string &strPlBusUrl, int iQOS);

	/** delete copy and move constructors and assign operators*/
	CIntMqttHandler(const CIntMqttHandler&) = delete;	 			/** Copy construct*/
	CIntMqttHandler& operator=(const CIntMqttHandler&) = delete;	/** Copy assign*/

	int getAppSeqNo();

	void subscribeTopics();
	void connected(const std::string &a_sCause) override;
	void disconnected(const std::string &a_sCause) override;
	void msgRcvd(mqtt::const_message_ptr a_pMsg) override;
	bool init();

	void handleConnMonitoringThread();
	void handleConnSuccessThread();

	/**function to set last connection status*/
	void setLastConStatus(eIntMQTTConStatus a_ConsStatus)
	{
		m_enLastConStatus.store(a_ConsStatus);
	}

	/**Function to get last connection status*/
	eIntMQTTConStatus getLastConStatus()
	{
		return m_enLastConStatus.load();
	}

	/**To set connection timeout status*/
	void setConTimeoutState(bool a_bFlag)
	{
		m_bIsInTimeoutState.store(a_bFlag);
	}

	/** to get connection timeout status*/
	bool getConTimeoutState()
	{
		return m_bIsInTimeoutState.load();
	}

#ifdef UNIT_TEST
	friend class InternalMQTTSubscriber_ut;
#endif


public:
	~CIntMqttHandler();/** destructor*/
	static CIntMqttHandler& instance(); /**function to get single instance of this class*/
	bool prepareCJSONMsg(std::vector<stRefForSparkPlugAction>& a_stRefActionVec);
	bool prepareCMDMsg(std::reference_wrapper<CSparkPlugDev>& a_refSparkPlugDev,
						metricMapIf_t& a_mapChangedMetrics);
	bool prepareWriteMsg(std::reference_wrapper<CSparkPlugDev>& a_refSparkPlugDev,
							metricMapIf_t& a_mapChangedMetrics);
        bool publish_msg_to_eii(string strPubMsg,string strMsgTopic);
};

#endif
