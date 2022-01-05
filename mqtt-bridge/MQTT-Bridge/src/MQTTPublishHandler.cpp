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

#include "MQTTPublishHandler.hpp"
#include "cjson/cJSON.h"
#include "Common.hpp"
#include "ConfigManager.hpp"

/**
 * Constructor Initializes MQTT publisher
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param strClientID :[in] client ID with which to subscribe (this is topic name)
 * @param iQOS :[in] QOS value with which publisher will publish messages
 * @return None
 */
CMQTTPublishHandler::CMQTTPublishHandler(std::string strPlBusUrl, std::string strClientID, int iQOS):
	CMQTTBaseHandler(strPlBusUrl, strClientID, iQOS, (false == CcommonEnvManager::Instance().getDevMode()),
        "/run/secrets/rootca/cacert.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_key.pem", "MQTTSubListener")
{
	try
	{
		connect();
		DO_LOG_DEBUG("MQTT initialized successfully. QOS to be used: " + std::to_string(m_QOS));
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL("Exception : " + std::string(e.what()));
	}
}

/**
 * Publish message on MQTT broker
 * @param a_sMsg :[in] message to publish
 * @param a_sTopic :[in] topic on which to publish message
 * @param a_tsMsgRcvd :[in] time stamp to add while publishing
 * @return true/false based on success/failure
 */
bool CMQTTPublishHandler::createNPubMsg(std::string &a_sMsg, std::string &a_sTopic)
{
	try
	{
		// Check if topic is blank
		if (true == a_sTopic.empty())
		{
			DO_LOG_ERROR("Empty topic. Message not processed");
			return false;
		}
		// Check if message is blank
		if (true == a_sMsg.empty())
		{
			DO_LOG_ERROR("Empty Message. No action for topic: " + a_sTopic);
			return false;
		}

		// Add timestamp to message
		struct timespec tsMsgPublish;
		timespec_get(&tsMsgPublish, TIME_UTC);
		std::string strTsPublish = std::to_string(CCommon::getInstance().get_micros(tsMsgPublish));

		// remove } bracket to add new key value pair to existing json
		a_sMsg.pop_back();

		std::string msgWithTimeStamp =  a_sMsg + ","+ "\"tsMsgReadyForPublish\"" + ":" + "\"" + strTsPublish + "\"" + "}";

		//publish data to MQTT
#ifdef INSTRUMENTATION_LOG
		DO_LOG_DEBUG("ZMQ Message: Time: "
				+ std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
		+ ", Msg: " + msgWithTimeStamp);
#endif

		publishMsg(msgWithTimeStamp, a_sTopic);
		return true;
	}
	catch (const std::exception &exc)
	{
		DO_LOG_ERROR(exc.what());
	}
	return false;
}

/**
 * Destructor
 */
CMQTTPublishHandler::~CMQTTPublishHandler()
{
}
