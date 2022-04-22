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

#include "../include/ZmqHandler_ut.hpp"

extern int char2int(char input);



void ZmqHandler_ut::SetUp()
{
	// Setup code
}

void ZmqHandler_ut::TearDown()
{
	// TearDown code
}


/* This test is to check whether topic type is getting or not */
//************check for the topic is other than pub or sub in PubTopic************
TEST_F(ZmqHandler_ut, prepare_test_other1)
{

	if(getenv("PubTopics") != NULL)
	{
		bool bRes = zmq_handler::prepareCommonContext("Other");

		if(!bRes)
		{

			retValue =bRes;

			EXPECT_EQ(false, retValue);
		}
		else{
			retValue =bRes;

			EXPECT_EQ(true, retValue);
		}
	}

}

/**Test for getCTX()**/
TEST_F(ZmqHandler_ut, getCTX2)
{
	try
	{

		zmq_handler::stZmqContext &busCTX = zmq_handler::getCTX("Modbus-TCP-Master_ReadRequest");
		EXPECT_EQ(NULL, busCTX.m_pContext);


	}
	catch(std::exception &e)
	{

		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (string)e.what());
	}
}

/**Test for getCTX()**/
TEST_F(ZmqHandler_ut, getCTX3)
{
	try
	{
		zmq_handler::stZmqContext &busCTX = zmq_handler::getCTX("Modbus-TCP-Master_PolledData");
		EXPECT_NE((void*)NULL, busCTX.m_pContext);

	}
	catch(std::exception &e)
	{

		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (std::string)e.what());

	}
}

/**Test for getCTX()**/
TEST_F(ZmqHandler_ut, getCTX4)
{
	try
	{
		zmq_handler::stZmqContext &busCTX = zmq_handler::getCTX("MQTT-Export/Modbus-TCP-Master_WriteRequest");
		EXPECT_NE((void*)NULL, busCTX.m_pContext);

	}
	catch(std::exception &e)
	{

		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (std::string)e.what());

	}
}



/***************************getSubCTX()***********************************/
//this test checks for correct context for correct sub topic accordingly
//and throws exceptions accordingly

/**Test for getSubCTX()**/
TEST_F(ZmqHandler_ut, getSubCtx)
{

	try
	{

		zmq_handler::prepareCommonContext("fgdgrpub");

		busSubCTX = zmq_handler::getSubCTX("Modbus-TCP-Master_ReadRequest");
		EXPECT_NE((void*)NULL, busSubCTX.sub_ctx);

	}
	catch(std::exception &e)
	{
		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (std::string)e.what());
	}

}


/******************************************getPubCTX()***********************************/
/**Test for getPubCTX()**/
TEST_F(ZmqHandler_ut, getPubCtx)
{
	zmq_handler::stZmqPubContext objTempPubCtx;
	std::string topic = "Modbus-TCP-Master_ReadRequest";

	try
	{

		zmq_handler::insertPubCTX(topic, objTempPubCtx);
		zmq_handler::getPubCTX(topic);
		busPubCTX = zmq_handler::getPubCTX("Modbus-TCP-Master_ReadRequest");

	}
	catch(std::exception &e)
	{
		EXPECT_EQ("map::at", (std::string)e.what());

	}

}
/***************getPubCTX() and insertPubCTX() functions are working properly
 ***need to check******/
/***********************************insertPubCTX()***********************************/


/**Test for getPubCTX()**/
TEST_F(ZmqHandler_ut, insertpub)
{
	try
	{
		zmq_handler::insertPubCTX("Modbus-TCP-Master_PolledData", objPubContext);
		busPubCTX = zmq_handler::getPubCTX("Modbus-TCP-Master_PolledData");

	}
	catch(std::exception &e)
	{
		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (std::string)e.what());


	}
}




/*******************************removeCTX()***********************************************/

/*This test is to check the behaviour of the removeCTX() function************/
/**Test for removeCTX()**/
TEST_F(ZmqHandler_ut, removeCTX)
{
	try
	{
		zmq_handler::removeCTX("MQTT-Export/Modbus-TCP-Master_WriteRequest");

	}
	catch(std::exception &e)
	{

		std::cout<<e.what()<<std::endl;
		EXPECT_EQ("map::at", (std::string)e.what());
	}
}

/*******************************removeSubCTX()***********************************/
/**Test for removeSubCTX()**/
TEST_F(ZmqHandler_ut, removeSubCTX)
{
	try
	{
		zmq_handler::removeSubCTX("Modbus-TCP-Master_ReadRequest");

	}
	catch(std::exception &e)
	{
		std::cout<<e.what();
		EXPECT_EQ("map::at", (std::string)e.what());
	}
}

/****************************removePubCTX()************************************/
/**Test for removePubCTX()**/
TEST_F(ZmqHandler_ut, removePubCTX)
{
	try
	{
		zmq_handler::removePubCTX("Modbus-TCP-Master_PolledData");
		EXPECT_EQ(1,1);
	}
	catch(std::exception &e)
	{
		std::cout<<e.what();
		EXPECT_EQ("map::at", (string)e.what());
	}
}

/**Test for isNumber()**/
TEST_F(ZmqHandler_ut, isNumber_number)
{
	bool result = network_info::isNumber("123");
	EXPECT_EQ(1, result);
}

/**Test for isNumber()**/
TEST_F(ZmqHandler_ut, isNumber_notNumber)
{
	bool result = network_info::isNumber("Test");
	EXPECT_EQ(0, result);
}

/**Test for prepareContext()**/
TEST_F(ZmqHandler_ut, prepareContext_NULLArg_msgbus_ctx)
{
	config_t config;

	bool Res = zmq_handler::prepareContext(false,
											NULL,
											"TestStr",
											&config);

	EXPECT_EQ(false, Res);
}

