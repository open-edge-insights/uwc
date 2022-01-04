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

#include "ZmqHandler.hpp"
#include "Common.hpp"
#include "QueueMgr.hpp"
#include "MQTTSubscribeHandler.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "MQTTPublishHandler.hpp"
#include "ConfigManager.hpp"
#include "EnvironmentVarHandler.hpp"
#include "ZmqHandler.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

using namespace zmq_handler;

std::vector<std::thread> g_vThreads;

std::atomic<bool> g_shouldStop(false);

#define APP_VERSION "0.0.6.6"

// patterns to be used to find on-demand topic strings
// topic syntax -
// for non-RT topic for polling - <topic_name>__PolledData
// for RT topic read RT - <topic_name>__RdReq_RT
#define POLLING			 		"_PolledData"
#define POLLING_RT 				"_PolledData_RT"
#define READ_RESPONSE 			"_RdResp"
#define READ_RESPONSE_RT		"_RdResp_RT"
#define WRITE_RESPONSE 			"_WrResp"
#define WRITE_RESPONSE_RT		"_WrResp_RT"

/**
 * Process message received from EII and send for publishing on MQTT
 * @param msg	:[in] actual message
 * @param mqttPublisher :[in] mqtt publisher instance from which to publish this message
 * returns true/false based on success/failure
 */
bool processMsg(msg_envelope_t *msg, CMQTTPublishHandler &mqttPublisher)
{
	int num_parts = 0;
	msg_envelope_serialized_part_t *parts = NULL;
	bool bRetVal = false;

	if(msg == NULL)
	{
		DO_LOG_ERROR("Received NULL msg in msgbus_recv_wait");
		return bRetVal;
	}

	struct timespec tsMsgRcvd;
	timespec_get(&tsMsgRcvd, TIME_UTC);

	std::string revdTopic;
	msg_envelope_elem_body_t* data;
	msgbus_ret_t msgRet = msgbus_msg_envelope_get(msg, "data_topic", &data);
	if(msgRet != MSG_SUCCESS)
	{
		DO_LOG_ERROR("topic key not present in zmq message");
		bRetVal = false;
	}
	else
	{
		revdTopic = data->body.string; // has the topic /flowmeter/PL0/D18/update

		std::string strTsRcvd = std::to_string(CCommon::getInstance().get_micros(tsMsgRcvd));
		msg_envelope_elem_body_t* tsMsgRcvdPut = msgbus_msg_envelope_new_string(strTsRcvd.c_str());
		msgbus_msg_envelope_put(msg, "tsMsgRcvdForProcessing", tsMsgRcvdPut);

		num_parts = msgbus_msg_envelope_serialize(msg, &parts);
		if (num_parts <= 0)
		{
			DO_LOG_ERROR("Failed to serialize message");
		}
		else if(NULL != parts)
		{
			if(NULL != parts[0].bytes)
			{
				std::string mqttMsg(parts[0].bytes);
				mqttPublisher.createNPubMsg(mqttMsg, revdTopic);

				bRetVal = true;
			}
		}
		else
		{
			DO_LOG_ERROR("NULL pointer received");
			bRetVal = false;
		}
	}

	if(parts != NULL)
	{
		msgbus_msg_envelope_serialize_destroy(parts, num_parts);
	}
	if(msg != NULL)
	{
		msgbus_msg_envelope_destroy(msg);
		msg = NULL;
	}
	parts = NULL;

	return bRetVal;
}

/**
 * Get operation info from global config depending on the topic name
 * @param topic	:[in] topic for which to retrieve operation info
 * @param operation	:[out] operation info
 * @return none
 */
void getOperation(std::string topic, globalConfig::COperation& operation)
{
	if(std::string::npos != topic.find(POLLING_RT,
			topic.length() - std::string(POLLING_RT).length(),
			std::string(POLLING_RT).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpPollingOpConfig().getRTConfig();
	}
	else if(std::string::npos != topic.find(POLLING,
			topic.length() - std::string(POLLING).length(),
			std::string(POLLING).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpPollingOpConfig().getNonRTConfig();
	}
	else if(std::string::npos != topic.find(READ_RESPONSE_RT,
			topic.length() - std::string(READ_RESPONSE_RT).length(),
			std::string(READ_RESPONSE_RT).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig().getRTConfig();
	}
	else if(std::string::npos != topic.find(WRITE_RESPONSE_RT,
			topic.length() - std::string(WRITE_RESPONSE_RT).length(),
			std::string(WRITE_RESPONSE_RT).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig().getRTConfig();
	}
	else if(std::string::npos != topic.find(READ_RESPONSE,
			topic.length() - std::string(READ_RESPONSE).length(),
			std::string(READ_RESPONSE).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig().getNonRTConfig();
	}
	else if(std::string::npos != topic.find(WRITE_RESPONSE,
			topic.length() - std::string(WRITE_RESPONSE).length(),
			std::string(WRITE_RESPONSE).length()))
	{
		operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig().getNonRTConfig();
	}
}

/**
 * Thread function to listen on EII and send data to MQTT
 * @param topic :[in] topic prefix to listen onto
 * @param context :[in] msg bus context
 * @param subContext :[in] sub context
 * @param operation :[in] operation type this thread needs to perform
 * @return None
 */
void listenOnEII(std::string topicPrefix, zmq_handler::stZmqContext context, zmq_handler::stZmqSubContext subContext, globalConfig::COperation operation)
{
	globalConfig::set_thread_sched_param(operation);
	globalConfig::display_thread_sched_attr(topicPrefix + " listenOnEII");
	int qos = operation.getQos();

	if(context.m_pContext == NULL || subContext.sub_ctx == NULL)
	{
		DO_LOG_ERROR("Cannot start listening on EII for topicPrefix : " + topicPrefix);
		return;
	}

	void *msgbus_ctx = context.m_pContext; // this is per subscriber
	recv_ctx_t *sub_ctx = subContext.sub_ctx; // this is per SUB topic

	//consider topic name as distinguishing factor for publisher
	CMQTTPublishHandler mqttPublisher(EnvironmentInfo::getInstance().getDataFromEnvMap("MQTT_URL_FOR_EXPORT").c_str(),
					topicPrefix, qos);
	mqttPublisher.connect();
	
	DO_LOG_INFO("ZMQ listening for topic : " + topicPrefix);

	while ((false == g_shouldStop.load()) && (msgbus_ctx != NULL) && (sub_ctx != NULL))
	{
		try
		{
			msg_envelope_t *msg = NULL;
			msgbus_ret_t ret;

			ret = msgbus_recv_wait(msgbus_ctx, sub_ctx, &msg);
			if (ret != MSG_SUCCESS)
			{
				// Interrupt is an acceptable error
				if (ret == MSG_ERR_EINTR)
				{
					DO_LOG_ERROR("received MSG_ERR_EINT");
				}
				DO_LOG_ERROR("Failed to receive message errno: " + std::to_string(ret));
				continue;
			}
			
			// process ZMQ message and publish to MQTT
			processMsg(msg, mqttPublisher);

		}
		catch (std::exception &ex)
		{
			DO_LOG_FATAL((std::string)ex.what()+" for topic : "+topic);
		}
	}//while ends

	DO_LOG_DEBUG("exited !!");
}

/**
 * publish message to EII
 * @param a_oRcvdMsg  :[in] message to publish on EII
 * @param embTopic :[in] EII topic
 * @return true/false based on success/failure
 */
bool publishEIIMsg(CMessageObject &a_oRcvdMsg, const std::string &embTopic)
{
	bool retVal = false;

	// Creating message to be published
	msg_envelope_t *msg = NULL;
	cJSON *root = NULL;

	try
	{
		msg = msgbus_msg_envelope_new(CT_JSON);
		if(msg == NULL)
		{
			DO_LOG_ERROR("could not create new msg envelope");
			return retVal;
		}
		std::string eiiMsg = a_oRcvdMsg.getStrMsg();
		//TODO: Remove the realtime flag in JSON body before publishing to EMB.
		//parse from root element
		root = cJSON_Parse(eiiMsg.c_str());
		if (NULL == root)
		{
			DO_LOG_ERROR("Could not parse value received from MQTT");

			if(msg != NULL)
			{
				msgbus_msg_envelope_destroy(msg);
			}

			return retVal;
		}

		auto addField = [&msg](const std::string &a_sFieldName, const std::string &a_sValue) {
			DO_LOG_DEBUG(a_sFieldName + " : " + a_sValue);
			msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_string(a_sValue.c_str());
			if((NULL != value) && (NULL != msg))
			{
				msgbus_msg_envelope_put(msg, a_sFieldName.c_str(), value);
			}
		};

		cJSON *device = root->child;
		while (device)
		{
			if(cJSON_IsString(device))
			{        
				addField(device->string, device->valuestring);
			}
			else if (cJSON_IsBool(device))
			{
				bool val = false;
				// Add bool in msg envelope
				if (cJSON_IsTrue(device))
				{
					val = true;
				}
				msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_bool(val);
				if (NULL != msg && NULL != value)
				{
					msgbus_msg_envelope_put(msg, device->string, value);					
				}
			}
			else if (cJSON_IsNumber(device))
			{
				// Add number in msg envelope
				msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_floating(device->valuedouble);
				if (NULL != msg && NULL != value)
				{
					msgbus_msg_envelope_put(msg, device->string, value);
				}
			}
			else
			{
				throw std::string("Invalid JSON");
			}
			// get and print key
			device = device->next;
		}

		if (root)
		{
			cJSON_Delete(root);
		}
		root = NULL;

		addField("tsMsgRcvdFromMQTT", (std::to_string(CCommon::getInstance().get_micros(a_oRcvdMsg.getTimestamp()))).c_str());
		addField("sourcetopic", a_oRcvdMsg.getTopic());
		
		std::string strTsReceived{""};
		bool bRet = true;
		if(true == zmq_handler::publishJson(strTsReceived, msg, embTopic, "tsMsgPublishOnEII"))
		{
			bRet = true;
		}
		else
		{
			DO_LOG_ERROR("Failed to publish write msg on EII: " + eiiMsg);
			bRet = false;
		}

		msgbus_msg_envelope_destroy(msg);
		msg = NULL;

		return bRet;
	}
	catch(std::string& strException)
	{
		DO_LOG_ERROR(strException);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}

	if(msg != NULL)
	{
		msgbus_msg_envelope_destroy(msg);
	}
	if (root)
	{
		cJSON_Delete(root);
	}

	return false;
}

/**
 * Map MQTT topic to new EMB topic format
 * @param mqttTopic :[in] MQTT topic format received from MQTT client (example: /flowmeter/PL0/D13/read)
 * @param isRealTime :[in] flag to indicate this thread function is running in RT or NRT thread
 * @return : New Topic format mapped topic (example: RT/read/flowmeter/PL0/D13)
 */
std::string mapMqttToEMBTopic(std::string mqttTopic, bool isRealTime) {

	// delimeter
	char delim = '/';

	// To store the index of last
	// character found
	size_t index;

	// Function to find the last delimeter "/"
	index = mqttTopic.find_last_of(delim);

	// If topic doesn't have
	// character delim present in it
	if (index == string::npos) {
		throw "Delimeter " + delim + " is not present in the topic" + mqttTopic + "received from MQTT client";
	}
	
	std::string dataPointAbsPath = mqttTopic.substr(0,index); // ABsolute path of the data point : example: /flowmeter/PL0/D13 
	index +=1; // update index to teh char immediately after last "/"
	std::string operation = str.substr(index);
	std::string rtOrNrt = (isRealTime == true)?"RT":"NRT";
	std::string embTopic = rtOrNrt + "/" + operation + "/" + dataPointAbsPath; // RT/read//flowmeter/PL0/D13 or NRT/read/flowmeter/PL0/D13
	return embTopic;
}

/**
 * Process message received from MQTT and send it on EII
 * @param recvdMsg :[in] message received from MQTT client to publish on EII
 * @param isRealtime :[in] RT or Non RT.
 * @return true/false based on success/failure
 */
void processMsgToSendOnEII(CMessageObject &recvdMsg, const bool isRealtime)
{
	try
	{
		//received msg from queue
		std::string rcvdTopic = recvdMsg.getTopic();
		std::string strMsg = recvdMsg.getStrMsg();

		//this should be present in each incoming request
		if (rcvdTopic.empty()) //will not be the case ever
		{
			DO_LOG_ERROR("topic key not present in message: " + strMsg);
			return;
		}

		DO_LOG_DEBUG("Request received from MQTT for topic "+ rcvdTopic);

		// To add the mapping logic from MQTT topic format to NEW mapped EII topic format
		// /flowmeter/PL0/D13/read to RT|NRT/read/flowmeter/PL0/D13
		std::string embTopic = mapMqttToEMBTopic(rcvdTopic, isRealtime);

		//Get the context for this EMB PUB topic
		zmq_handler::prepareContext(true, zmq_handler::m_pub_msgbus_ctx, embTopic, zmq_handler::m_pub_config);

		if (embTopic.empty())
		{
			DO_LOG_ERROR("EMB topic is not set to publish on EMB"+ rcvdTopic);
			return;
		}
		else
		{
			//publish data to EII
			DO_LOG_DEBUG("MQTT topic is Mapped to new EMB topic format : " + embTopic);

			if(publishEIIMsg(recvdMsg, embTopic))
			{
				DO_LOG_DEBUG("Published EII message : "	+ strMsg + " on topic :" + embTopic);
			}
			else
			{
				DO_LOG_ERROR("Failed to publish EII message : "	+ strMsg + " on topic :" + embTopic);
			}
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}

	return;
}

/**
 * Set thread priority for threads that send messages from MQTT-Export to EII
 * depending on read and real-time parameters
 * @param isRealtime :[in] is operation real-time or not
 * @param isRead :[in] is it read or write operation
 * @return None
 */
void set_thread_priority_for_eii(bool& isRealtime, bool& isRead)
{
	globalConfig::COperation operation;

	try
	{
		if(isRealtime)
		{
			if(isRead)
			{
				operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig().getRTConfig();
			}
			else
			{
				operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig().getRTConfig();
			}
		}
		else
		{
			if(isRead)
			{
				operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig().getNonRTConfig();
			}
			else
			{
				operation = globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig().getNonRTConfig();
			}
		}
		globalConfig::set_thread_sched_param(operation);
		DO_LOG_DEBUG("Set thread priorities for isRead: " + std::to_string(isRead) + ", isRealtime : " + std::to_string(isRealtime));
	}
	catch(std::exception &e)
	{
		DO_LOG_FATAL(e.what());
	}
}

/**
 * Thread function to read requests from queue filled up by MQTT and send data to EII
 * @param qMgr 	:[in] pointer to respective queue manager
 * @return None
 */
void postMsgsToEII(QMgr::CQueueMgr& qMgr)
{
	DO_LOG_DEBUG("Starting thread to send messages on EII");

	bool isRealtime = qMgr.isRealTime(); // RT is extracted from JSON body sent from mqtt client. for backward compatibility will keep this. 
	bool isRead = qMgr.isRead();

	//set priority to send msgs on EII from MQTT-export (on-demand)
	set_thread_priority_for_eii(isRealtime, isRead);

	globalConfig::display_thread_sched_attr("postMsgsToEII");

	// std::string eiiTopic = "";
	// if(! isRead)//write request
	// {
	// 	if(isRealtime)
	// 	{
	// 		eiiTopic.assign(EnvironmentInfo::getInstance().getDataFromEnvMap("WriteRequest_RT"));
	// 	}
	// 	else
	// 	{
	// 		eiiTopic.assign(EnvironmentInfo::getInstance().getDataFromEnvMap("WriteRequest"));
	// 	}
	// }
	// else//read request
	// {
	// 	if(isRealtime)
	// 	{
	// 		eiiTopic.assign(EnvironmentInfo::getInstance().getDataFromEnvMap("ReadRequest_RT"));
	// 	}
	// 	else
	// 	{
	// 		eiiTopic.assign(EnvironmentInfo::getInstance().getDataFromEnvMap("ReadRequest"));
	// 	}
	// }

	try
	{
		while (false == g_shouldStop.load())
		{
			CMessageObject oTemp;
			if(true == qMgr.isMsgArrived(oTemp))
			{
				processMsgToSendOnEII(oTemp, isRealtime);
			}
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
	}
}

/**
 * Get EII topic list, get corresponding message bus and topic contexts.
 * Spawn threads to listen to EII messages, receive messages from EII and publish them to MQTT
 * @param None
 * @return None
 */
void postMsgstoMQTT()
{
	DO_LOG_DEBUG("Initializing threads to start listening on EII topics...");

	// get sub topic list
	std::vector<std::string> vFullTopics;
	bool tempRet = zmq_handler::returnAllTopics("sub", vFullTopics);
	if(tempRet == false) {
		return;
	} 

	for (auto &topic : vFullTopics)
	{
		if(topic.empty())
		{
			DO_LOG_ERROR("found empty MQTT subscriber topic");
			continue;
		}

		zmq_handler::stZmqContext& context = zmq_handler::getCTX(topic);
		//will give topic context
		zmq_handler::stZmqSubContext& subContext = zmq_handler::getSubCTX(topic);
		DO_LOG_DEBUG("Full topic - " + topic + " AND listening on: " + topic);

		//get operation depending on the topic
		globalConfig::COperation objOperation;
		getOperation(topic, objOperation);

		g_vThreads.push_back(
				std::thread(listenOnEII, topic, context, subContext, objOperation));
	}
}

/**
 * Create EII msg bus context and topic context for publisher and subscriber both
 * @param None
 * @return true/false based on success/failure
 */
bool initEIIContext()
{
	bool retVal = true;
	// Initializing all the pub/sub topic base context for ZMQ
	int num_of_publishers = zmq_handler::getNumPubOrSub("pub");
	if (num_of_publishers >= 1)
	{
		if (true != zmq_handler::prepareCommonContext("pub"))
		{
			DO_LOG_ERROR("Context creation failed for pub topic ");
			retVal = false;
		}
	}
	else
	{
		DO_LOG_ERROR("could not find any Publishers in publisher Configuration ");
		retVal = false;
	}

	int num_of_subscribers = zmq_handler::getNumPubOrSub("sub");
	if(num_of_subscribers >= 1)
	{
		if (true != zmq_handler::prepareCommonContext("sub"))
		{
			DO_LOG_ERROR("Context creation failed for sub topic");
			retVal = false;
		}
	}
	else
	{
		DO_LOG_ERROR("could not find any subscribers in subscriber Configuration");
		retVal = false;
	}
	return retVal;
}

/**
 * Main function of application
 * @param argc :[in] number of input parameters
 * @param argv :[in] input parameters
 * @return 	0/-1 based on success/failure
 */
int main(int argc, char *argv[])
{
	DO_LOG_DEBUG("Starting MQTT Export ...");

	try
	{
		CLogger::initLogger(std::getenv("Log4cppPropsFile"));
		DO_LOG_DEBUG("Starting MQTT Export ...");

		DO_LOG_INFO("MQTT-Expprt container app version is set to :: "+  std::string(APP_VERSION));

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

		globalConfig::CPriorityMgr::getInstance();

		//read environment values from settings
		CCommon::getInstance();

		//Prepare MQTT for publishing & subscribing
		//subscribing to topics happens in callback of connect()
		CMQTTHandler::instance();

		//Prepare ZMQ contexts for publishing & subscribing data
		if(!initEIIContext()) {
			DO_LOG_ERROR("Error in initEIIContext");
		}

#ifdef UNIT_TEST
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
#endif

		//Start listening on EII & publishing to MQTT
		postMsgstoMQTT();
		//threads to send on-demand requests on EII
		g_vThreads.push_back(std::thread(postMsgsToEII, std::ref(QMgr::getRTRead())));
		g_vThreads.push_back(std::thread(postMsgsToEII, std::ref(QMgr::getRTWrite())));
		g_vThreads.push_back(std::thread(postMsgsToEII, std::ref(QMgr::getRead())));
		g_vThreads.push_back(std::thread(postMsgsToEII, std::ref(QMgr::getWrite())));


		for (auto &th : g_vThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(" Exception : " + std::string(e.what()));
		return -1;
	}
	catch (...)
	{
		DO_LOG_FATAL("Exception : Unknown Exception Occurred. Exiting");
		return -1;
	}

	DO_LOG_WARN("Exiting MQTT Export Container");
	return 0;
}
