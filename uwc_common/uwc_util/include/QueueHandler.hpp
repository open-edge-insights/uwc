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
/*** QueueHandler.hpp is used to handle mqtt message queue.*/

#ifndef QHANDLER_HPP_
#define QHANDLER_HPP_

#include <atomic>
#include <map>
#include <semaphore.h>
#include "mqtt/async_client.h"
#include <queue>
#include <string>

	/**
	 * Message Object class which handles mqtt message, time & topic related operations
	 */
	class CMessageObject
	{
		mqtt::const_message_ptr m_mqttMsg; /** mqtt-export message*/
		struct timespec m_stTs; /** timestamp when this object is created*/

		public:
		CMessageObject() : m_mqttMsg{}, m_stTs{}
		{
			timespec_get(&m_stTs, TIME_UTC);
		}

		CMessageObject(const std::string &a_sTopic, const std::string &a_sMsg) 
			: m_stTs{}
		{
			fprintf(stderr, "GREEN.........1\n\n\n");
			m_mqttMsg = mqtt::make_message(a_sTopic, a_sMsg);
			fprintf(stderr, "GREEN.........2\n\n\n");
			timespec_get(&m_stTs, TIME_UTC);
			fprintf(stderr, "GREEN.........3\n\n\n");
		}
		
		CMessageObject(mqtt::const_message_ptr a_mqttMsg) 
			: m_mqttMsg{a_mqttMsg}, m_stTs{}
		{
			timespec_get(&m_stTs, TIME_UTC);
		}

		
		CMessageObject(const CMessageObject& a_obj)
		: m_mqttMsg{a_obj.m_mqttMsg}, m_stTs{a_obj.m_stTs}
		{}

		CMessageObject& operator=(const CMessageObject &a_obj)
	    { 
	    	m_mqttMsg = a_obj.m_mqttMsg;
	    	m_stTs = a_obj.m_stTs;
	        return *this; 
	    }

		/** function to get topic*/
		std::string getTopic() 
		{
			if (NULL == m_mqttMsg)
			{
				return "";
			}
			return m_mqttMsg->get_topic();
		}
		/** function to string msg*/
		std::string getStrMsg() 
		{
			if (NULL == m_mqttMsg)
			{
				return "";
			}
			return m_mqttMsg->get_payload();
		}
		mqtt::const_message_ptr& getMqttMsg() {return m_mqttMsg;}
		struct timespec getTimestamp() {return m_stTs;}
	};
	/**
	 * Queue handler class which implements queue operations to be used across modules
	 */
	class CQueueHandler
	{
		bool initSem();

		std::mutex m_queueMutex; /** queue mutex*/
		std::queue<CMessageObject> m_msgQ; /** message queue*/
		sem_t m_semaphore;/** semaphore*/

		// delete copy and move constructors and assign operators
		CQueueHandler& operator=(const CQueueHandler&)=delete;	// Copy assign
		CQueueHandler(const CQueueHandler&)=delete;	 			// Copy construct

	public:
		CQueueHandler();//default constructor
		virtual ~CQueueHandler();

		bool pushMsg(CMessageObject msg);
		bool isMsgArrived(CMessageObject& msg);
		bool getSubMsgFromQ(CMessageObject& msg);

		bool breakWaitOnQ();

		void cleanup();
		void clear();
	};
#endif
