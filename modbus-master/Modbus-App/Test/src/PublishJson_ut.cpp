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


#include "../include/PublishJson_ut.hpp"

void PublishJson_ut::SetUp()
{
	// Setup code
}

void PublishJson_ut::TearDown()
{
	// TearDown code
}

/**
 * Test case to check the behaviour of isElementExistInJson() with NULL argument
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, element_exist_inJSON)
{
	cJSON *root = NULL;
	std::string a_sKeyName = "datapoints" ;
	bool result = false;
	try
	{
		result = isElementExistInJson(root, a_sKeyName);
		EXPECT_EQ(false, result);
	}
	catch(std::exception &e)
	{
		EXPECT_EQ("", e.what());
	}
}

/**
 * Test case to check the behaviour of setTopicForOperation() with empty topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, setTopicForOperation_emptyTopic)
{
	bool result;
	std::string topic;
	result = PublishJsonHandler::instance().setTopicForOperation(topic);
	EXPECT_EQ(false, result);
}

/**
 * Test case to check the behaviour of setTopicForOperation() with valid non rt topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, setTopicForOperation_ValNonRTTopic)
{
	bool result;
	std::string topic = "NRT/read";
	result = PublishJsonHandler::instance().setTopicForOperation(topic);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check the behaviour of setTopicForOperation() with valid rt topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, setTopicForOperation_ValRTTopic)
{
	bool result;
	std::string topic = "RT/read";
	result = PublishJsonHandler::instance().setTopicForOperation(topic);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check the behaviour of setTopicForOperation() with valid wrResp topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, setTopicForOperation_WrRes)
{
	bool result;
	std::string topic = "NRT/write";
	result = PublishJsonHandler::instance().setTopicForOperation(topic);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check the behaviour of setTopicForOperation() with valid wrResp rt topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(PublishJson_ut, setTopicForOperation_WRITE_RES_RT)
{
	bool result;
	std::string topic = "RT/write";
	result = PublishJsonHandler::instance().setTopicForOperation(topic);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check the behaviour of setTopicForOperation() with polled data topic
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
// TEST_F(PublishJson_ut, setTopicForOperation_PolledData)
// {
// 	bool result;
// 	std::string topic = "TCP1_PolledData";
// 	result = PublishJsonHandler::instance().setTopicForOperation(topic);
// 	EXPECT_EQ(true, result);
// }

// /**
//  * Test case to check the behaviour of setTopicForOperation() with polled data rt topic
//  * @param :[in] None
//  * @param :[out] None
//  * @return None
//  */
// TEST_F(PublishJson_ut, setTopicForOperation_PolledData_RT)
// {
// 	bool result;
// 	std::string topic = "TCP1_PolledData_RT";
// 	result = PublishJsonHandler::instance().setTopicForOperation(topic);
// 	EXPECT_EQ(true, result);
// }
