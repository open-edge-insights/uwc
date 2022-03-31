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

#ifndef TEST_INCLUDE_ZMQHANDLER_UT_HPP_
#define TEST_INCLUDE_ZMQHANDLER_UT_HPP_

#include <string>
#include <stdlib.h>
#include <map>
#include <thread>
#include "gtest/gtest.h"
#include <eii/utils/thread_safe_queue.h>
#include <eii/utils/config.h>
#include <eii/utils/json_config.h>
#include <eii/msgbus/msgbus.h>
//eii configmgr
#include "eii/config_manager/config_mgr.hpp"
#include "ConfigManager.hpp"
#include "NetworkInfo.hpp"
#include "ZmqHandler.hpp"
#include "NetworkInfo.hpp"

extern void populatePollingRefData();

class ZmqHandler_ut : public ::testing::Test {
protected:
	virtual void SetUp();
	virtual void TearDown();
public:
	msgbus_ret_t retVal = MSG_SUCCESS;
	bool retValue = false;
	recv_ctx_t* sub_ctx = NULL;
	zmq_handler::stZmqSubContext stsub_ctx;
	 std::vector<std::string> Topics;
	 zmq_handler::stZmqPubContext busPubCTX;
	 zmq_handler::stZmqSubContext busSubCTX;
	 zmq_handler::stZmqPubContext objPubContext;
	 std::string stValue = "0xFF00FF00";
	 std::string TValue = "0xFFFF";
	 unsigned char byte1;


	 zmq_handler::stZmqSubContext objTempSubCtx;
	 zmq_handler::stZmqSubContext *TempSubCtx = &objTempSubCtx;
	// objTempSubCtx.sub_ctx= sub_ctx;
	 publisher_ctx_t *pub_ctx = NULL;
	 uint16_t u16TransacID = 1;
//	 cJSON *root = cJSON_Parse(msg.c_str());
	virtual ~ZmqHandler_ut()
	{ };

};


#endif /* TEST_INCLUDE_ZMQHANDLER_UT_HPP_ */
