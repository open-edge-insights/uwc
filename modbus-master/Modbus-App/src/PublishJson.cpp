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

#include "PublishJson.hpp"
#include "ZmqHandler.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include "ZmqHandler.hpp"

#include "Logger.hpp"

using namespace zmq_handler;

/**
 * Constructor
 */
PublishJsonHandler::PublishJsonHandler()
{
	u32CutoffIntervalPercentage = 0;
}

/**
 * Return single instance of this class
 * @return
 */
PublishJsonHandler& PublishJsonHandler::instance()
{
	static PublishJsonHandler handler;
	return handler;
}

/**
 * Set the topic name as per specified patterns
 * @param a_sTopic		:[in] Topic for which pub or sub context needs to be created
 * @return 	true : on success,
 * 			false : on error
 */
bool PublishJsonHandler::setTopicForOperation(std::string a_sTopic)
{
	bool bRet = true;
	if(regExFun(a_sTopic, READ_RES))
	{
		PublishJsonHandler::instance().setSReadResponseTopic(a_sTopic);
		DO_LOG_INFO("read res topic = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, READ_RES_RT))
	{
		PublishJsonHandler::instance().setSReadResponseTopicRT(a_sTopic);
		DO_LOG_INFO("read res RT topic = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, WRITE_RES))
	{
		PublishJsonHandler::instance().setSWriteResponseTopic(a_sTopic);
		DO_LOG_INFO("write res topic = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, WRITE_RES_RT))
	{
		PublishJsonHandler::instance().setSWriteResponseTopicRT(a_sTopic);
		DO_LOG_INFO("write res RT topic = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, POLLDATA))
	{
		PublishJsonHandler::instance().setPolledDataTopic(a_sTopic);
		DO_LOG_INFO("poll topic = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, POLLDATA_RT))
	{
		PublishJsonHandler::instance().setPolledDataTopicRT(a_sTopic);
		DO_LOG_INFO("poll topic RT = " + a_sTopic);
	}
	else if(regExFun(a_sTopic, READ_REQ) or
			regExFun(a_sTopic, READ_REQ_RT) or
			regExFun(a_sTopic, WRITE_REQ) or
			regExFun(a_sTopic, WRITE_REQ_RT))
	{
		// Do nothing it just to check the correct pattern
	}
	else
	{
		DO_LOG_ERROR("Invalid topic name in SubTopics/PubTopics. hence ignoring :: " + a_sTopic);
		DO_LOG_ERROR("Kindly specify correct topic name as per specification :: " + a_sTopic);
		bRet = false;
	}
	return bRet;
}
