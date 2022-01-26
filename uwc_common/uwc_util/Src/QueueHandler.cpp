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

#include "QueueHandler.hpp"
#include "Logger.hpp"

/**
 * Clean up, destroy semaphores, disables callback, disconnect from MQTT broker
 * @param None
 * @return None
 */
void CQueueHandler::cleanup()
{
	sem_destroy(&m_semaphore);
}

/**
 * Clear queue
 * @param None
 * @return None
 */
void CQueueHandler::clear()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_msgQ = {};
}

/**
 * Constructor of queue manager
 * @param isRead :[in] set if operational instance is for read operation
 * 					or write (toggle between read and write)
 * @param isRealTime :[in] set if operational instance is for real-time operation
 * 					or non-RT (toggle between RT and non-RT)
 * @return None
 */
CQueueHandler::CQueueHandler()
{
	initSem();
}

/**
 * Destructor
 */
CQueueHandler::~CQueueHandler()
{
	cleanup();
}

/**
 * Initialize semaphores
 * @param None
 * @return true on success; application exits if fails to initialize semaphore
 */
bool CQueueHandler::initSem()
{
	/* Initial value of zero*/
	if(-1 == sem_init(&m_semaphore, 0, 0))
	{
	   DO_LOG_ERROR("Error :: could not create semaphore, exiting");
	   exit(0);
	}

	DO_LOG_DEBUG("Semaphore initialized successfully");

	return true;
}

/**
 * Push message in operational queue
 * @param msg :[in] MQTT message to push in message queue
 * @return true/false based on success/failure
 */
bool CQueueHandler::pushMsg(CMessageObject msg)
{
	try
	{
		fprintf(stderr, "GREEN.........4\n\n\n");
		std::lock_guard<std::mutex> lock(m_queueMutex);
		fprintf(stderr, "GREEN.........5\n\n\n");
		m_msgQ.push(msg);
        fprintf(stderr, "GREEN.........6\n\n\n");
		sem_post(&m_semaphore);
		fprintf(stderr, "GREEN.........7\n\n\n");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Push message in operational queue
 * @param msg :[in] MQTT message to push in message queue
 * @return true/false based on success/failure
 */
bool CQueueHandler::breakWaitOnQ()
{
	try
	{
		sem_post(&m_semaphore);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Retrieve non-RT read message from message queue to publish on EII
 * @param msg :[in] reference to message to retrieve from queue
 * @return true/false based on success/failure
 */
bool CQueueHandler::getSubMsgFromQ(CMessageObject& msg)
{
	try
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		if(m_msgQ.empty())
		{
			return false;
		}
		
		msg = m_msgQ.front();
		m_msgQ.pop();
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * Checks if a new message has arrived and retrieves the message
 * @param msg :[out] reference to new message
 * @return	true/false based on success/failure
 */
bool CQueueHandler::isMsgArrived(CMessageObject& msg)
{
	try
	{
		if((sem_wait(&m_semaphore)) == -1 && errno == EINTR)
		{
			// Continue if interrupted by handler
			return false;
		}
		else
		{
			if (false == getSubMsgFromQ(msg))
			{
				DO_LOG_INFO("No message to send to EII in queue");
				return false;
			}
		}
	}
	catch(std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

