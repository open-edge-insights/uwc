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
#include "MQTTPubSubClient.hpp"
#include "Logger.hpp"

/**
 * This is a callback function to inform action failure related to mqtt
 * @param tok :[in] mqtt token
 * @return None
 */
void action_listener::on_failure(const mqtt::token& tok)  
{
	auto top = tok.get_topics();
	if (top && !top->empty())
	{
		DO_LOG_ERROR(name_ + ": On failure - Type: " + std::to_string(tok.get_type()) + 
					", Topic- " + (*top)[0]);
	}
	else
	{
		DO_LOG_ERROR(name_ + ": On failure - Type: " + std::to_string(tok.get_type()));
	}
}

/**
 * This is a callback function to inform action success related to mqtt
 * @param tok :[in] mqtt token
 * @return None
 */
void action_listener::on_success(const mqtt::token& tok)  
{
	auto top = tok.get_topics();
	if (top && !top->empty())
	{
		DO_LOG_DEBUG(name_ + ": Success - Type: " + std::to_string(tok.get_type()) + 
					", Topic- " + (*top)[0]);
	}
	else
	{
		DO_LOG_DEBUG(name_ + ": Success - Type: " + std::to_string(tok.get_type()));
	}
}

/**
 * Constructor: Sets all parameters needed to set a connection with MQTT broker
 * @param a_sBrokerURL :[in] MQTT broker URL
 * @param a_sClientID: [in] Client id to be used to establish a connection
 * @param a_iQOS :[in] QOS level to be used for communication with broker
 * @param a_bIsTLS :[in] Tells whether TLS connection is needed
 * @param a_sCATrustStoreSecret :[in] MQTT CA certificate, needed when TLS = true
 * @param a_sClientCertSecret :[in] MQTT client certificate, needed when TLS = true
 * @param a_sClientPvtKeySecret :[in] MQTT client private key, needed when TLS = true
 * @param a_sListener :[in] Action listener name to be used
 */
CMQTTPubSubClient::CMQTTPubSubClient(const std::string &a_sBrokerURL, std::string a_sClientID, 
	int a_iQOS, //mqtt::message_ptr &a_willMsg,
	bool a_bIsTLS, std::string a_sCATrustStoreSecret, 
	std::string a_sClientCertSecret, std::string a_sClientPvtKeySecret, 
	std::string a_sListener)
	: m_iQOS{a_iQOS}, m_sClientID{a_sClientID}, m_Client{a_sBrokerURL, a_sClientID}, m_Listener{a_sListener}
{
	try
	{
		// Check QOS value
		if(  (0 != m_iQOS) && (1 != m_iQOS) && (2 != m_iQOS) )
		{
			// Setting defalt value in case of incorrect configuration
			m_iQOS = 1;
		}
		//
		//connect options for sync publisher/client
		m_ConOptions.set_keep_alive_interval(60);
		m_ConOptions.set_clean_session(true);
		m_ConOptions.set_automatic_reconnect(1, 10);

		// set the certificates if dev mode is false
		if(true == a_bIsTLS)
		{
			if(a_sCATrustStoreSecret.empty() || a_sClientPvtKeySecret.empty()
				|| a_sClientCertSecret.empty())
			{
				throw;
			}
			mqtt::ssl_options sslopts;
			sslopts.set_trust_store(a_sCATrustStoreSecret);
			sslopts.set_key_store(a_sClientCertSecret);
			sslopts.set_private_key(a_sClientPvtKeySecret);
			sslopts.set_enable_server_cert_auth(true);
			m_ConOptions.set_ssl(sslopts);
		}

		m_Client.set_callback(*this);
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		throw;
	}
}

/**
 * This function tries to establish a connection with MQTT broker
 * @return true/false status based on success/failure
 */
bool CMQTTPubSubClient::connect()
{
	try
	{
		if(false == m_Client.is_connected())
		{
			m_Client.connect(m_ConOptions, nullptr, *this);
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * This function tries to stop a connection with MQTT broker
 * @return true/false status based on success/failure
 */
bool CMQTTPubSubClient::disconnect()
{
	try
	{
		if(true == m_Client.is_connected())
		{
			m_Client.disconnect();
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * This function subscribes to a topic on MQTT broker
 * @param a_sTopic :[in] topic to be published
 * @return none
 */
void CMQTTPubSubClient::subscribe(const std::string &a_sTopic)
{
	try
	{
		m_Client.subscribe(a_sTopic, m_iQOS, nullptr, m_Listener);
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * This function publishes a message on MQTT broker
 * @param a_pubMsg :[in] pointer to message to be published
 * @param a_bIsWaitForCompletion :[in] waits for completion
 * @return true/false status based on success/failure 
 */
bool CMQTTPubSubClient::publishMsg(mqtt::message_ptr &a_pubMsg, bool a_bIsWaitForCompletion)
{
	try
	{
		if(true == m_Client.is_connected())
		{
			a_pubMsg->set_qos(m_iQOS);
			auto pubtoken = m_Client.publish(a_pubMsg, nullptr, m_Listener);
			if(a_bIsWaitForCompletion)
			{
				pubtoken->wait();
			}
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * This is a callback function which gets called when subscriber fails to send/receive/connect
 * @param tok :[in] failed message token
 * @return None
 */
void CMQTTPubSubClient::on_failure(const mqtt::token& tok)
{
	try
	{
		if(mqtt::token::Type::CONNECT == tok.get_type())
		{
			DO_LOG_ERROR("Connection attempt failed: " + m_sClientID);
			if(m_bNotifyDisConnection)
			{
				m_fcbDisconnected("CONNECT_FAILED");
			}
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * This is a callback function which gets called when subscriber succeeds in send/receive/(re)connection
 * Either this or connected() can be used for callbacks. 
 * @param tok :[in] message token
 * @return None
 */
void CMQTTPubSubClient::on_success(const mqtt::token& tok)
{
}

/**
 * This is a callback function which gets called when subscriber is connected with MQTT broker
 * @param cause :[in] reason for connect
 * @return None
 */
void CMQTTPubSubClient::connected(const std::string& a_sCause)
{
	try
	{
		DO_LOG_INFO(m_sClientID + " Connected: " + a_sCause);
		if(m_bNotifyConnection)
		{
			m_fcbConnected(a_sCause);
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * This is a callback function which gets called when subscriber is disconnected from MQTT broker
 * @param cause :[in] cause of connection lost
 * @return None
 */
void CMQTTPubSubClient::connection_lost(const std::string& a_sCause)
{
	try
	{
		DO_LOG_ERROR(m_sClientID + ": Connection lost: " + a_sCause);

		if(m_bNotifyDisConnection)
		{
			m_fcbDisconnected("CONNECT_LOST");
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * This is a callback function which gets called when a message is arrived on subscriber
 * @param msg :[in] pointer to message arriving at subscriber
 * @return None
 */
void CMQTTPubSubClient::message_arrived(mqtt::const_message_ptr msg)
{
	try
	{
		DO_LOG_DEBUG(m_sClientID + ": Message arrived: " + msg->get_topic());
		if(m_bNotifyMsgRcvd)
		{
			m_fcbMsgRcvd(msg);
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

//----------------------------------------------------------

/**
 * Constructor: Sets all parameters needed to set a connection with MQTT broker
 * @param a_sBrokerURL :[in] MQTT broker URL
 * @param a_sClientID: [in] Client id to be used to establish a connection
 * @param a_iQOS :[in] QOS level to be used for communication with broker
 * @param a_bIsTLS :[in] Tells whether TLS connection is needed
 * @param a_sCaCert :[in] MQTT CA certificate, needed when TLS = true
 * @param a_sClientCert :[in] MQTT client certificate, needed when TLS = true
 * @param a_sClientKey :[in] MQTT client private key, needed when TLS = true
 * @param a_sListener :[in] Action listener name to be used
 */
CMQTTBaseHandler::CMQTTBaseHandler(const std::string &a_sBrokerURL, const std::string &a_sClientID,
			int a_iQOS, bool a_bIsTLS, const std::string &a_sCaCert, const std::string &a_sClientCert,
			const std::string &a_sClientKey, const std::string &a_sListener):
	m_MQTTClient(a_sBrokerURL, a_sClientID, a_iQOS, a_bIsTLS, 
	a_sCaCert, a_sClientCert, a_sClientKey, a_sListener)
{
	try
	{
		m_QOS = a_iQOS;

		m_MQTTClient.setNotificationConnect(std::bind(&CMQTTBaseHandler::connected, this, std::placeholders::_1));
		m_MQTTClient.setNotificationDisConnect(std::bind(&CMQTTBaseHandler::disconnected, this, std::placeholders::_1));
		m_MQTTClient.setNotificationMsgRcvd(std::bind(&CMQTTBaseHandler::msgRcvd, this, std::placeholders::_1));

		DO_LOG_DEBUG("MQTT initialized successfully");
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Destructor
 */
CMQTTBaseHandler::~CMQTTBaseHandler()
{
	disconnect();
}

/**
 * This is a callback function which gets called when subscriber is connected with MQTT broker
 * @param a_sCause :[in] reason for connect
 * @return None
 */
void CMQTTBaseHandler::connected(const std::string &a_sCause)
{
	DO_LOG_DEBUG(a_sCause);
}

/**
 * This is a callback function which gets called when subscriber is disconnected with MQTT broker
 * @param a_sCause :[in] reason for disconnect
 * @return None
 */
void CMQTTBaseHandler::disconnected(const std::string &a_sCause)
{
	DO_LOG_DEBUG(a_sCause);
}

/**
 * This is a callback function which gets called when a msg is received
 * @param a_pMsg :[in] pointer to received message
 * @return None
 */
void CMQTTBaseHandler::msgRcvd(mqtt::const_message_ptr a_pMsg)
{
	DO_LOG_DEBUG(a_pMsg->get_topic() + ", Msg: " + a_pMsg->get_payload());
}

/**
 * Helper function to disconnect MQTT client from Internal MQTT broker
 * @param None
 * @return None
 */
void CMQTTBaseHandler::disconnect()
{
	try
	{
		if(true == m_MQTTClient.isConnected())
		{
			m_MQTTClient.disconnect();
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * Helper function to connect MQTT client to Internal MQTT broker
 * @param None
 * @return None
 */
void CMQTTBaseHandler::connect()
{
	try
	{
		DO_LOG_INFO("Connecting to Internal MQTT ... ");
		m_MQTTClient.connect();
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * Checks if internal MQTT subscriber has been connected with the  MQTT broker
 * @param none
 * @return true/false as per the connection status of the external MQTT subscriber
 */
bool CMQTTBaseHandler::isConnected()
{
	return m_MQTTClient.isConnected();
}

/**
 * Publish message on MQTT broker for MQTT-Export
 * @param a_sMsg :[in] message to publish
 * @param a_sTopic :[in] topic on which to publish message
 * @return true/false based on success/failure
 */
bool CMQTTBaseHandler::publishMsg(const std::string &a_sMsg, const std::string &a_sTopic)
{
	try
	{
		// Check if topic is blank
		if (true == a_sTopic.empty())
		{
			DO_LOG_ERROR("Blank topic. Message not posted");
			return false;
		}
		mqtt::message_ptr pubmsg = mqtt::make_message(a_sTopic, a_sMsg, m_QOS, false);
		m_MQTTClient.publishMsg(pubmsg);

		DO_LOG_DEBUG("Published message on Internal MQTT broker successfully with QOS:"+ std::to_string(m_QOS));

		return true;
	}
	catch (const mqtt::exception &exc)
	{
		DO_LOG_ERROR(exc.what());
	}
	return false;
}

