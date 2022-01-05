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

#include "MqttHandler.hpp"
#include "Logger.hpp"
#include "CommonDataShare.hpp"
#include "EnvironmentVarHandler.hpp"
#include "KPIAppConfigMgr.hpp"

#define SUBSCRIBER_ID "_KPI_SUBSCRIBER"

extern std::atomic<bool> g_stopThread;

/**
 * Constructor Initializes MQTT publisher
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param strClientID :[in] client ID with which to subscribe (this is topic name)
 * @param iQOS :[in] QOS value with which publisher will publish messages
 * @return None
 */
CMqttHandler::CMqttHandler(const std::string &strPlBusUrl, int iQOS):
	CMQTTBaseHandler(strPlBusUrl, EnvironmentInfo::getInstance().getDataFromEnvMap("AppName")+SUBSCRIBER_ID, 
	iQOS, (false == CcommonEnvManager::Instance().getDevMode()),
	"/run/secrets/rootca/cacert.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem", 
	"/run/secrets/mymqttcerts/mymqttcerts_client_key.pem", "MQTTSubListener")
{
	try
	{
		DO_LOG_DEBUG("MQTT initialized successfully");
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Maintain single instance of this class
 * @param None
 * @return Reference of this instance of this class, if successful;
 * 			Application exits in case of failure
 */
CMqttHandler& CMqttHandler::instance()
{
	static bool bIsFirst = true;
	static std::string strPlBusUrl = EnvironmentInfo::getInstance().getDataFromEnvMap("MQTT_URL");
	static int nQos = 1;

	if(bIsFirst)
	{
		if(strPlBusUrl.empty())
		{
			DO_LOG_ERROR("MQTT_URL Environment variable is not set");
			std::cout << __func__ << ":" << __LINE__ << " Error : MQTT_URL Environment variable is not set" <<  std::endl;
			throw std::runtime_error("Missing required config..");
		}
	}

	DO_LOG_DEBUG("Internal MQTT subscriber is connecting with QOS : " + std::to_string(nQos));
	static CMqttHandler handler(strPlBusUrl.c_str(), nQos);

	if(bIsFirst)
	{
		handler.init();
		handler.connect();
		bIsFirst = false;
	}
	return handler;
}

/**
 * Subscribe to required topics for SCADA MQTT
 * @param none
 * @return none
 */
void CMqttHandler::subscribeTopics()
{
	try
	{
		auto subList = [&](const std::vector<std::string> &a_sList) 
		{
			for(auto &itr: a_sList)
			{
				m_MQTTClient.subscribe(itr);
			}
		};
		
		subList(CKPIAppConfig::getInstance().getControlLoopMapper().getPollingTopics());
		subList(CKPIAppConfig::getInstance().getControlLoopMapper().getWrRspTopics());
		
		DO_LOG_DEBUG("Subscribed topics with internal broker");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a callback function which gets called when subscriber is connected with MQTT broker
 * @param a_sCause :[in] reason for connect
 * @return None
 */
void CMqttHandler::connected(const std::string &a_sCause)
{
	try
	{
		sem_post(&m_semConnSuccess);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a callback function which gets called when a msg is received
 * @param a_pMsg :[in] pointer to received message
 * @return None
 */
void CMqttHandler::msgRcvd(mqtt::const_message_ptr a_pMsg)
{
	try
	{
		pushMsgInQ(a_pMsg);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a singleton class. Used to handle communication with SCADA master
 * through external MQTT.
 * Initiates number of threads, semaphores for handling connection successful
 * and internal MQTT connection lost scenario.
 * @param None
 * @return true on successful init
 */
bool CMqttHandler::init()
{
	// Initiate semaphore for requests
	int retVal = sem_init(&m_semConnSuccess, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_ERROR("Could not create semaphore for success connection");
		return false;
	}

	std::thread{ std::bind(&CMqttHandler::handleConnSuccessThread,
			std::ref(*this)) }.detach();

	return true;
}

/**
 * Thread function to handle SCADA connection success scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CMqttHandler::handleConnSuccessThread()
{
	while(false == g_stopThread.load())
	{
		try
		{
			do
			{
				if((sem_wait(&m_semConnSuccess)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_stopThread.load())
				{
					break;
				}

				if(true == m_MQTTClient.isConnected())
				{
					// Client is connected. 
					// Semaphore got signalled means connection is successful
					// As a process first subscribe to topics
					subscribeTopics();
				}
			} while(0);

		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("failed to initiate request :: " + std::string(e.what()));
		}
	}
}

/**
 * Push message in message queue to send on EII
 * @param msg :[in] reference of message to push in queue
 * @return true/false based on success/failure
 */
bool CMqttHandler::pushMsgInQ(mqtt::const_message_ptr& a_msgMQTT)
{
	bool bRet = true;
	try
	{
		auto endsWith = [](const std::string &a_sMain, const std::string &a_sToMatch)
		{
			if((a_sMain.size() >= a_sToMatch.size()) && 
				(a_sMain.compare(a_sMain.size() - a_sToMatch.size(), a_sToMatch.size(), a_sToMatch) == 0))
			{
				return true;
			}
			else
			{
				return false;
			}
		};
		std::string sTopic{a_msgMQTT->get_topic()};
		// Decide whether to use polling queue or writeResponse queue
		if(true == endsWith(sTopic, "/update"))
		{
			// For now put msg in polling queue
			CMessageObject oMsg{a_msgMQTT};
			QMgr::PollMsgQ().pushMsg(oMsg);
			bRet = true;
		}
		else if(true == endsWith(sTopic, "/writeResponse"))
		{
			// For now put msg in polling queue
			CMessageObject oMsg{a_msgMQTT};
			QMgr::WriteRespMsgQ().pushMsg(oMsg);
			bRet = true;
		}
		else
		{
			DO_LOG_INFO(sTopic + " : does not match pattern. Ignored.");
		}		
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		bRet = false;
	}
	return bRet;
}

/**
 * Destructor
 */
CMqttHandler::~CMqttHandler()
{
	sem_destroy(&m_semConnSuccess);
}
