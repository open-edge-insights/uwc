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

#ifndef INCLUDE_MQTTEXPORTMAIN_UT_HPP_
#define INCLUDE_MQTTEXPORTMAIN_UT_HPP_

#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <vector>
#include <iterator>
#include <assert.h>
#include <semaphore.h>
#include <csignal>

#include <gtest/internal/gtest-internal.h>
#include <cstdlib>
#include <iostream>
#include <string>

#include "Common.hpp"
#include "MQTTSubscribeHandler.hpp"
#include "MQTTPublishHandler.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "ZmqHandler.hpp"
#include "EnvironmentVarHandler.hpp"

#include "QueueMgr.hpp"


std::string parse_msg(const char *json);
extern bool initEIIContext();
extern void postMsgstoMQTT();
extern void signalHandler(int signal);
extern bool addSrTopic(std::string &json, std::string& topic);
extern void postMsgsToEII(QMgr::CQueueMgr& qMgr);
extern bool processMsg(msg_envelope_t *msg, CMQTTPublishHandler &mqttPublisher);
extern void processMsgToSendOnEII(CMessageObject &recvdMsg, const bool isRealtime);
extern void getOperation(std::string topic, globalConfig::COperation& operation);

extern std::vector<std::thread> g_vThreads;;

extern void Temp_Function(std::string& topic);

extern void set_thread_priority_for_eii(bool& isRealtime, bool& isRead);

extern std::atomic<bool> g_shouldStop;


std::vector<std::thread> g_vThreads_UT;


class Main_ut : public ::testing::Test{

protected:
	virtual void SetUp();
	virtual void TearDown();

public:


};

#endif /* INCLUDE_MQTTEXPORTMAIN_UT_HPP_ */
