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

#include "../Inc/Main_ut.hpp"

void Main_ut::SetUp()
{
	// Setup code
}

void Main_ut::TearDown()
{
	// TearDown code
}

/****************************************************************/

/**
 * Helper function to set global variable g_shouldStop as true after 2 sec
 * @param :[in] None
 * @param :[out] g_shouldStop
 * @return None
 */
static void Set_g_shouldStop()
{
	std::this_thread::sleep_for(std::chrono::seconds(2));
	g_shouldStop = true;
}

/**
 * Helper function to wait for 1 sec and then push message in the queue
 * @param :[in] Object to CQueueHandler class
 * @param :[in] message
 * @param :[out] None
 * @return None
 */
void Call_PushMsg(CQueueHandler& Qu, mqtt::const_message_ptr& msg)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	Qu.pushMsg(msg);
	g_shouldStop = true;
}
/****************************************************************/

/**
 * Test case to test processInternalMQTTMsg() with 2 topic parts and valid death topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DeathMsg)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	CVendorAppList CVendorAppList_obj;
	CSparkPlugDev CSparkPlugDev_obj("A", "A", false);

	CVendorAppList_obj.addDevice("A", CSparkPlugDev_obj);

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Death/A",
			"B",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with 2 topic parts but invalid
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DeathMsgInvalid)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/A",
			"B",
			stRefActionVec);

	EXPECT_EQ(false, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with 3 topic parts and valid birth topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_BirthMsg)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Birth/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint8\", \"value\": 100}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with 3 topic parts and valid data topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_Uint8)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint8\", \"value\": 100}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with birth message of repeated data type
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_BirthMsg_repeated_sameDataType)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Birth/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint8\", \"value\": 100}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with birth message of different data types
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_BirthMsg_repeated_DifferentDataType)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Birth/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int8\", \"value\": 9211}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with birth message of different values
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_BirthMsg_repeated_DifferentValue)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Birth/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int8\", \"value\": 420}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type boolean
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_Bool)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"boolean\", \"value\": true}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type uint16
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_uint16)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint16\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type uint32
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_uint32)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint32\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type uint64
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_uint64)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"uint64\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type int8
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_int8)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int8\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type int16
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_int16)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int16\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type int32
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_int32)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int32\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type int364
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_int64)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"int64\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type float
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_float)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"float\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type double
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_double)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"double\", \"value\": 111}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and data type string
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_string)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"string\", \"value\": \"StringValue\"}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and invalid value
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_DataMsg_InvVal)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"Data/A/B",
			"{\"metrics\": [{\"name\":\"UtData01\", \"dataType\":\"double\", \"value\": \"111\"}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with data message, and invalid value
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_BirthDataMsgInvalid)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C",
			"{\"wellhead\": \"PL0\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(false, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
				"RealDev/A/B/D/Update",
				"{\"metric\": \"UtData01\", \"status\": \"bad\", \"value\": \"0x00\", \"usec\": \"1571887474111145\", \"lastGoodUsec\": \"val1\", \"error_code\": \"2002\"}",
				stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "metric" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData_DataMissingInPayload_01)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"{\"NoMetric\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic with invalid device name
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData_InvalideDevName)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"{\"metric\": [{\"name\":\"UtData01\"}],\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "timestamp" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData_DataMissingInPayload_03)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"{\"metric\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp_Invalid\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic with invalid real device
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData_InvalidRealDev)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"{\"metric\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "value" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageData_DataMissingInPayload_05)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"{\"metric\": \"UtData02\",\"command\": \"D1\",\"value_Invalid\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and array payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayPayload)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "name" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayPayloadNameMissing)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name_Invalid\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}


TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayPayloadNameValInv)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": 100,\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayTimestampMissing)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp_Invalid\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "dataType" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayDataTypeMissing)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType_Invalid\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and invalid "dataType" in payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayDataTypeValInv)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": \"UtData02\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": 100}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with "update" topic and "value" missing from payload
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_MessageDataArrayValueMissing)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A/B/C/D/Update",
			"[{\"name\": \"UtData02\",\"command\": \"D1\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\", \"dataType\": \"uint8\"}]",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(true, Bool_Res);
}

/**
 * Test case to test processInternalMQTTMsg() with invalid topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMQTTMsg_InvalidTopic)
{
	std::vector<stRefForSparkPlugAction> stRefActionVec;

	Bool_Res = CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
			"A",
			"{\"wellhead\": \"PL0\",\"command\": \"D1\",\"value\": \"0x00\",\"timestamp\": \"2019-09-20 12:34:56\",\"usec\": \"1571887474111145\",\"version\": \"2.0\",\"app_seq\": \"1234\",\"realtime\":\"1\"}",
			stRefActionVec);

	//Does nothing
	EXPECT_EQ(false, Bool_Res);
}

/**
 * Test case to test processInternalMqttMsgs() with a death message
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMqttMsgs_DeathMsg)
{
	mqtt::const_message_ptr msg = mqtt::make_message("Death/A", "Msg_UT", 0, false);
	CQueueHandler Qu;

	std::thread TestTarget( processInternalMqttMsgs, std::ref(Qu) );
	std::thread TestHelperPostSem( Call_PushMsg, std::ref(Qu), std::ref(msg) );

	TestTarget.join();
	TestHelperPostSem.join();

	// Setting the value of "g_shouldStop back" to default value
	g_shouldStop = false;
	Qu.cleanup();
}

/**
 * Test case to test processInternalMqttMsgs() with a birth message
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processInternalMqttMsgs_BirthMsg)
{
	mqtt::const_message_ptr msg = mqtt::make_message("Birth/A", "Msg_UT", 0, false);
	CQueueHandler Qu;

	std::thread TestTarget( processInternalMqttMsgs, std::ref(Qu) );
	std::thread TestHelperPostSem( Call_PushMsg, std::ref(Qu), std::ref(msg) );

	TestTarget.join();
	TestHelperPostSem.join();

	// Setting the value of "g_shouldStop back" to default value
	g_shouldStop = false;
	Qu.cleanup();
}

/**
 * Test case to test processExternalMqttMsgs() with a spark plug message
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Main_ut, processExternalMqttMsgs_SparkPlugMessage)
{

	org_eclipse_tahu_protobuf_Payload ddata_payload;
	get_next_payload(&ddata_payload);

	size_t buffer_length = 128;
	uint8_t *binary_buffer; 
	try{
	binary_buffer = (uint8_t *)malloc(buffer_length * sizeof(uint8_t));
	if( binary_buffer == NULL )
	{
	    fprintf(stderr,"Buffer memory is not allocated");
		return;
	}
	size_t message_length = encode_payload(binary_buffer, buffer_length, &ddata_payload);
	mqtt::const_message_ptr msg = mqtt::make_message("spBv1.0/Sparkplug B Devices/DCMD/C Edge Node 1/A-B",
			(char*)binary_buffer,
			0,
			false);

	CQueueHandler Qu;
	Qu.pushMsg(msg);

	std::thread TestTarget( processExternalMqttMsgs, std::ref(Qu) );
	std::thread TestHelperPostSem( Call_PushMsg, std::ref(Qu), std::ref(msg) );

	TestTarget.join();
	TestHelperPostSem.join();

	// Setting the value of "g_shouldStop back" to default value
	g_shouldStop = false;
	}catch(std::exception &ex)
	{
		DO_LOG_ERROR(std::string("Error:") + ex.what());
	}
}


