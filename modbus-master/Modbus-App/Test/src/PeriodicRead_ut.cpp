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

#include "../include/PeriodicRead_ut.hpp"
#include "ConfigManager.hpp"

#include <typeinfo>

extern void getTimeBasedParams(const CRefDataForPolling& a_objReqData, std::string &a_sTimeStamp, std::string &a_sUsec, std::string &a_sTxID);

void PeriodicRead_ut::SetUp()
{
	// Setup code
}

void PeriodicRead_ut::TearDown()
{
	// TearDown code
}

extern eMbusAppErrorCode readPeriodicRTCallBack(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);
extern eMbusAppErrorCode readPeriodicCallBack(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);


/**
 * Test case to check the behaviour of handleResponse() with NULL pointer
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, handleResponse_NULLArguments) {

	pstException->m_u8ExcCode = 0;
	pstException->m_u8ExcStatus = 0;

	try
	{
		CPeriodicReponseProcessor::Instance().handleResponse(pstMbusAppCallbackParams,
				MBUS_CALLBACK_ONDEMAND_READ,
				"Response",
				false);

		EXPECT_EQ("", test_str);
	}
	catch(std::exception &e)
	{
		test_str = e.what();
		EXPECT_EQ("", test_str);

	}

}

/**
 * Test case to check the behaviour of isInitialized() with m_bIsInitialized = true
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, isInitialized_RetTrue) {

	try
	{

		pstException->m_u8ExcCode = 0;
		pstException->m_u8ExcStatus = 0;
		u8numBytes = 10;

		Temp_Bool = CPeriodicReponseProcessor::Instance().isInitialized();
		EXPECT_EQ(true, Temp_Bool);
	}
	catch(std::exception &e)
	{
		test_str = e.what();
		EXPECT_EQ("", test_str);

	}

}

/**
 * Test case to check the behaviour of initRespHandlerThreads() when called 1st time
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, initRespHandlerThreads_CalledNTime) {

	try
	{
		/* Called 1st time */
		CPeriodicReponseProcessor::Instance().initRespHandlerThreads();
		EXPECT_EQ("", test_str);
	}
	catch(std::exception &e)
	{
		test_str = e.what();
		EXPECT_EQ("", test_str);
	}

}

/**
 * Test case to check the behaviour of readPeriodicCallBack() with NULL argument
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, readPeriodicCallBack_NULLArg1)
{
	uint16_t uTxID = 20;

	try
	{
		readPeriodicCallBack(pstMbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ("", e.what());
	}

}

/**
 * Test case to check the behaviour of isTxIDPresent() with non RT and
 * input token ID is not found in "m_mapTxIDReqData" map
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, isTxIDPresent_return)
{

	try
	{
		struct StPollingInstance stPollRef;
		std::vector<StPollingTracker> listPollTracker;
		CTimeMapper::instance().getPollingTrackerList(2, listPollTracker);

		for(auto &pollInterval: listPollTracker)
		{
			CTimeRecord &a = pollInterval.m_objTimeRecord.get();
			CRequestInitiator::instance().initiateMessages(stPollRef, a, true);
		}

		PeriodicTimer::timer_start(1000);
		EXPECT_EQ( false, CRequestInitiator::instance().isTxIDPresent(3, false) );

	}
	catch(std::exception &e)
	{
		EXPECT_EQ("", e.what()); //Test fails
	}

}

/**
 * Test case to check the behaviour of timer_Stop() with g_stopTimer as false initially,
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, timer_Stop)
{
	PeriodicTimer::timer_stop();
	EXPECT_EQ(true, g_stopTimer);
}

/**
 * Test case to check the behaviour of removeTxIDReqData() with non RT
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, removeTxIDReqData_return) {

	try
	{
		CRequestInitiator::instance().removeTxIDReqData(3, false);
		EXPECT_EQ(1, 1); //Programme should not hang

	}
	catch(std::exception &e)
	{
		EXPECT_EQ("", e.what()); //Test fails.s
	}

}

/**
 * Test case to check the behaviour of timerThread() with interval as 1000
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, timer_Thread)
{
	bool g_stopTimer = false;
	PeriodicTimer::timerThread(1000);
	EXPECT_EQ(false, g_stopTimer);

}

/**
 * Test case to check the behaviour of getPolledPointList() with invalid uiRef
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, getPolledPointList_InvalidRef)
{
	uint32_t uiRef=0;
	try
	{
		CTimeMapper::instance().getPolledPointList(uiRef, true);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ("map::at", (string)e.what());
	}
}

#ifdef MODBUS_STACK_TCPIP_ENABLED
/**
 * Test case to check the behaviour of getMinTimerFrequency() when TCPIP is enabled
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, getMinTimerFrequency_TCP)
{
	uint32_t ulMinFreq = CTimeMapper::instance().getMinTimerFrequency();
	EXPECT_EQ( 25, ulMinFreq);
}

#else
/**
 * Test case to check the behaviour of getMinTimerFrequency() when TCPIP is disabled
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, getMinTimerFrequency_RTU)
{
//	struct timespec tsPoll = {0};
//	clock_gettime(CLOCK_REALTIME, &tsPoll);
//	CTimeMapper::instance().checkTimer(1000, 1000, tsPoll);

	uint32_t ulMinFreq = CTimeMapper::instance().getMinTimerFrequency();
	EXPECT_EQ( 100, ulMinFreq);
}
#endif

/**
 * Test case to check the behaviour of checkTimer() when interval is Cutoff Interval
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, checkTimer_NotPolling)
{

	struct timespec tsPoll = {0};
	clock_gettime(CLOCK_REALTIME, &tsPoll);

	CRefDataForPolling CRefDataForPolling_obj{CUniqueDataPoint_obj, 100};

	CTimeRecord CTimeRecord_obj{600, CRefDataForPolling_obj};

	CTimeMapper::instance().addToPollingTracker(600, CTimeRecord_obj, false);
	CTimeMapper::instance().checkTimer(600, 600, tsPoll);

}

/**
 * Test case to check the behaviour of checkTimer() when interval is Polling Interval
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, checkTimer_IsPolling)
{
	struct timespec tsPoll = {0};
	clock_gettime(CLOCK_REALTIME, &tsPoll);

	CRefDataForPolling CRefDataForPolling_obj{CUniqueDataPoint_obj, 100};
	CTimeRecord CTimeRecord_obj{600, CRefDataForPolling_obj};

	CTimeMapper::instance().addToPollingTracker(600, CTimeRecord_obj, true);
	CTimeMapper::instance().checkTimer(600, 600, tsPoll);
}

/**
 * Test case to check the behaviour of setScaledValue() for datatype int with 1 width register and 
 * scalefactor 20
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, ScaleValue)
{
	std::string Value = "0x12";
	std::string dataType = "int";
	stOnDemandRequest reqData ;
	int width = reqData.m_iWidth =1;
	double ScaleFactor = 20;
        //msg_envelope_elem_body_t *p;
	msg_envelope_elem_body_t* ptScaleValue = CPeriodicReponseProcessor::Instance().setScaledValue(Value, dataType, ScaleFactor, width);  
        EXPECT_EQ(360, ptScaleValue->body.integer);
}

/**
 * Test case to check the behaviour of setScaledValue() for datatype int with 2 width register and 
 * scalefactor 20
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PeriodicRead_ut, ScaleValueInt_1)
{
    std::string Value = "0x24";
	std::string dataType = "int";
	stOnDemandRequest reqData ;
	int width = reqData.m_iWidth =2;
	double ScaleFactor = 20;
	msg_envelope_elem_body_t* ptScaleValue = CPeriodicReponseProcessor::Instance().setScaledValue(Value, dataType, ScaleFactor, width);
        EXPECT_EQ(720, ptScaleValue->body.integer);
}




