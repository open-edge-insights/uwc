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

#include <iostream>
#include <atomic>
#include <algorithm>
#include <map>
#include "ZmqHandler.hpp"
#include <mutex>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <functional>
#include "ConfigManager.hpp"

using namespace eii::config_manager;
using namespace zmq_handler;

std::mutex fileMutex;
std::mutex __ctxMapLock;
std::mutex __SubctxMapLock;
std::mutex __PubctxMapLock;
std::mutex __mtxUniqueTracker;
std::mutex __mtxMakePubThSafe;

// Unnamed namespace to define globals
namespace
{
	std::map<std::string, stZmqContext> g_mapContextMap;
	std::map<std::string, stZmqSubContext> g_mapSubContextMap;
	std::map<std::string, stZmqPubContext> g_mapPubContextMap;
	std::map<std::string,int> g_mapUniqueTopicTracker;
	stPubCtxCfg g_pubCtxCfg;
	// Check if EMB or Mqtt is specified in config
	bool enable_EMB;
	// Storing topic and corresponding RT/NRT check
	std::map<std::string,std::string> RT_NRT;
	}

// lamda function to return true if given key matches the given pattern
std::function<bool(std::string, std::string)> regExFun = [](std::string a_sTopic, std::string a_sKeyToFind) ->bool {
	if(std::string::npos != a_sTopic.find(a_sKeyToFind.c_str(),
			a_sTopic.length() - std::string(a_sKeyToFind).length(),
			std::string(a_sKeyToFind).length()))
	{
		return true;
	}
	else
	{
		return false;
	}
};
/**
 * function to form the topic in /flowmeter/PL0/D1 format and corresponding RT/NRT value
 * @param RT_NRT_check :[in] information about device ,wellhead, data_point and Real time seprated by -
 * @return None,
 */

void zmq_handler::set_RT_NRT(std::string RT_NRT_check)
{
	std::string delimiter = "-";
	size_t last = 0;
	size_t next = 0;
	std::string value;
	std::string key;
	int size = RT_NRT_check.size();
	// Checking if topic is default then storing the default value in map
	if((RT_NRT_check.find("default-"))!= std::string::npos){
		next = RT_NRT_check.find("-");
		value = RT_NRT_check.substr(next+1,size);
		RT_NRT.insert({"default",value});
	}else{
		// extracting topic and corresponding RT/NRT value
		std::vector<std::string> a_vsrt_nrt_values;

		while ((next = RT_NRT_check.find(delimiter, last)) != std::string::npos)
		{
			a_vsrt_nrt_values.push_back(RT_NRT_check.substr(last, next - last));
			last = next + 1;
		}
		a_vsrt_nrt_values.push_back(RT_NRT_check.substr(last));
		size = a_vsrt_nrt_values.size();
		key = "/" + a_vsrt_nrt_values[0] + "/" + a_vsrt_nrt_values[1] + "/" +a_vsrt_nrt_values[2];
		if(size==4){
			value = a_vsrt_nrt_values[3];
			RT_NRT.insert({key,value});
		}
	}
	
}
/**
 * To get the corresponding RT/NRT value for the topic
 * @param topic              :[in] Mqtt topic 
 * @return  RT/NRT value for the topic,
 */
std::string zmq_handler::get_RT_NRT(std::string topic)
{
   std::string RT_NRT_value ="";
   auto RT_NRT_pointer = RT_NRT.find(topic);
   if(RT_NRT_pointer!=RT_NRT.end()){
   		RT_NRT_value = RT_NRT_pointer->second;
    }
   return RT_NRT_value;   
}
/**
 * Prepare pub or sub context for ZMQ communication
 * @param a_bIsPub		:[in] flag to check for Pub or Sub
 * @param msgbus_ctx	:[in] Common message bus context used for zmq communication
 * @param a_sTopic		:[in] Topic for which pub or sub context needs to be created
 * @param config		:[in] Config instance used for zmq library
 * @return 	true : on success,
 * 			false : on error
 */
bool zmq_handler::prepareContext(bool a_bIsPub, 
		void* msgbus_ctx,
		std::string a_sTopic,
		config_t *config)
{
	if(a_bIsPub && isPubTopicPresentInMap(a_sTopic)) { // if pub topic
		DO_LOG_DEBUG("This Pub topic" + a_sTopic + "is already present in the map, hence no need of getting the context again for this, so just exit the prepareContext");
		return false; // If this PUB topic "a_sTopic" is already present in the MAP then no need of getting the context again for this, so just exit the prepareContext().
	}
	if(!a_bIsPub && isSubTopicPresentInMap(a_sTopic)) { // if sub topic
		DO_LOG_DEBUG("This SUB topic " + a_sTopic + "is already present in the map, hence no need of getting the context again for this, so just exit the prepareContext");
		return false; // If this SUB topic "a_sTopic" is already present in the MAP then no need of getting the context again for this, so just exit the prepareContext().
	}

	bool bRetVal = false;
	msgbus_ret_t retVal = MSG_SUCCESS;
	publisher_ctx_t* pub_ctx = NULL;
	recv_ctx_t* sub_ctx = NULL;
	bool tempIsPub = false;
	if(NULL == msgbus_ctx || NULL == config || a_sTopic.empty())
	{
		DO_LOG_ERROR("NULL pointers received while creating context for topic ::" + a_sTopic);
		//< Failed to create publisher or subscriber topic's message bus context so remove context, destroy message bus context created.
		goto err;
	}

	if(a_bIsPub) // if pub
	{
		if(!isPubTopicPresentInMap(a_sTopic)) {			
			if(isTopicUnique(a_sTopic)) {
				std::unique_lock<std::mutex> pubLock(__mtxMakePubThSafe);
				retVal = msgbus_publisher_new(msgbus_ctx, a_sTopic.c_str(), &pub_ctx);
				pubLock.unlock();
				tempIsPub = true;
			}
		}
	}
	else // else if sub
	{
		if(!isSubTopicPresentInMap(a_sTopic)) {
			retVal = msgbus_subscriber_new(msgbus_ctx, a_sTopic.c_str(), NULL, &sub_ctx);
			tempIsPub = false;
		}
	}

	if(retVal != MSG_SUCCESS)
	{
		/// cleanup
		if(tempIsPub) {
			DO_LOG_ERROR("Failed to create publisher for topic "+a_sTopic + " with error code:: "+std::to_string(retVal));
		} else {
			DO_LOG_ERROR("Failed to create  subscriber for topic "+a_sTopic + " with error code:: "+std::to_string(retVal));
		}
		goto err;
	}
	else
	{
		bRetVal = true;
		stZmqContext objTempCtx{msgbus_ctx};
		zmq_handler::insertCTX(a_sTopic, objTempCtx);
		//TODO: INserting all PUB topics into this map will grow the map very long which might not be needed. So, this logic needs to be uupdated.
		if(a_bIsPub)
		{
			stZmqPubContext objTempPubCtx;
			objTempPubCtx.m_pContext= pub_ctx;
			zmq_handler::insertPubCTX(a_sTopic, objTempPubCtx);
		}
		else
		{
			stZmqSubContext objTempSubCtx;
			objTempSubCtx.sub_ctx= sub_ctx;
			zmq_handler::insertSubCTX(a_sTopic, objTempSubCtx);
		}
	}
	return bRetVal;

err:
	// remove mgsbus context
	removeCTX(a_sTopic);

	if(NULL != pub_ctx && NULL != config)
	{
		msgbus_publisher_destroy(config, pub_ctx);
	}
	if(NULL != sub_ctx && NULL != config)
	{
		msgbus_recv_ctx_destroy(config, sub_ctx);
	}
	/// free msg bus context
	if(msgbus_ctx != NULL)
	{
		msgbus_destroy(msgbus_ctx);
		msgbus_ctx = NULL;
	}

	return false;
}


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
 * 			false : on error
 */
bool zmq_handler::prepareCommonContext(std::string topicType)
{
	DO_LOG_DEBUG("Start: zmq_handler::prepareCommonContext");
	bool retValue = false;

	PublisherCfg* pub_ctx;
	SubscriberCfg* sub_ctx;
	config_t* pub_config;
	config_t* sub_config;
	void* g_msgbus_ctx = NULL;

	if(!(topicType == "pub" || topicType == "sub"))
	{
		DO_LOG_ERROR("Invalid TopicType parameter ::" + topicType);
		return retValue;
	}

	// This check if EII cfgmgr is created or not
	if(CfgManager::Instance().IsClientCreated())
	{
		if(topicType == "pub") {
			int numPublishers = CfgManager::Instance().getEiiCfgMgr()->getNumPublishers();
			for(auto it =0; it<numPublishers; ++it) {
				pub_ctx = CfgManager::Instance().getEiiCfgMgr()->getPublisherByIndex(it);
				pub_config = pub_ctx->getMsgBusConfig();

    			if (pub_config == NULL) {
        			DO_LOG_ERROR("Failed to get message bus config");
        			return false;
    			}
				g_pubCtxCfg.m_pub_config = pub_config;
				char* pub_config_char = configt_to_char(pub_config);
				std::string pub_config_str = std::string(pub_config_char);

				g_msgbus_ctx = msgbus_initialize(pub_config);
    			if (g_msgbus_ctx == NULL) {
        			DO_LOG_ERROR("Failed to initialize message bus");
        			return false;
    			}
				g_pubCtxCfg.m_pub_msgbus_ctx = g_msgbus_ctx;
			}
		} 
		else {  // else if its sub
				int numSubscribers = CfgManager::Instance().getEiiCfgMgr()->getNumSubscribers();
				for(auto it =0; it<numSubscribers; ++it) {
					sub_ctx = CfgManager::Instance().getEiiCfgMgr()->getSubscriberByIndex(it);
					sub_config = sub_ctx->getMsgBusConfig();
    				if (sub_config == NULL) {
        				DO_LOG_ERROR("Failed to get message bus config");
        				return false;
    				}
					g_msgbus_ctx = msgbus_initialize(sub_config);
    				if (g_msgbus_ctx == NULL) {
        				LOG_ERROR_0("Failed to initialize message bus");
        				return false;
    				}
					std::vector<std::string> topics = sub_ctx->getTopics();
					if(topics.empty()){
        				DO_LOG_ERROR("Failed to get topics");
        				return false;
    				}
					for (auto topic_it = 0; topic_it < topics.size(); topic_it++) {
     					std::string ind_topic = topics.at(topic_it);
						prepareContext(false, g_msgbus_ctx, ind_topic, sub_config);
    				}
				}
		}	// end of sub part else
	} // end of if eii configmgr created if() 
	else
	{
		DO_LOG_ERROR("EII Configmgr creation failed !! ");
	}
	DO_LOG_DEBUG("End: ");
	return true;
}
/**
 * function to check if EMB or Mqtt is specified in config
 * @param None
 * @return None,
 */
bool zmq_handler::enable_EMB(){
	ConfigMgr* pub_ch = new ConfigMgr();
	AppCfg* cfg = pub_ch->getAppConfig();
        config_value_t* app_config = cfg->getConfigValue("enable_EMB");
	bool enable_EMB = app_config->body.boolean;
    
	if(enable_EMB==true){
	 	fprintf(stderr, "\n Publishing on EII \n");
		DO_LOG_INFO("Publishing on EII");
		return true;
	}else{
		fprintf(stderr, "\n Publishing on MQTT \n");
		DO_LOG_INFO("Publishing on MQTT");
		return false;
	}

}

/**
 * Get sub context for topic
 * @param a_sTopic	:[in] topic to get sub context for
 * @return structure containing EII contexts
 */
stZmqSubContext& zmq_handler::getSubCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__SubctxMapLock);
	/// return the request ID
	return g_mapSubContextMap.at(a_sTopic);
}

/**
 * Insert sub context
 * @param a_sTopic	:[in] insert context for topic
 * @param ctxRef	:[in] reference to context
 */
void zmq_handler::insertSubCTX(std::string a_sTopic, stZmqSubContext ctxRef)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__SubctxMapLock);
	/// insert the data in map
	g_mapSubContextMap.insert(std::pair <std::string, stZmqSubContext> (a_sTopic, ctxRef));
	DO_LOG_DEBUG("End: ");
}

/**
 * Remove sub context
 * @param a_sTopic	:[in] remove sub context for topic
 */
void zmq_handler::removeSubCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__SubctxMapLock);

	g_mapSubContextMap.erase(a_sTopic);
	DO_LOG_DEBUG("End:");
}

/**
 * Get msgbus context for topic
 * @param a_sTopic :[in] topic for msgbus context
 * @return reference to structure containing contexts
 */
stZmqContext& zmq_handler::getCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__ctxMapLock);
	/// return the request ID
	return g_mapContextMap.at(a_sTopic);
}

/**
 * Insert msgbus context
 * @param a_sTopic	:[in] topic for which to insert msgbus context
 * @param ctxRef	:[in] msgbus context
 */
void zmq_handler::insertCTX(std::string a_sTopic, stZmqContext& ctxRef)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__ctxMapLock);

	/// insert the data in map
	g_mapContextMap.insert(std::pair <std::string, stZmqContext> (a_sTopic, ctxRef));
	DO_LOG_DEBUG("End: ");
}

/**
 * Remove msgbus context
 * @param a_sTopic	:[in] remove msgbus context for topic
 */
void zmq_handler::removeCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__ctxMapLock);

	g_mapContextMap.erase(a_sTopic);
	DO_LOG_DEBUG("End:");
}

/**
 * Get pub context
 * @param a_sTopic	:[in] topic for which to get pub context
 * @return reference to structure containing EII contexts
 */
stZmqPubContext& zmq_handler::getPubCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__PubctxMapLock);

	DO_LOG_DEBUG("End: ");

	/// return the context
	return g_mapPubContextMap.at(a_sTopic);
}

/**
 * Insert pub contexts
 * @param a_sTopic	:[in] topic for which to insert pub context
 * @param ctxRef	:[in] context
 * @return 	true : on success,
 * 			false : on error
 */
bool zmq_handler::insertPubCTX(std::string a_sTopic, stZmqPubContext ctxRef)
{
	DO_LOG_DEBUG("Start: ");
	bool bRet = true;
	try
	{
		std::unique_lock<std::mutex> lck(__PubctxMapLock);

		/// insert the data
		g_mapPubContextMap.insert(std::pair <std::string, stZmqPubContext> (a_sTopic, ctxRef));
	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		bRet = false;
	}
	DO_LOG_DEBUG("End: ");

	return bRet;
}

/**
 * Remove pub context
 * @param a_sTopic	:[in] topic for which to remove context
 */
void zmq_handler::removePubCTX(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: " + a_sTopic);
	std::unique_lock<std::mutex> lck(__PubctxMapLock);
	g_mapPubContextMap.erase(a_sTopic);
	DO_LOG_DEBUG("End: ");
}

/**
 * Publish json
 * @param a_sUsec		:[out] USEC timestamp value at which a message is published
 * @param msg			:[in] message to publish
 * @param a_sTopic		:[in] topic on which to publish
 * @return 	true : on success,
 * 			false : on error
 */
bool zmq_handler::publishJson(std::string &a_sUsec, msg_envelope_t* msg, const std::string &a_sTopic, std::string a_sPubTimeField)
{
	if(NULL == msg)
	{
		DO_LOG_ERROR(": Failed to publish message - Input message is NULL");
		return false;
	}
	DO_LOG_DEBUG("msg to publish :: Topic :: " + a_sTopic);
	zmq_handler::stZmqContext& msgbus_ctx = zmq_handler::getCTX(a_sTopic);
	void* pub_ctx = zmq_handler::getPubCTX(a_sTopic).m_pContext;
	if((NULL == msgbus_ctx.m_pContext) || (NULL == pub_ctx))
	{
		DO_LOG_ERROR(": Failed to publish message - context is NULL: " + a_sTopic);
		return false;
	}
	msgbus_ret_t ret;

	{
		std::lock_guard<std::mutex> lock(msgbus_ctx.m_mutex);
		if(a_sPubTimeField.empty() == false)
		{
			auto p1 = std::chrono::system_clock::now();
			unsigned long uTime = (unsigned long)(std::chrono::duration_cast<std::chrono::microseconds>(p1.time_since_epoch()).count());
			a_sUsec = std::to_string(uTime);
			msg_envelope_elem_body_t* ptUsec = msgbus_msg_envelope_new_string(a_sUsec.c_str());
			if(NULL != ptUsec)
			{
				msgbus_msg_envelope_put(msg, a_sPubTimeField.c_str(), ptUsec);
			}
		}
		ret = msgbus_publisher_publish(msgbus_ctx.m_pContext, (publisher_ctx_t*)pub_ctx, msg);
		if(ret == MSG_SUCCESS) {
			DO_LOG_DEBUG("Successfully published the message on the topic " + a_sTopic);
		}
	}

	if(ret != MSG_SUCCESS)
	{
		DO_LOG_ERROR(" Failed to publish message errno: " + std::to_string(ret));
		return false;
	}
	DO_LOG_INFO("msgbus publish is successfull");
	return true;
}

bool zmq_handler::returnAllTopics(std::string topicType, std::vector<std::string>& vecTopics) {
	int numPubsOrSubs;
	if(topicType == "pub") {
		numPubsOrSubs = CfgManager::Instance().getEiiCfgMgr()->getNumPublishers();
	} else if(topicType == "sub") {
		numPubsOrSubs = CfgManager::Instance().getEiiCfgMgr()->getNumSubscribers();
	}
	
	for(auto pub_or_sub_id=0; pub_or_sub_id<numPubsOrSubs; ++pub_or_sub_id) {
		std::vector<std::string> topics;
		if(topicType == "pub") {
			PublisherCfg* pub_ctx = CfgManager::Instance().getEiiCfgMgr()->getPublisherByIndex(pub_or_sub_id);
			topics = pub_ctx->getTopics();
		} else {
			SubscriberCfg* sub_ctx = CfgManager::Instance().getEiiCfgMgr()->getSubscriberByIndex(pub_or_sub_id);
			topics = sub_ctx->getTopics();
		}
		
		if(topics.empty()){
		DO_LOG_ERROR("Failed to get topics");
		return false;
		}
		size_t numTopics = topics.size(); // num of topics in indivisual publisher or subscriber
		for(size_t indv_topic=0; indv_topic < numTopics; ++indv_topic) {
			vecTopics.push_back(topics[indv_topic]);
		}
	}
	return true;
}

size_t zmq_handler::getNumPubOrSub(std::string topicType) {
	size_t count = 0;
	if(topicType == "pub") {
		count = CfgManager::Instance().getEiiCfgMgr()->getNumPublishers();
	} else {
		count = CfgManager::Instance().getEiiCfgMgr()->getNumSubscribers();
	}
	return count;
}
stPubCtxCfg& zmq_handler::getPubCtxCfg() {
	return g_pubCtxCfg;	
}

bool zmq_handler::isPubTopicPresentInMap(std::string pubTopic) {
	if( g_mapPubContextMap.find(pubTopic) == g_mapPubContextMap.end() ) {
		return false; // pub topic not found in map
	} else {
		return true; // pub topic found in map
	}
}

bool zmq_handler::isSubTopicPresentInMap(std::string subTopic) {
	if( g_mapSubContextMap.find(subTopic) == g_mapSubContextMap.end() ) {
		return false; // sub topic not found in map
	} else {
		return true; // sub topic found in map
	}
}

bool zmq_handler::isTopicUnique(std::string a_sTopic)
{
	DO_LOG_DEBUG("Start: ");
	bool bRet = true;
	try
	{
		std::unique_lock<std::mutex> lck(__mtxUniqueTracker);

		/// insert the data
		g_mapUniqueTopicTracker.insert(std::pair <std::string, int> (a_sTopic, 1));
		bRet=true; // yes the topic is unique
		DO_LOG_DEBUG("The topic" + a_sTopic + " is NOT inserted in hashmap yet, so inserting now");
	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		DO_LOG_DEBUG("The topic" + a_sTopic + " is already inserted in hashmap");
		bRet = false; // topic is not unique
	}
	DO_LOG_DEBUG("End: ");

	return bRet;
}
