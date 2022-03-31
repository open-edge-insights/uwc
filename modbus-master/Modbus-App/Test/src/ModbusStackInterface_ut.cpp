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

#include "../include/ModbusStackInterface_ut.hpp"
#include "Common.hpp"

void ModbusStackInterface_ut::SetUp()
{
	// Setup code
}

void ModbusStackInterface_ut::TearDown()
{
	// TearDown code
}

// /**
//  * Test case to check the behaviour of OnDemandRead_AppCallback()
//  * @param :[in] None
//  * @param :[out] None
//  * @return None
//  */
// TEST_F(ModbusStackInterface_ut, AppCallback01)
// {
// #ifdef MODBUS_STACK_TCPIP_ENABLED

// 	try
// 	{
// 		stOnDemandRequest reqData;
// 		stOnDemandRequest onDemandReqData ;
// 		MbusAPI_t stMbusApiPram = {};

// 		uint16_t u16TxID = 52;
// 		reqData.m_isByteSwap = true;
// 		reqData.m_isWordSwap = false;
// 		reqData.m_obtReqRcvdTS.tv_nsec = 21132323;
// 		reqData.m_obtReqRcvdTS.tv_sec = 1;
// 		reqData.m_strAppSeq = "455";
// 		reqData.m_strMetric = "Test";
// 		reqData.m_strTopic = "kzdjfhdszh";
// 		reqData.m_strVersion = "2.1";
// 		reqData.m_strWellhead = "test";
// 		uint16_t uTxID = 20;

// 		stMbusAppCallbackParams.m_lPriority = 1;
// 		stMbusAppCallbackParams.m_objTimeStamps.tsReqRcvd.tv_nsec = 10;
// 		stMbusAppCallbackParams.m_objTimeStamps.tsReqRcvd.tv_sec = 20;
// 		stMbusAppCallbackParams.m_objTimeStamps.tsReqSent.tv_nsec = 15;
// 		stMbusAppCallbackParams.m_objTimeStamps.tsRespRcvd.tv_nsec = 18;
// 		stMbusAppCallbackParams.m_objTimeStamps.tsRespSent.tv_sec = 5;
// 		stMbusAppCallbackParams.m_u16Quantity = 0;
// 		stMbusAppCallbackParams.m_u16StartAdd = 0;
// 		stMbusAppCallbackParams.m_u16TransactionID = 52;
// 		stMbusAppCallbackParams.m_u8ExceptionExcCode = 1;
// 		stMbusAppCallbackParams.m_u8ExceptionExcStatus = 1;
// 		stMbusAppCallbackParams.m_u8FunctionCode = READ_COIL_STATUS;
// 		stMbusAppCallbackParams.m_u8MbusRXDataLength = 100;
// 		stMbusAppCallbackParams.m_u8UnitID = 0;
// 		stMbusAppCallbackParams.u16Port = 0;

// 		common_Handler::insertReqData(u16TxID, stMbusApiPram);

// 		OnDemandRead_AppCallback(pstMbusAppCallbackParams, uTxID);

// 	}
// 	catch(std::exception &e)
// 	{
// 		EXPECT_EQ("", e.what());
// 	}
// }

// #else
// const char* pcWriteRespTopic = std::getenv("WRITE_RESPONSE_TOPIC");

// try
// {
// 	msg_envelope_t *msg = NULL;
// 	stOnDemandRequest reqData;
// 	uint16_t uTxID = 20;
// 	uint16_t u16TxID = 52;
// 	stOnDemandRequest onDemandReqData;

// 	common_Handler::insertReqData(u16TxID, stMbusApiPram);
// 	OnDemandRead_AppCallback(pstMbusAppCallbackParams, uTxID);

// }
// catch(std::exception &e)
// {
// 	EXPECT_EQ(" ", e.what());
// }
// }
// #endif


// /************************************MOdbusMAster_AppCallback()*******************************/
// /**
//  * Test case to check the behaviour of OnDemandRead_AppCallback()
//  * @param :[in] None
//  * @param :[out] None
//  * @return None
//  */
// TEST_F(ModbusStackInterface_ut, AppCallback_null_ip)
// {
// #ifdef MODBUS_STACK_TCPIP_ENABLED

// 	try
// 	{
// 		fprintf(stderr,"purple..1\n");
// 		stOnDemandRequest reqData;
// 		stOnDemandRequest onDemandReqData ;
// 		uint16_t uTxID = 20;
// 		fprintf(stderr,"purple..2\n");
// 		OnDemandRead_AppCallback(MbusAppCallbackParams, uTxID);

// 	}
// 	catch(std::exception &e)
// 	{
// 		EXPECT_EQ(" ", e.what());
// 	}
// }

// #else
// const char* pcWriteRespTopic = std::getenv("WRITE_RESPONSE_TOPIC");

// try
// {
// 	msg_envelope_t *msg = NULL;
// 	stOnDemandRequest reqData;
// 	stOnDemandRequest onDemandReqData;
// 	uint16_t uTxID = 20;

// 	OnDemandRead_AppCallback(MbusAppCallbackParams, uTxID);

// }
// catch(std::exception &e)
// {
// 	EXPECT_EQ(" ", e.what());
// }
// }
// #endif


/**
 * Test case to check the behaviour of OnDemandReadRT_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandRead_RT)
{
	uint16_t uTxID = 20;

	try
	{
		OnDemandReadRT_AppCallback(pstMbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ", (std::string)e.what());
	}
}


/**
 * Test case to check the behaviour of OnDemandReadRT_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandRead_RT_NULL)
{
	uint16_t uTxID = 20;
	try
	{
		OnDemandReadRT_AppCallback(MbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ",(std::string)e.what());
	}
}

/**
 * Test case to check the behaviour of OnDemandWrite_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandWrite)
{
	uint16_t uTxID = 20;
	try
	{
		OnDemandWrite_AppCallback(pstMbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ", (std::string)e.what());
	}
}

/**
 * Test case to check the behaviour of OnDemandWrite_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandWrite_NULL)
{
	uint16_t uTxID = 20;
	try
	{
		OnDemandWrite_AppCallback(MbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ", (std::string)e.what());
	}
}

/**
 * Test case to check the behaviour of OnDemandWriteRT_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandWrite_RT)
{
	uint16_t uTxID = 20;
	try
	{
		OnDemandWriteRT_AppCallback(pstMbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ", (std::string)e.what());
	}
}

/**
 * Test case to check the behaviour of OnDemandWriteRT_AppCallback()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, OnDemandWrite_RT_NULL)
{
	uint16_t uTxID = 20;
	try
	{
		OnDemandWriteRT_AppCallback(MbusAppCallbackParams, uTxID);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ(" ", (std::string)e.what());
	}
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_Readcoil)
{

	int retval = Modbus_Stack_API_Call(READ_COIL_STATUS,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_ReadInStatus)
{

	uint8_t retval = Modbus_Stack_API_Call(READ_INPUT_STATUS,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_ReadHoldReg)
{

	uint8_t retval = Modbus_Stack_API_Call(READ_HOLDING_REG,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_ReadInReg)
{

	uint8_t retval = Modbus_Stack_API_Call(READ_INPUT_REG,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_WriteSingleCoils)
{


	uint8_t retval = Modbus_Stack_API_Call(WRITE_SINGLE_COIL,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_WriteSingleReg)
{

	uint8_t retval = Modbus_Stack_API_Call(WRITE_SINGLE_REG,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_WriteMulCoils)
{

	uint8_t retval = Modbus_Stack_API_Call(WRITE_MULTIPLE_COILS,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}

/**
 * Test case to check the behaviour of Modbus_Stack_API_Call()
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(ModbusStackInterface_ut, Modbus_Stack_API_Call_WriteMulReg)
{

	uint8_t retval = Modbus_Stack_API_Call(WRITE_MULTIPLE_REG,
			pstMbusApiPram,
			NULL);

	EXPECT_EQ(11, retval);
}
