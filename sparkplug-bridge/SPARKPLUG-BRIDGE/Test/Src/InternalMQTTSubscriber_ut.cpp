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


#include "../Inc/InternalMQTTSubscriber_ut.hpp"

#include <iostream>
using namespace std;


void InternalMQTTSubscriber_ut::SetUp()
{
	// Setup code
}

void InternalMQTTSubscriber_ut::TearDown()
{
	// TearDown code
}

/**
 * Test case to check if prepareCJSONMsg() behaves as expected
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(InternalMQTTSubscriber_ut, prepareCJSONMsg_EmptyVector)
{
	bool result = CIntMqttHandler::instance().prepareCJSONMsg(stRefActionVec);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check if prepareWriteMsg() behaves as expected
 * @param :[in] None
 * @param :[out] None
 * @return: bool
 */
TEST_F(InternalMQTTSubscriber_ut, writeRequest)
{
	std::string value = "2.0.0.2";
	std::string a_Name = "Properties/Version";
	uint32_t a_uiDataType = METRIC_DATA_TYPE_STRING;
	//CValObj(uint32_t a_uiDataType, var_t a_objVal)
	CValObj ocval(METRIC_DATA_TYPE_STRING, value);
	uint64_t timestamp = 1486144502122;
	//CMetric(std::string a_sName, const CValObj &a_objVal, const uint64_t a_timestamp)
	std::shared_ptr<CIfMetric> ptrCIfMetric = std::make_shared<CMetric>(a_Name, ocval, timestamp);
	metricMapIf_t mapChangedMetrics;
	mapChangedMetrics.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));
	std::string a_sSubDev = "RBOX510";
	std::string a_sSparkPluName = "flowmeter-PL0";
	bool a_bIsVendorApp = false;
	CSparkPlugDev oDev(a_sSubDev, a_sSparkPluName, a_bIsVendorApp);
	auto refDev = std::ref(oDev);
	bool result = CIntMqttHandler::instance().prepareWriteMsg(refDev, mapChangedMetrics);
	EXPECT_EQ(true, result);
}


/**
 * Test case to check if prepareCMDMsg() behaves as expected
 * @param :[in] None
 * @param :[out] None
 * @return: bool
 */
TEST_F(InternalMQTTSubscriber_ut, prepareCMDMsg)
{
	std::string a_Name = "Properties/Version";
	uint32_t a_uiDataType = METRIC_DATA_TYPE_STRING;
	std::shared_ptr<CIfMetric> ptrCIfMetric = std::make_shared<CMetric>(a_Name, a_uiDataType);
	metricMapIf_t mapChangedMetrics;
	mapChangedMetrics.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));

	std::string a_sSubDev = "flowmeter";
	std::string a_sSparkPluName = "spBv-Test";
	bool a_bIsVendorApp = true;

	CSparkPlugDev oDev(a_sSubDev, a_sSparkPluName, a_bIsVendorApp);
	auto refDev = std::ref(oDev);
	bool result = CIntMqttHandler::instance().prepareCMDMsg(refDev, m_mapChangedMetrics);
	EXPECT_EQ(true, result);
}

/*
 * Test Case to check if prepareCJSONMsg
 *
 */
TEST_F(InternalMQTTSubscriber_ut, prepareCJSONMsg)
{

	std::string value = "2.0.0.2";
	std::string a_Name = "Properties/Version";
	uint32_t a_uiDataType = METRIC_DATA_TYPE_STRING;
	CValObj ocval(a_uiDataType, value);
	uint64_t timestamp = 1486144502122;
	std::shared_ptr<CIfMetric> ptrCIfMetric = std::make_shared<CMetric>(a_Name, ocval, timestamp);
	metricMapIf_t mapChangedMetrics;
	mapChangedMetrics.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));
	std::string a_sSubDev = "flowmeter";
	std::string a_sSparkPluName = "spBv-Test";
	bool a_bIsVendorApp = true;
	CSparkPlugDev oDev(a_sSubDev, a_sSparkPluName, a_bIsVendorApp);
	auto refDev = std::ref(oDev);
	stRefForSparkPlugAction stRefVec(refDev, enMSG_BIRTH, mapChangedMetrics);
	std::vector<stRefForSparkPlugAction> v1;
	v1.push_back(stRefVec);
	bool result = CIntMqttHandler::instance().prepareCJSONMsg(v1);
	EXPECT_EQ(true, result);
}

/*
 * Test Case to check if publish_msg_to_emb is working for vendor app
 *
 */
TEST_F(InternalMQTTSubscriber_ut, publish_msg_to_emb_va)
{
	zmq_handler::prepareCommonContext("pub");
	std::string Msg = "[{\"name\":\"Properties/Version\",\"dataType\":\"String\",\"value\":\"100\",\"timestamp\":1649389685419}]";
	std::string Topic = "CMD/UWC_1/COM1";
	bool result = CIntMqttHandler::instance().publish_msg_to_emb(Msg, Topic);
	EXPECT_EQ(true, result);
}

/*
 * Test Case to check if publish_msg_to_emb is working for real device
 *
 */
TEST_F(InternalMQTTSubscriber_ut, publish_msg_to_emb_rd_default)
{
	zmq_handler::set_RT_NRT("default-0");
	std::string Msg = "{\"wellhead\":\"PL0\",\"command\":\"D2\",\"dataType\":\"Int16\",\"scaledValue\":20,\"timestamp\":\"2022-04-08 03:49:04\",\"usec\": \"1649389744700\",\"version\":\"2.0\",\"app_seq\":\"SCADA_RTU_1\"}";
	std::string Topic = "/flowmeter/PL0/D2/write";
	bool result = CIntMqttHandler::instance().publish_msg_to_emb(Msg, Topic);
	EXPECT_EQ(true, result);
}

/*
 * Test Case to check if publish_msg_to_emb is working for real device NRT
 *
 */
TEST_F(InternalMQTTSubscriber_ut, publish_msg_to_emb_rd_nrt)
{
	zmq_handler::set_RT_NRT("flowmeter-PL0-D2-0");
	std::string Msg = "{\"wellhead\":\"PL0\",\"command\":\"D2\",\"dataType\":\"Int16\",\"scaledValue\":20,\"timestamp\":\"2022-04-08 03:49:04\",\"usec\": \"1649389744700\",\"version\":\"2.0\",\"app_seq\":\"SCADA_RTU_1\"}";
	std::string Topic = "/flowmeter/PL0/D2/write";
	bool result = CIntMqttHandler::instance().publish_msg_to_emb(Msg, Topic);
	EXPECT_EQ(true, result);
}
/*
 * Test Case to check if publish_msg_to_emb is working for real device RT
 *
 */
TEST_F(InternalMQTTSubscriber_ut, publish_msg_to_emb_rd_rt)
{
	zmq_handler::set_RT_NRT("flowmeter-PL0-D2-1");
	std::string Msg = "{\"wellhead\":\"PL0\",\"command\":\"D2\",\"dataType\":\"Int16\",\"scaledValue\":20,\"timestamp\":\"2022-04-08 03:49:04\",\"usec\": \"1649389744700\",\"version\":\"2.0\",\"app_seq\":\"SCADA_RTU_1\"}";
	std::string Topic = "/flowmeter/PL0/D2/write";
	bool result = CIntMqttHandler::instance().publish_msg_to_emb(Msg, Topic);
	EXPECT_EQ(true, result);
}

TEST_F(InternalMQTTSubscriber_ut, subscribeTopics)
{
	_subscribeTopics();
}












