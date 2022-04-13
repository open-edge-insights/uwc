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

#ifndef TEST_INCLUDE_SCADAHANDLER_UT_H_
#define TEST_INCLUDE_SCADAHANDLER_UT_H_

#include "SCADAHandler.hpp"
#include "InternalMQTTSubscriber.hpp"
#include "Metric.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif


class SCADAHandler_ut : public::testing::Test
{
protected:
	virtual void SetUp();
	virtual void TearDown();
	bool _publishSparkplugMsg(org_eclipse_tahu_protobuf_Payload& a_payload, string spName, bool a_bIsNBirth)
	{
		return CSCADAHandler::instance().publishSparkplugMsg(a_payload, spName, a_bIsNBirth);
	}

	void _prepareNodeDeathMsg(bool a_bPublishMsg)
	{
		CSCADAHandler::instance().prepareNodeDeathMsg(a_bPublishMsg);
	}

	void _publish_node_birth()
	{
		CSCADAHandler::instance().publish_node_birth();
	}

	void _publishAllDevBirths(bool a_bIsNBIRTHProcess)
	{
		CSCADAHandler::instance().publishAllDevBirths(a_bIsNBIRTHProcess);
	}

	void _publish_device_birth(std::string a_deviceName, bool a_bIsNBIRTHProcess)
	{
		CSCADAHandler::instance().publish_device_birth(a_deviceName, a_bIsNBIRTHProcess);
	}

	bool _publishMsgDDEATH(const stRefForSparkPlugAction& a_stRefAction)
	{
		bool result = CSCADAHandler::instance().publishMsgDDEATH(a_stRefAction);
		return result;
	}

	bool _publishMsgDDEATH(const std::string a_sDevName)
	{
		bool result = CSCADAHandler::instance().publishMsgDDEATH(a_sDevName);
		return result;
	}

	bool _publishMsgDDATA(const stRefForSparkPlugAction& a_stRefAction)
	{
		bool result = CSCADAHandler::instance().publishMsgDDATA(a_stRefAction);
		return result;
	}

	void _subscribeTopics()
	{
		CSCADAHandler::instance().subscribeTopics();
	}

	void _connected(const std::string &a_sCause)
	{
		CSCADAHandler::instance().connected(a_sCause);
	}

	void _disconnected(const std::string &a_sCause)
	{
		CSCADAHandler::instance().disconnected(a_sCause);
	}

	void _publishNewUDTs()
	{
		CSCADAHandler::instance().publishNewUDTs();
	}

public:
	bool Bool_Res = false;
	org_eclipse_tahu_protobuf_Payload_Metric a_metric = { NULL, false, 0, true, get_current_timestamp(), true,
							METRIC_DATA_TYPE_UNKNOWN, false, 0, false, 0, false,
							true, false,
				org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
							false,
									org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
							0,
							{ 0 } };
	org_eclipse_tahu_protobuf_Payload_Template udt_template = org_eclipse_tahu_protobuf_Payload_Template_init_default;
bool success = true;
};

#endif /* TEST_INCLUDE_SCADAHANDLER_UT_H_ */
