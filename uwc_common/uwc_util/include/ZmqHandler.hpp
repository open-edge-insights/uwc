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

/*** ZmqHandler.hpp is To prepare and manage context related information for ZMQ communication*/

#ifndef INCLUDE_INC_ZMQHANDLDER_HPP_
#define INCLUDE_INC_ZMQHANDLDER_HPP_

#include <string>
#include <map>
#include <functional>
#include <mutex>
#include "cjson/cJSON.h"

#include <eii/utils/thread_safe_queue.h>
#include <eii/utils/config.h>
#include <eii/utils/json_config.h>
#include <eii/msgbus/msgbus.h>
//eii configmgr
#include "eii/config_manager/config_mgr.hpp"
// uwc configmgr
#include "ConfigManager.hpp"
#include "CommonDataShare.hpp"
#include "LibErrCodeManager.hpp"
/** removepubctx.. keep that & as local variable*/
/** return ctx from creating */
extern std::function<bool(std::string, std::string)> regExFun;

/** zmq_handler is a namespace holding context regarding information and functions for zmq communication*/
namespace zmq_handler
{
	void* m_pub_msgbus_ctx = NULL;
	config_t* m_pub_config;
	/** structure maintain context related information*/
	struct stZmqContext
	{
		void *m_pContext;/**msg bus context*/
		std::mutex m_mutex; /** mutex for msg*/

		stZmqContext(void *a_pContext):m_pContext{a_pContext}, m_mutex{} {;};
		stZmqContext(const stZmqContext &a_copy):m_pContext{a_copy.m_pContext}, m_mutex{} {;};
		stZmqContext& operator=(const stZmqContext &a_copy)=delete;
	};

	/** structure maintaining zmq publish context*/
	struct stZmqPubContext
	{
		void *m_pContext; /**msg bus context*/
	};
	/** structure maintaining zmq subscribe context*/
	struct stZmqSubContext
	{
		recv_ctx_t* sub_ctx; /** sub context*/
	};

	/**
	 * Prepare all EII contexts for zmq communications based on topic configured in
	 * SubTopics or PubTopics section from docker-compose.yml file
	 * Following is the sequence of context creation
	 * 	1. Get the topic from SubTopics/PubTopics section
	 * 	2. Create msgbus config
	 * 	3. Create the msgbus context based on msgbus config
	 * 	4. Once msgbus context is successful then create pub and sub context for zmq publisher/subscriber
	 *
	 * @param topicType	:[in] topic type to create context for, value is either "sub" or "pub"
	 * @return 	true : on success,
	 * 	        false : on error
	 */
	bool prepareCommonContext(std::string topicType);

	bool prepareContext(bool a_bIsPub,
			void* msgbus_ctx,
			std::string a_sTopic,
			config_t *config);

	/** function to set topic for different operations*/
	bool setTopicForOperation(std::string a_sTopic);

	/** function to get message bus context based on topic*/
	stZmqContext& getCTX(std::string str_Topic);

	/** function to insert new entry in map*/
	void insertCTX(std::string, stZmqContext& );

	/** function to remove entry from the map once reply is sent*/
	void removeCTX(std::string);

	/** function to get message bus context based on topic*/
	stZmqSubContext& getSubCTX(std::string str_Topic);

	/** function to insert new entry in map*/
	void insertSubCTX(std::string, stZmqSubContext );

	/** function to remove entry from the map once reply is sent*/
	void removeSubCTX(std::string);

	/** function to get message bus publish context based on topic*/
	stZmqPubContext& getPubCTX(std::string str_Topic);

	/** function to insert new entry in map*/
	bool insertPubCTX(std::string, stZmqPubContext );

	/** function to remove entry from the map*/
	void removePubCTX(std::string);

	/** function to publish json data on ZMQ*/
	bool publishJson(std::string &a_sUsec, msg_envelope_t* msg, const std::string &a_sTopic, std::string a_sPubTimeField);

	/**
	 *  function to return all pub/sub topics
	 *  @param topicType     : [in] pub or sub
	 *  @param vector         : [out] vector of topics of pub/sub
	 *  @return bool          : true for success. false for failure
	 **/
	bool returnAllTopics(std::string topicType, std::vector<std::string>& vecTopics);

	/**
	 *  function to return number of pub/sub topics
	 *  @param topicType     : [in] pub or sub
	 *  @return size_t       : [out] count of publishers or subscribers
	 **/
	 size_t getNumPubOrSub(std::string topicType);
}

#endif /* INCLUDE_INC_ZMQHANDLDER_HPP_ */
