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

#include "MQTTSubscribeHandler.hpp"
#include "Logger.hpp"
#include "Common.hpp"
#include "cjson/cJSON.h"
#include "CommonDataShare.hpp"
#include "EnvironmentVarHandler.hpp"
#include "ConfigManager.hpp"
#include <functional>
#include "QueueMgr.hpp"
#include <error.h>

#define SUBSCRIBER_ID "MQTT_EXPORT_SUBSCRIBER"

extern std::atomic<bool> g_shouldStop;

/**
 * Constructor Initializes MQTT publisher
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param strClientID :[in] client ID with which to subscribe (this is topic name)
 * @param iQOS :[in] QOS value with which publisher will publish messages
 * @return None
 */
CMQTTHandler::CMQTTHandler(const std::string &strPlBusUrl, int iQOS):
	CMQTTBaseHandler(strPlBusUrl, SUBSCRIBER_ID, iQOS, (false == CcommonEnvManager::Instance().getDevMode()),
	"/run/secrets/rootca/cacert.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem", 
	"/run/secrets/mymqttcerts/mymqttcerts_client_key.pem", "MQTTSubListener")
{
	DO_LOG_DEBUG("MQTT handler initialized successfully");
}

/**
 * Maintain single instance of this class
 * @param None
 * @return Reference of this instance of this class, if successful;
 * 			Application exits in case of failure
 */
CMQTTHandler& CMQTTHandler::instance()
{
	static bool bIsFirst = true;
	static std::string strPlBusUrl = EnvironmentInfo::getInstance().getDataFromEnvMap("MQTT_URL_FOR_EXPORT");
	static int nQos = 1;

	if(bIsFirst)
	{
		if(strPlBusUrl.empty())
		{
			DO_LOG_ERROR("MQTT_URL_FOR_EXPORT Environment variable is not set");
			throw std::runtime_error("Missing required config..");
		}
	}

	DO_LOG_DEBUG("Internal MQTT subscriber is connecting with QOS : " + std::to_string(nQos));
	static CMQTTHandler handler(strPlBusUrl.c_str(), nQos);

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
void CMQTTHandler::subscribeTopics()
{
	try
	{
		auto subTopic = [&](const std::string &a_sEnv) 
		{
			const char* pcEnvVal = std::getenv(a_sEnv.c_str());
			if(pcEnvVal == NULL)
			{
				DO_LOG_ERROR(a_sEnv + " Environment Variable is not set" + a_sEnv);
			}
			else
			{
				m_MQTTClient.subscribe(pcEnvVal);
			}
		};

		subTopic("mqtt_SubReadTopic");
		subTopic("mqtt_SubWriteTopic");

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
void CMQTTHandler::connected(const std::string &a_sCause)
{
	try
	{
		signalIntMQTTConnDoneThread();
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
void CMQTTHandler::msgRcvd(mqtt::const_message_ptr a_pMsg)
{
	try
	{
		CMQTTHandler::instance().pushMsgInQ(a_pMsg);
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
bool CMQTTHandler::init()
{
	// Initiate semaphore for requests
	int retVal = sem_init(&m_semConnSuccess, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create semaphore for success connection " + std::to_string(errno) + " " + std::strerror(errno));
		return false;
	}

	std::thread{ std::bind(&CMQTTHandler::handleConnSuccessThread,
			std::ref(*this)) }.detach();

	return true;
}

/**
 * Thread function to handle SCADA connection success scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CMQTTHandler::handleConnSuccessThread()
{
	while(false == g_shouldStop.load())
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
				if(true == g_shouldStop.load())
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
* parse message to retrieve QOS and topic names
* @param json :[in] message from which to retrieve real-time
* @param isRealtime :[in] is it a message for real-time operation
* @param bIsDefault :[in] default RT value
* @return true/false based on success/failure
*/
bool CMQTTHandler::parseMQTTMsg(const std::string &sJson, bool &isRealtime, const bool bIsDefault)
{
	bool bRetVal = false;

	try
	{
		isRealtime = bIsDefault;
		cJSON *root = cJSON_Parse(sJson.c_str());
		if (NULL == root)
		{
			DO_LOG_ERROR("Message received from MQTT could not be parsed in json format");
			return bRetVal;
		}

		if(! cJSON_HasObjectItem(root, "realtime"))
		{
			DO_LOG_DEBUG("Message received from MQTT does not have key \"realtime\", request will be sent as non-RT");
			if (NULL != root)
				cJSON_Delete(root);

			return true;
		}

		char *crealtime = cJSON_GetObjectItem(root, "realtime")->valuestring;
		if (NULL == crealtime)
		{
			DO_LOG_ERROR("Key 'realtime' could not be found in message received from ZMQ");
			if (NULL != root)
			{
				cJSON_Delete(root);
			}
			return true;
		}
		else
		{
			if(strcmp(crealtime, "0") == 0)
			{
				isRealtime = false;
			}
			else if(strcmp(crealtime, "1") == 0)
			{
				isRealtime = true;
			}
		}

		if (NULL != root)
			cJSON_Delete(root);

		bRetVal = true;
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		bRetVal = false;
	}

	return bRetVal;
}

/**
 * Push message in message queue to send on EII
 * @param msg :[in] reference of message to push in queue
 * @return true/false based on success/failure
 */
bool CMQTTHandler::pushMsgInQ(mqtt::const_message_ptr& a_msgMQTT)
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
		std::string payload{a_msgMQTT->get_payload()};
		CMessageObject oTemp{a_msgMQTT};
		bool bDefRT = false;
		bool isWrite = false;
		// Decide whether to use polling queue or writeResponse queue
		if(true == endsWith(sTopic, "/read"))
		{
			isWrite = false;
			bDefRT = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getDefaultRTConfig();
			// For now put msg in polling queue
		}
		else if(true == endsWith(sTopic, "/write"))
		{
			isWrite = true;
			bDefRT = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getDefaultRTConfig();
		}
		else
		{
			DO_LOG_INFO(sTopic + " : does not match pattern. Ignored.");
			return false;
		}
		
		//this should be present in each incoming request
		if (payload == "") //will not be the case ever
		{
			DO_LOG_ERROR("No payload with request");
			return false;
		}
		
		//parse payload to check if realtime is true or false
		bool isRealTime = false;
		if( ! parseMQTTMsg(payload.c_str(), isRealTime, bDefRT))
		{
			DO_LOG_DEBUG("Could not parse MQTT msg");
			return false;
		}

		//if payload contains realtime flag, push message to real time queue
		if(isRealTime)
		{
			if(isWrite)
			{
				QMgr::getRTWrite().pushMsg(oTemp);
			}
			else
			{
				QMgr::getRTRead().pushMsg(oTemp);
			}
		}
		else //non-RT
		{
			if(isWrite)
			{
				QMgr::getWrite().pushMsg(oTemp);
			}
			else
			{
				QMgr::getRead().pushMsg(oTemp);
			}
		}

		DO_LOG_DEBUG("Pushed MQTT message in queue");
		bRet = true;
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		bRet = false;
	}
	return bRet;
}

/**
 * Clean up, destroy semaphores, disables callback, disconnect from MQTT broker
 * @param None
 * @return None
 */
void CMQTTHandler::cleanup()
{
}

/**
 *
 * Signals that initernal MQTT connection is established
 * @return none
 */
void CMQTTHandler::signalIntMQTTConnDoneThread()
{
	sem_post(&m_semConnSuccess);
}

/**
 * Destructor
 */
CMQTTHandler::~CMQTTHandler()
{
	sem_destroy(&m_semConnSuccess);
}

