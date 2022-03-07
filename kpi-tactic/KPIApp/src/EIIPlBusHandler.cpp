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

#include "EIIPlBusHandler.hpp"
#include "KPIAppConfigMgr.hpp"
#include "CommonDataShare.hpp"
#include "EnvironmentVarHandler.hpp"
#include "Logger.hpp"
#include "Common.hpp"
#include "ZmqHandler.hpp"

extern std::atomic<bool> g_stopThread;

// patterns to be used to find on-demand topic strings
#define POLLING			 		"update"
#define WRITE_RESPONSE 			"writeResponse"


/**
 * Create EII msg bus context and topic context for publisher and subscriber both
 * @param None
 * @return true/false based on success/failure
 */
bool CEIIPlBusHandler::initEIIContext()
{
	bool retVal = true;
	// Initializing all the pub/sub topic base context for ZMQ
	int num_of_publishers = zmq_handler::getNumPubOrSub("pub");
	try
	{
		if (num_of_publishers >= 1)
		{
			if (true != zmq_handler::prepareCommonContext("pub"))
			{
				DO_LOG_ERROR("Context creation failed for pub topic ");
				std::cout << __func__ << ":" << __LINE__ << " Error : Context creation failed for pub topic" <<  std::endl;
				retVal = false;
			}
		}
		else
		{
			DO_LOG_ERROR("could not find any Publishers in publisher Configuration");
			std::cout << __func__ << ":" << __LINE__ << " Error : could not find any Publishers in publisher Configuration" <<  std::endl;
			retVal = false;
		}

		int num_of_subscribers = zmq_handler::getNumPubOrSub("sub");
		if(num_of_subscribers >= 1)
		{
			if (true != zmq_handler::prepareCommonContext("sub"))
			{
				DO_LOG_ERROR("Context creation failed for sub topic");
				std::cout << __func__ << ":" << __LINE__ << " Error : Context creation failed for sub topic" <<  std::endl;
				retVal = false;
			}
		}
		else
		{
			DO_LOG_ERROR("could not find any subscribers in subscriber Configuration");
			std::cout << __func__ << ":" << __LINE__ << " Error : could not find any subscribers in subscriber Configuration" <<  std::endl;
			retVal = false;
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR((std::string)ex.what());
	}

	return retVal;
}

/**
 * Process message received from EII for control loop operations
 * @param msg	:[in] actual message
 * @param a_bIsPollMsg :[in] true = polling msg, false = write response
 * returns true/false based on success/failure
 */
bool CEIIPlBusHandler::processMsg(msg_envelope_t *msg, CQueueHandler &a_rQ,
		const std::function<bool(const std::string &)> &a_fPointListChecker)
{
	bool bRetVal = false;
	try
	{
		if(msg == NULL)
		{
			DO_LOG_ERROR("Received NULL msg in msgbus_recv_wait");
			return false;
		}

		msg_envelope_elem_body_t* data;
		msgbus_ret_t msgRet = msgbus_msg_envelope_get(msg, "data_topic", &data);
		if(msgRet != MSG_SUCCESS)
		{
			DO_LOG_ERROR("topic key not present in zmq message");
			return false;
		}
		std::string sRcvdTopic{data->body.string};
		// Commented following code for optimization
		/*if(false == a_fPointListChecker(sRcvdTopic))
		{
			DO_LOG_DEBUG(sRcvdTopic + ": This is not a control loop topic. Ignored.")
			return false;
		}*/

		msg_envelope_serialized_part_t *parts = NULL;
		int num_parts = msgbus_msg_envelope_serialize(msg, &parts);
		if (num_parts <= 0)
		{
			DO_LOG_ERROR("Failed to serialize message");
		}
		else if(NULL != parts)
		{
			if(NULL != parts[0].bytes)
			{
				std::string sMsgBody(parts[0].bytes);
				CMessageObject oMsg{sRcvdTopic, sMsgBody};
				a_rQ.pushMsg(oMsg);
				bRetVal = true;
			}
		}
		else
		{
			DO_LOG_ERROR("NULL pointer received");
			bRetVal = false;
		}
		if(NULL != parts)
		{
			msgbus_msg_envelope_serialize_destroy(parts, num_parts);
			parts = NULL;
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR((std::string)ex.what());
	}

	return bRetVal;
}

/**
 * Thread function to listen on EII and send data to MQTT
 * @param sTopic :[in] topic to listen onto
 * @param context :[in] msg bus context
 * @param subContext :[in] sub context
 * @param a_bIsPolling :[in] operation type this thread needs to perform
 * @return None
 */
void CEIIPlBusHandler::listenOnEII(std::string sTopic, zmq_handler::stZmqContext context,
			zmq_handler::stZmqSubContext subContext, bool a_bIsPolling)
{
	if(context.m_pContext == NULL || subContext.sub_ctx == NULL)
	{
		std::cout << "Context is null. Cannot start listening on EII for topic : " << sTopic << std::endl;
		DO_LOG_ERROR("Context is null. Cannot start listening on EII for topic : " + sTopic);
		return;
	}

	std::function<bool(const std::string &)> fPointListChecker = 
		std::bind(&CControlLoopMapper::isControlLoopPollPoint,
			&CKPIAppConfig::getInstance().getControlLoopMapper(), std::placeholders::_1);
	std::reference_wrapper<CQueueHandler> refQ{QMgr::PollMsgQ()};
	if(false == a_bIsPolling)
	{
		refQ = QMgr::WriteRespMsgQ();
		fPointListChecker = 
			std::bind(&CControlLoopMapper::isControlLoopWrRspPoint, 
				&CKPIAppConfig::getInstance().getControlLoopMapper(), std::placeholders::_1);
	}

	void *msgbus_ctx = context.m_pContext;
	recv_ctx_t *sub_ctx = subContext.sub_ctx;

	DO_LOG_INFO("ZMQ listening for topic : " + sTopic);

	while ((false == g_stopThread.load()) && (msgbus_ctx != NULL) && (sub_ctx != NULL))
	{
		try
		{
			msg_envelope_t *msg = NULL;
			msgbus_ret_t ret = msgbus_recv_wait(msgbus_ctx, sub_ctx, &msg);
			if (ret != MSG_SUCCESS)
			{
				// Interrupt is an acceptable error
				if (ret == MSG_ERR_EINTR)
				{
					DO_LOG_ERROR("received MSG_ERR_EINT");
					//break;
				}
				DO_LOG_ERROR("Failed to receive message errno: " + std::to_string(ret));
				continue;
			}
			
			if(NULL != msg)
			{
				// process ZMQ message 
				processMsg(msg, refQ, fPointListChecker);
				
				msgbus_msg_envelope_destroy(msg);
				msg = NULL;
			}
		}
		catch (std::exception &ex)
		{
			DO_LOG_ERROR((std::string)ex.what() + " for topic : " + sTopic);
		}
	}//while ends

	DO_LOG_DEBUG("exited !!");
}

/**
 * publish message to EMB
 * @param a_sEMBMsg :[in] message to publish on EMB
 * @param a_sEMBTopic :[in] emb topic
 * @return true/false based on success/failure
 */
bool CEIIPlBusHandler::publishEMBMsg(std::string a_sEMBMsg, std::string &a_sEMBTopic)
{
	// Creating message to be published
	msg_envelope_t *msg = NULL;
	cJSON *root = NULL;

	try
	{
		msg = msgbus_msg_envelope_new(CT_JSON);
		if(msg == NULL)
		{
			return false;
		}
		//Get the context for this EMB PUB topic
		zmq_handler::prepareContext(true, (zmq_handler::getPubCtxCfg()).m_pub_msgbus_ctx, a_sEMBTopic, (zmq_handler::getPubCtxCfg()).m_pub_config);		
		//parse from root element
		root = cJSON_Parse(a_sEMBMsg.c_str());
		if (NULL == root)
		{
			if(msg != NULL)
			{
				msgbus_msg_envelope_destroy(msg);
			}
			return false;
		}

		cJSON *device = root->child;
		while (device)
		{
			if(cJSON_IsString(device))
			{
				msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_string(
						device->valuestring);
				if(value != NULL)
				{
					msgbus_msg_envelope_put(msg, device->string, value);
				}
			}
			else
			{
				throw std::string("Invalid JSON");
			}
			device = device->next;
		}

		if (root)
		{
			cJSON_Delete(root);
			root = NULL;
		}

		//add time stamp before publishing msg on EII
		std::string strTsReceived{""};
		bool bRet = true;
		if(true == zmq_handler::publishJson(strTsReceived, msg, a_sEMBTopic, "tsMsgPublishOnEII"))
		{
			bRet = true;
		}
		else
		{
			DO_LOG_ERROR("Failed to publish write msg on EII: " + a_sEMBMsg);
			bRet = false;
		}

		if(msg != NULL)
			msgbus_msg_envelope_destroy(msg);
		msg = NULL;

		return bRet;
	}
	catch(std::string& strException)
	{
		DO_LOG_ERROR("publishEMBMsg error1::" + strException);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR("publishEMBMsg error2::" + std::string(ex.what()));
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
 * Configures EII listeners based on configurations
 * @param a_bIsPollingRT :[in] tells whether RT or Non-RT polling topics to be scanned
 * @param a_bIsWrOpRT :[in] tells whether RT or Non-RT write response topics to be scanned
 * @return None
 */
void CEIIPlBusHandler::configEIIListerners(bool a_bIsPollingRT, bool a_bIsWrOpRT)
{
	try
	{
		// get sub topic list
		std::vector<std::string> vFullTopics;
		bool tempRet = zmq_handler::returnAllTopics("sub", vFullTopics);
		if(tempRet == false) {
			throw "returnAllTopics failed";
		}

		for (auto &sTopic : vFullTopics)
		{
			if(sTopic.empty())
			{
				continue;
			}
			bool bIsPolling = true;
			// Pattern matching
			if(std::string::npos != sTopic.find(POLLING))
			{
				// This topic is for polling
				bIsPolling = true;
			}
			else if(std::string::npos != sTopic.find(WRITE_RESPONSE))
			{
				// This topic is for write response processing 
				bIsPolling = false;
			}
			else
			{
				// Not matching with pattern. 
				DO_LOG_INFO(sTopic + ": Topic will not be monitored on ZMQ");
				continue;
			}
			DO_LOG_INFO(sTopic + ": Topic will be monitored on ZMQ");

			zmq_handler::stZmqContext& context = zmq_handler::getCTX(sTopic);
			zmq_handler::stZmqSubContext& subContext = zmq_handler::getSubCTX(sTopic);
			m_vThreads.push_back(
				std::thread(&CEIIPlBusHandler::listenOnEII, this, sTopic, context, subContext, bIsPolling));
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Stops EII listener threads
 * @return None
 */
void CEIIPlBusHandler::stopEIIListeners()
{
	try
	{
		for (auto &itr : m_vThreads)
		{
			if (itr.joinable())
			{
				itr.join();
			}
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
	}
}
