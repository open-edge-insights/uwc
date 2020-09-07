/************************************************************************************
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation. Title to the
 * Material remains with Intel Corporation.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or otherwise.
 ************************************************************************************/

#ifndef MQTT_HANDLER_HPP_
#define MQTT_HANDLER_HPP_

#include "mqtt/async_client.h"
#include "MQTTCallback.hpp"
#include "Logger.hpp"
#include "QueueMgr.hpp"

using namespace std;

class CIntMqttHandler
{
	mqtt::async_client m_subscriber;
	mqtt::connect_options m_connOpts;
	mqtt::token_ptr m_conntok;
	int m_QOS;

	CSubscriberCallback m_mqttSubscriberCB;
	CMQTTActionListener m_listener;

	friend class CSubscriberCallback;
	friend class CMQTTActionListener;

	CIntMqttHandler(std::string strPlBusUrl, int iQOS);

	// delete copy and move constructors and assign operators
	CIntMqttHandler(const CIntMqttHandler&) = delete;	 			// Copy construct
	CIntMqttHandler& operator=(const CIntMqttHandler&) = delete;	// Copy assign

	bool subscribeToTopics();

public:
	~CIntMqttHandler();
	static CIntMqttHandler& instance(); //function to get single instance of this class
	bool isIntMqttSubConnected();
	bool pushMsgInQ(mqtt::const_message_ptr msg);
	bool connect();
 	void cleanup();
 };

#endif