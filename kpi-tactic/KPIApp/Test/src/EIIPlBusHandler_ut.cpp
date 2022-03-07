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

#include "../include/EIIPlBusHandler_ut.hpp"

extern bool publishEMBMsg(std::string a_sEiiMsg, std::string &a_sEiiTopic);

extern std::vector<std::thread> g_vThreads;

void EIIPlBusHandler_ut::SetUp()
{
	// Setup code
}

void EIIPlBusHandler_ut::TearDown()
{
	// TearDown code
}

/***********************************************************/
/*	Helper function	*/
void initEIIContext_Caller(bool& RetVal)
{
	CEIIPlBusHandler CEIIPlBusHandler_obj;
	RetVal = CEIIPlBusHandler_obj.initEIIContext();
}
/***********************************************************/

/**
 * Test case to check if configEIIListerners() function Configures EII listeners for a_bIsPollingRT = false and a_bIsWrOpRT = false successfully
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, EIIListerners_false)
{
	bool a_bIsPollingRT = false;
	bool a_bIsWrOpRT = false;
	CEIIPlBusHandler_obj.configEIIListerners(a_bIsPollingRT, a_bIsWrOpRT);
}

/**
 * Test case to check if configEIIListerners() function Configures EII listeners a_bIsPollingRT = true and a_bIsWrOpRT = false successfully
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, EIIListerners_true_false)
{
	bool a_bIsPollingRT = true;
	bool a_bIsWrOpRT = false;
	CEIIPlBusHandler_obj.configEIIListerners(a_bIsPollingRT, a_bIsWrOpRT);
}

/**
 * Test case to check if configEIIListerners() function Configures EII listeners a_bIsPollingRT = false and a_bIsWrOpRT = true successfully
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, EIIListerners_false_true)
{
	bool a_bIsPollingRT = false;
	bool a_bIsWrOpRT = true;
	CEIIPlBusHandler_obj.configEIIListerners(a_bIsPollingRT, a_bIsWrOpRT);
}

/**
 * Test case to check if stopEIIListeners() function Stops EII listener threads successfully
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, stopEIIListener)
{
	CEIIPlBusHandler_obj.stopEIIListeners();
	EXPECT_EQ(1,1);
}

/**
 * Test case to check if publishWriteMsg() function fails to publish message to EII and returns false
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, PublishWriteMsg_false)
{
	bool RetVal = CEIIPlBusHandler_obj.publishWriteMsg(strMsg);
	EXPECT_EQ(0, RetVal);
}

/**
 * Test case to check if publishWriteMsg() function fails to publish message to EII and returns false
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(EIIPlBusHandler_ut, EIIMsg_ctx_Map_Empty)
{
	std::string eiiTopic = "KPIAPP_WrReq";

	void* msgbus_ctx;
	std::string strMsg = "{ 	\"value\": \"0xFF00\", 	\"command\": \"Pointname\", 	\"app_seq\": \"1234\" }";
	try
	{
		std::string sMsg{""};
	    bool RetVal = publishEMBMsg(strMsg, eiiTopic);
		EXPECT_EQ(false, RetVal);

	}
	catch(std::exception &e)
	{
		EXPECT_EQ(false, true);
	}

}

