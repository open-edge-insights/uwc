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

#ifndef TEST_INCLUDE_INTMQTTSUB_UT_HPP_
#define TEST_INCLUDE_INTMQTTSUB_UT_HPP_

#include "InternalMQTTSubscriber.hpp"
#include "Common.hpp"
#include "ConfigManager.hpp"
#include "SparkPlugDevices.hpp"
#include <gtest/gtest.h>

class InternalMQTTSubscriber_ut : public::testing::Test
{
protected:
	virtual void SetUp();
	virtual void TearDown();

public:

	std::string			Test_Str = "";
	std::string			Expected_output = "";
	std::vector<stRefForSparkPlugAction> stRefActionVec;
	std::vector<stRefForSparkPlugAction> stRefActionVec1;
	std::reference_wrapper<CSparkPlugDev> *a_ref;
		eMsgAction a_enAction = enMSG_NONE;
		//metricMap_t m_mapChangedMetrics;
		metricMapIf_t m_mapChangedMetrics;

	stRefForSparkPlugAction stDummyAction{*a_ref, a_enAction, m_mapChangedMetrics};

	void _subscribeTopics()
	{
		CIntMqttHandler::instance().subscribeTopics();
	}

};

#endif /* TEST_INCLUDE_INTMQTTSUB_UT_HPP_  */
