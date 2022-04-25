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
