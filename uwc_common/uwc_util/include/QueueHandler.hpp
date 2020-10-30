/************************************************************************************
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation. Title to the
 * Material remains with Intel Corporation.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or otherwise.
 ************************************************************************************/

#ifndef QHANDLER_HPP_
#define QHANDLER_HPP_

#include <atomic>
#include <map>
#include <semaphore.h>
#include "mqtt/async_client.h"
#include <queue>
#include <string>

	class CMessageObject
	{
		mqtt::const_message_ptr m_mqttMsg;
		struct timespec m_stTs;

		public:
		CMessageObject() : m_mqttMsg{}, m_stTs{}
		{
			timespec_get(&m_stTs, TIME_UTC);
		}

		CMessageObject(const std::string &a_sTopic, const std::string &a_sMsg) 
			: m_stTs{}
		{
			m_mqttMsg = mqtt::make_message(a_sTopic, a_sMsg);
			timespec_get(&m_stTs, TIME_UTC);
		}
		
		CMessageObject(mqtt::const_message_ptr a_mqttMsg) 
			: m_mqttMsg{a_mqttMsg}, m_stTs{}
		{
			timespec_get(&m_stTs, TIME_UTC);
		}

		
		CMessageObject(const CMessageObject& a_obj)
		: m_mqttMsg{a_obj.m_mqttMsg}, m_stTs{a_obj.m_stTs}
		{}

		std::string getTopic() {return m_mqttMsg->get_topic();}
		std::string getStrMsg() {return m_mqttMsg->get_payload();}
		mqtt::const_message_ptr& getMqttMsg() {return m_mqttMsg;}
		struct timespec getTimestamp() {return m_stTs;}
	};
	/**
	 * Queue handler class which implements queue operations to be used across modules
	 */
	class CQueueHandler
	{
		bool initSem();

		std::mutex m_queueMutex;
		std::queue<CMessageObject> m_msgQ;
		sem_t m_semaphore;

		// delete copy and move constructors and assign operators
		CQueueHandler& operator=(const CQueueHandler&)=delete;	// Copy assign
		CQueueHandler(const CQueueHandler&)=delete;	 			// Copy construct

	public:
		CQueueHandler();
		virtual ~CQueueHandler();

		bool pushMsg(CMessageObject msg);
		bool isMsgArrived(CMessageObject& msg);
		bool getSubMsgFromQ(CMessageObject& msg);

		bool breakWaitOnQ();

		void cleanup();
		void clear();
	};
#endif