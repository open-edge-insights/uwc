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

#include "../Inc/SCADAHandler_ut.hpp"

void SCADAHandler_ut::SetUp()
{
	// Setup code
}

void SCADAHandler_ut::TearDown()
{
	// TearDown code
}

/************************Helper functions************************/
/**
 * Helper function to call prepareSparkPlugMsg from thread
 * @param :[in] Vector: Reference action vector
 * @param :[out] boolean: Output result
 * @return None
 */
void TargetFunctionCaller( std::vector<stRefForSparkPlugAction> stRefActionVec, bool& bRes )
{
	bRes = CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);
}

/**
 * Helper function to call signalIntMQTTConnLostThread()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
void PostSem_semIntMQTTConnLost()
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	CSCADAHandler::instance().signalIntMQTTConnLostThread();
}



TEST_F(SCADAHandler_ut, AddModbusMetric_true)
{
	std::string SparkPlugname = "A";
	std::string SparkPlugValue = "";
	double dScale = 10;
	uint32_t a_uiPollInterval = 250;
	var_t a_objVal{true};
		CValObj CValObj_obj{METRIC_DATA_TYPE_BOOLEAN, a_objVal};
	//bool Result = CSCADAHandler::instance().addModbusMetric(a_metric, SparkPlugname, SparkPlugValue, true, a_uiPollInterval, true, dScale);
	bool Result = CSCADAHandler::instance().addModbusMetric(a_metric, SparkPlugname, CValObj_obj,
			                                                    true, a_uiPollInterval, true, dScale);
	EXPECT_EQ(true, Result);
}

TEST_F(SCADAHandler_ut, AddPropForBirth)
{
	std::string Protocol = {""};
	bool Result = CSCADAHandler::instance().addModbusPropForBirth(udt_template, Protocol);
    EXPECT_EQ(true, Result);
}

TEST_F(SCADAHandler_ut, IntMQTTConnectionEstablish)
{
	CSCADAHandler::instance().signalIntMQTTConnEstablishThread();
	EXPECT_EQ(true, success);
}


/**
 * Test case to check prepareSparkPlugMsg() with Data message
 * @param :[in] None
 * @param :[out] None
 * @return None
 */


TEST_F(SCADAHandler_ut, publishSparkplugMsg)
{
	org_eclipse_tahu_protobuf_Payload 	a_payload;
	memset(&a_payload, 0, sizeof(org_eclipse_tahu_protobuf_Payload));
	a_payload.has_timestamp = true;
	a_payload.timestamp = get_current_timestamp();
	a_payload.has_seq = true;
	a_payload.seq = 0;

	std::string value = "2.0.0.2";
	std::string a_Name = "Properties/Version";
	uint32_t a_uiDataType = METRIC_DATA_TYPE_STRING;
	CValObj ocval(a_uiDataType, value);
	uint64_t timestamp = 1486144502122;

	std::shared_ptr<CIfMetric> ptrCIfMetric = std::make_shared<CMetric>(a_Name, ocval, timestamp);
	metricMapIf_t mapChangedMetrics;
	mapChangedMetrics.emplace(ptrCIfMetric->getName(), std::move(ptrCIfMetric));

	org_eclipse_tahu_protobuf_Payload_Metric a_metric = { NULL, false, 0, true, get_current_timestamp(), true,
			METRIC_DATA_TYPE_STRING, false, 0, false, 0, false,
													true, false,
													org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
													false,
													org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
													0,
													{ 0 }
											};

	a_metric.name = "testName";
	a_metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_string_value_tag;
	a_metric.value.string_value = "String Value";
 	add_metric_to_payload(&a_payload, &a_metric);
    std::string spName = "test-sp";
    bool Result = _publishSparkplugMsg(a_payload, spName, false);
    EXPECT_EQ(true, Result);
}


/**
 * Test case to check prepareNodeDeathMsg() with Data message
 * @param :[in] bool
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, prepareNodeDeathMsg)
{
	_prepareNodeDeathMsg(true);
}



/**
 * Test case to check publish_node_birth() with Data message
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, publish_node_birth)
{
	_publish_node_birth();
}

/*
 * Test case to check publishAllDevBirths
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, publishAllDevBirths)
{
	 bool a_bIsNBIRTHProcess = false;
	_publishAllDevBirths(a_bIsNBIRTHProcess);
}


/*
 * Test case to check publish_device_birth
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, publish_device_birth)
{
	 std::string a_deviceName = "testDevice";
	 bool a_bIsNBIRTHProcess = true;
	_publish_device_birth(a_deviceName, a_bIsNBIRTHProcess);
}


/*
 * Test case to check publishMsgDDEATH
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, publishMsgDDEATH)
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
	stRefForSparkPlugAction stRef(refDev, enMSG_DEATH, mapChangedMetrics);
	bool result = _publishMsgDDEATH(stRef);
	EXPECT_EQ(true, result);
}



/*
 * Test case to check publishMsgDDEATH
 * @param :[in] Device Name
 * @param :[out] None
 * @return None
 */
TEST_F(SCADAHandler_ut, publishMsgDDEATH_DevName)
{
	 std::string a_sDevName = "Test-Dev";
	 bool result = _publishMsgDDEATH(a_sDevName);
	 EXPECT_EQ(true, result);
}


/*
 * Test case to check publishMsgDDATA
 * @param :[in]  stRefForSparkPlugAction
 * @param :[out] bool
 * @return None
 */

TEST_F(SCADAHandler_ut, publishMsgDDATA)
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
	stRefForSparkPlugAction stRef(refDev, enMSG_DATA, mapChangedMetrics);
	bool result = _publishMsgDDATA(stRef);
	EXPECT_EQ(true, result);
}


/*
 * Test case to check subscribeTopics
 * @param :[in]  subscribeTopics
 * @param :[out]
 * @return None
 */

TEST_F(SCADAHandler_ut, subscribeTopics)
{
	_subscribeTopics();
}

/*
 * Test case to check connected
 * @param :[in]  string
 * @param :[out]
 * @return None
 */
TEST_F(SCADAHandler_ut, connected)
{
	 const std::string a_sCause = "Test Cause";
	_connected(a_sCause);
}

/*
 * Test case to check disconnected
 * @param :[in]  string
 * @param :[out]
 * @return None
 */
TEST_F(SCADAHandler_ut, disconnected)
{
	 const std::string a_sCause = "Test Cause";
	_disconnected(a_sCause);
}

/*
 * Test case to check publishNewUDTs
 * @param :[in]  string
 * @param :[out]
 * @return None
 */

TEST_F(SCADAHandler_ut, publishNewUDTs)
{
	_publishNewUDTs();
}



