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
#include "QueueMgr.hpp"
#include "MqttHandler.hpp"
#include "EIIPlBusHandler.hpp"
#include "KPIAppConfigMgr.hpp"
#include "ControlLoopHandler.hpp"
#include "ZmqHandler.hpp"

static CEIIPlBusHandler& getEIIPlBusHandler()
{
	static CEIIPlBusHandler _obj;
	return _obj;
}

/**
 * Get reference of polled message
 * @param None
 * @return reference of on-demand read operation instance
 */
CQueueHandler& QMgr::PollMsgQ()
{
	static CQueueHandler ng_qPollMsgs;
	return ng_qPollMsgs;
}

/**
 * Get reference of write response message
 * @param None
 * @return reference of on-demand read operation instance
 */
CQueueHandler& QMgr::WriteRespMsgQ()
{
	static CQueueHandler ng_qWrRespMsgs;
	return ng_qWrRespMsgs;
}

/**
 * Based on whether MQTT mode or ZMQ mode, function prepares MQTT for publishing & subscribing,
 * or prepares contexts and create threads respectively
 * @param None
 * @return
 */
void PlBusMgr::initPlatformBusHandler(bool a_bIsMQTTMode)
{
	try
	{
		if(a_bIsMQTTMode)
		{
			// Prepare MQTT for publishing & subscribing
			// subscribing to topics happens in callback of connect()
			CMqttHandler::instance();
		}
		else
		{
			// It is ZMQ mode. Prepare contexts and create threads
			getEIIPlBusHandler().initEIIContext();

			getEIIPlBusHandler().configEIIListerners(CKPIAppConfig::getInstance().isRTModeForPolledPoints(),
				CKPIAppConfig::getInstance().isRTModeForWriteOp());
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Forms and publishes write request on MQTT
 * @param a_sPubTopic	[in]: Topic for publishing 
 * @param a_sMsg		[in]: Message to be published
 * @return true/false based on success/failure
 */
bool publishWriteReqOnMQTT(const std::string &a_sPubTopic, const std::string &a_sMsg)
{
	try
	{
		return CMqttHandler::instance().publishMsg(a_sMsg, a_sPubTopic);
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(a_sPubTopic + ": Write request error: " +  e.what());
	}
	
	return false;
}
/**
* map MQTT topic to EMB topic
* @param mqttTopic :[in] Mqtt topic
* @param isRealTime:[in] RT/NRT value
* @return embtopic
*/

std::string mapMqttToEMBRespTopic(std::string mqttTopic,std::string isRealTime)
{
	std::string delimeter = "/write";
	int size = mqttTopic.find(delimeter);
	mqttTopic = mqttTopic.substr(0,size);
	std::string embtopic;
	isRealTime = (isRealTime == "1")?"RT":"NRT";
	embtopic = isRealTime+delimeter+mqttTopic;
	return embtopic;
}
/**
 * Forms and publishes write request
 * @param a_rCtrlLoop	[in]: Control loop for which write needs to be published
 * @param a_sWrSeq		[in]: Sequence number to be used in write request
 * @return true/false based on success/failure
 */
bool PlBusMgr::publishWriteReq(const CControlLoopOp& a_rCtrlLoop, 
			const std::string &a_sWrSeq)
{
	try
	{
		std::string sWrRT{"0"};
		if(true == CKPIAppConfig::getInstance().isRTModeForWriteOp())
		{
			sWrRT.assign("1");
		}
		std::string sPubTopic{a_rCtrlLoop.getWritePoint()+"/write"};
		std::string sMsg{""};
		
		if(false == commonUtilKPI::createWriteRequest(sMsg, a_sWrSeq, 
			a_rCtrlLoop.getWellHeadNameForWrReq(), 
			a_rCtrlLoop.getPointNameForWrReq(), a_rCtrlLoop.getValue(), sWrRT,
			sPubTopic, CKPIAppConfig::getInstance().isMQTTModeOn()))
		{
			DO_LOG_ERROR(a_sWrSeq + ": Unable to create a write request. Ignoring the message");
			return false;
		}

		if(true == CKPIAppConfig::getInstance().isMQTTModeOn())
		{
			return publishWriteReqOnMQTT(sPubTopic, sMsg);
		}
		else
		{
			sPubTopic = mapMqttToEMBRespTopic(sPubTopic,sWrRT);
			return getEIIPlBusHandler().publishEIIMsg(sMsg,sPubTopic);
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(a_rCtrlLoop.getWritePoint() + ": Write request error: " +  e.what());
	}
	
	return false;
}

/**
 * Stops listener threads
 * @return None
 */
void PlBusMgr::stopListeners()
{
	try
	{
		if(true == CKPIAppConfig::getInstance().isMQTTModeOn())
		{
			;
		}
		else
		{
			getEIIPlBusHandler().stopEIIListeners();
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
	}
}
