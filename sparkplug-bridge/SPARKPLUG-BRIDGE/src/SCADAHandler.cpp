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

#include <thread>
#include "SCADAHandler.hpp"
#include "InternalMQTTSubscriber.hpp"
#include "SparkPlugUDTMgr.hpp"
#include <errno.h>

extern std::atomic<bool> g_shouldStop;

// Declarations used for MQTT
#define SCADASUBSCRIBERID								"SCADA_SUBSCRIBER_"

/**
 * constructor Initializes MQTT m_subscriber
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param iQOS :[in] QOS with which to get messages
 * @return None
 */
CSCADAHandler::CSCADAHandler(const std::string &strMqttURL, int iQOS) :
	CMQTTBaseHandler(strMqttURL, 
	SCADASUBSCRIBERID + CCommon::getInstance().getGroupName() + CCommon::getInstance().getNodeName(),
	iQOS, CCommon::getInstance().isScadaTLS(), 
	"/run/secrets/scadahost_ca_cert", "/run/secrets/scadahost_client_cert", 
	"/run/secrets/scadahost_client_key", "SCADAMQTTListener")
{
	try
	{
		prepareNodeDeathMsg(false);

		init();

		DO_LOG_DEBUG("MQTT initialized successfully");
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(" Exception : " + std::string(e.what()));
	}
}

/**
 * This is a singleton class. Used to handle communication with SCADA master
 * through external MQTT.
 * Initiates number of threads, semaphores for handling connection successful
 * and internal MQTT connection lost scenario.
 * @param None
 * @return true on successful init
 */
bool CSCADAHandler::init()
{
	// Initiate semaphore for requests
	int retVal = sem_init(&m_semSCADAConnSuccess, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for SCADA success connection " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}
	std::thread{ std::bind(&CSCADAHandler::handleSCADAConnectionSuccessThread,
			std::ref(*this)) }.detach();

	retVal = sem_init(&m_semIntMQTTConnLost, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for Internal MQTT connection lost " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}
	std::thread{ std::bind(&CSCADAHandler::handleIntMQTTConnLostThread,
			std::ref(*this)) }.detach();

	// publish dbirth for few devices
	retVal = sem_init(&m_semIntMQTTConnEstablished, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for Internal MQTT connection established " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}
	std::thread{ std::bind(&CSCADAHandler::handleIntMQTTConnEstablishThread,
		std::ref(*this)) }.detach();


	return true;
}

/**
 * Thread function to handle SCADA connection success scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CSCADAHandler::handleSCADAConnectionSuccessThread()
{
	while(false == g_shouldStop.load())
	{
		try
		{
			do
			{
				if((sem_wait(&m_semSCADAConnSuccess)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_shouldStop.load())
				{
					break;
				}

				// As a process first subscribe to topics
				subscribeTopics();

				// Publish the NBIRTH
				publish_node_birth();

				// Publish the DBIRTH for all devices
				publishAllDevBirths(true);

				setInitStatus(true);
			} while(0);

		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("failed to send birth messages :: " + std::string(e.what()));
		}
	}
}

/**
 * Thread function to handle internal MQTT connection lost scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CSCADAHandler::handleIntMQTTConnLostThread()
{
	while(false == g_shouldStop.load())
	{
		try
		{
			do
			{
				if((sem_wait(&m_semIntMQTTConnLost)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_shouldStop.load())
				{
					break;
				}
				DO_LOG_ERROR("INFO: Internal MQTT connection lost. DDEATH to be sent");

				// Publish DDEATH for each device
				auto vDevList = CSparkPlugDevManager::getInstance().getDeviceList();
				for(auto &itrDevice : vDevList)
				{
					// Check if internal MQTT connection is established.
					if(true == CIntMqttHandler::instance().isConnected())
					{
						// Connection is established.
						// No need to send remaining DDEATH messages
						DO_LOG_INFO("Info: Internal MQTT connection is established. No More DDEATH to be sent.");
						break;
					}
					if(false == getInitStatus())
					{
						DO_LOG_INFO("Node init is not done. SparkPlug message publish is not done");
						break;
					}
					DO_LOG_INFO("Sending DDEATH for : " + itrDevice);
					publishMsgDDEATH(itrDevice);
					CSparkPlugDevManager::getInstance().setMsgPublishedStatus(enDEVSTATUS_DOWN, itrDevice);
				}
			} while(0);
		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("ERROR :: " + std::string(e.what()));
		}
	}
}

/**
 * Thread function to handle internal MQTT connection establish scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CSCADAHandler::handleIntMQTTConnEstablishThread()
{
	while(false == g_shouldStop.load())
	{
		try
		{
			do
			{
				if((sem_wait(&m_semIntMQTTConnEstablished)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_shouldStop.load())
				{
					break;
				}
				if(false == getInitStatus())
				{
					DO_LOG_INFO("Node init is not done. SparkPlug DBIRTH message publish is not attempted on internal MQTT connection establishment.");
					break;
				}
				DO_LOG_INFO("INFO: Internal MQTT connection established. DBIRTH to be attempted.");

				// Publish the DBIRTH for all devices
				publishAllDevBirths(false);

			} while(0);
		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("ERROR :: " + std::string(e.what()));
		}
	}
}

/**
 * Signals that internal MQTT connection is lost
 * @return none
 */
void CSCADAHandler::signalIntMQTTConnLostThread()
{
	sem_post(&m_semIntMQTTConnLost);
}

/**
 * Signals that internal MQTT connection is established
 * @return none
 */
void CSCADAHandler::signalIntMQTTConnEstablishThread()
{
	sem_post(&m_semIntMQTTConnEstablished);
}

/**
 * Initializes payload to 0
 * @param a_payload :[in] SparkPlug payload to reset
 * @return none
 */
void CSCADAHandler::defaultPayload(org_eclipse_tahu_protobuf_Payload& a_payload)
{
	memset(&a_payload, 0, sizeof(org_eclipse_tahu_protobuf_Payload));
	a_payload.has_timestamp = true;
	a_payload.timestamp = get_current_timestamp();
	a_payload.has_seq = true;
	a_payload.seq = 0;
}

/**
 * Publish message on MQTT broker for MQTT-Export
 * @param a_ddata_payload :[in] spark plug message to publish
 * @param a_topic :[in] topic on which to publish message
 * @param a_bIsNBirth: [in] tells whether message is NBIRTH
 * @return true/false based on success/failure
 */
bool CSCADAHandler::publishSparkplugMsg(org_eclipse_tahu_protobuf_Payload& a_payload, string a_topic, bool a_bIsNBirth = false)
{
	std::lock_guard<std::mutex> lck(m_mutexSparkPlugMsgPub);
	static uint8_t payload_sequence = 0;
	try
	{
		// Encode the payload into a binary format so it can be published in the MQTT message.
		size_t buffer_length = 0;

		uint8_t next_payload_sequence = payload_sequence + 1;
		if(true == a_bIsNBirth)
		{
			next_payload_sequence = 0;
		}

		// Set sequence number for this payload
		a_payload.has_seq = true;
		a_payload.seq = next_payload_sequence;

		bool encode_passed = pb_get_encoded_size(&buffer_length, org_eclipse_tahu_protobuf_Payload_fields, &a_payload);
		if(encode_passed == false)
		{
			DO_LOG_ERROR("Failed to calculate the payload length");
			return false;
		}

		uint8_t *binary_buffer = (uint8_t *)malloc(buffer_length * sizeof(uint8_t));
		if(binary_buffer == NULL)
		{
			DO_LOG_ERROR("Failed to allocate new memory");
			return false;
		}
		size_t message_length = encode_payload(binary_buffer, buffer_length, &a_payload);
		if(message_length == 0)
		{
			DO_LOG_ERROR("Failed to encode payload");

			if(binary_buffer != NULL)
			{
				free(binary_buffer);
			}
			return false;
		}
		
		// Publish the DDATA on the appropriate topic
		mqtt::message_ptr pubmsg = mqtt::make_message(a_topic, (void*)binary_buffer, message_length, m_QOS, false);

		// SCADA master expects messages to be in order. Publish messages in order. Hence, Wait for completion.
		m_MQTTClient.publishMsg(pubmsg, true);
		payload_sequence = next_payload_sequence;

		// Free the memory
		if(binary_buffer != NULL)
		{
			free(binary_buffer);
		}
		return true;
	}
	catch(std::exception& ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
}

/**
 * Prepare a will message for subscriber. That will
 * act as a death message when this node goes down.
 * @param none
 * @return none
 */
void CSCADAHandler::prepareNodeDeathMsg(bool a_bPublishMsg)
{
	try
	{
		// Create the NDEATH payload
		org_eclipse_tahu_protobuf_Payload ndeath_payload;
		//get_next_payload(&ndeath_payload);
		defaultPayload(ndeath_payload);

		add_simple_metric(&ndeath_payload, "bdSeq", false, 0,
					METRIC_DATA_TYPE_UINT64, false, false, &m_uiBDSeq, sizeof(m_uiBDSeq));

		size_t buffer_length = 0;

		bool encode_passed = pb_get_encoded_size(&buffer_length, org_eclipse_tahu_protobuf_Payload_fields, &ndeath_payload);
		if(encode_passed == false)
		{
			DO_LOG_ERROR("Failed to calculate the payload length");
			return ;
		}
		uint8_t *binary_buffer = (uint8_t *)malloc(buffer_length * sizeof(uint8_t));
		if(binary_buffer == NULL)
		{
			DO_LOG_ERROR("Failed to allocate new memory");
			return;
		}
		size_t message_length = encode_payload(binary_buffer, buffer_length, &ndeath_payload);

		mqtt::will_options willOpts(CCommon::getInstance().getDeathTopic(), (void*)binary_buffer, message_length, m_QOS, false);
		//connect options for async m_subscriber
		m_MQTTClient.setWillMsg(willOpts);
		if(true == a_bPublishMsg)
		{
		// Publish the DDATA on the appropriate topic
		mqtt::message_ptr pubmsg = mqtt::make_message(CCommon::getInstance().getDeathTopic(), (void*)binary_buffer, message_length, m_QOS, false);

			m_MQTTClient.publishMsg(pubmsg, true);
		}

		if(binary_buffer != NULL)
		{
			free(binary_buffer);
		}
		free_payload(&ndeath_payload);
	}
	catch(std::exception& ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * Publish device birth message on SCADA for all devices
 * @param a_deviceName : [in] device for which to publish birth message
 * @return none
 */
void CSCADAHandler::publishAllDevBirths(bool a_bIsNBIRTHProcess)
{
	try
	{
		auto vDevList = CSparkPlugDevManager::getInstance().getDeviceList();
		for(auto &itrDevice : vDevList)
		{
			DO_LOG_DEBUG("Device : " + itrDevice);
			publish_device_birth(itrDevice, a_bIsNBIRTHProcess);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * Publish device birth message on SCADA
 * @param a_deviceName : [in] device for which to publish birth message
 * @param a_bIsNBIRTHProcess: [in] indicates whether DBIRTH is needed as a part of NBIRTH process
 * @return none
 */
void CSCADAHandler::publish_device_birth(string a_deviceName, bool a_bIsNBIRTHProcess)
{
	// Create the DBIRTH payload
	org_eclipse_tahu_protobuf_Payload dbirth_payload;
	//get_next_payload(&dbirth_payload);
	defaultPayload(dbirth_payload);

	try
	{
		if(true == CSparkPlugDevManager::getInstance().prepareDBirthMessage(dbirth_payload, a_deviceName, a_bIsNBIRTHProcess))
		{
			string strDBirthTopic = CCommon::getInstance().getDBirthTopic() + "/" + a_deviceName;

			publishSparkplugMsg(dbirth_payload, strDBirthTopic);
			CSparkPlugDevManager::getInstance().setMsgPublishedStatus(enDEVSTATUS_UP, a_deviceName);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
	// Free the memory
	free_payload(&dbirth_payload);
}

/**
 * Maintain single instance of this class
 * @param None
 * @return Reference of this instance of this class, if successful;
 * 			Application exits in case of failure
 */
CSCADAHandler& CSCADAHandler::instance()
{
	static bool bIsFirst = true;
	static string strMqttUrl = CCommon::getInstance().getExtMqttURL();
	static int nQos = CCommon::getInstance().getMQTTQos();
	if(bIsFirst)
	{
		if(strMqttUrl.empty())
		{
			DO_LOG_ERROR("EXTERNAL_MQTT_URL variable is not set in config file");
			throw std::runtime_error("Missing required config..");
		}
	}
	DO_LOG_DEBUG("External MQTT subscriber is connecting with QOS : " + std::to_string(nQos));
	static CSCADAHandler handler(CCommon::getInstance().getExtMqttURL(), nQos);

	if(bIsFirst)
	{
		handler.connect();
		bIsFirst = false;
	}

	return handler;
}

/**
 * Destructor
 */
CSCADAHandler::~CSCADAHandler()
{
	sem_destroy(&m_semSCADAConnSuccess);
	sem_destroy(&m_semIntMQTTConnLost);
	sem_destroy(&m_semIntMQTTConnEstablished);
}

/**
 * Publish node birth message on SCADA
 * @param none
 * @return none
 */
void CSCADAHandler::publish_node_birth()
{
	org_eclipse_tahu_protobuf_Payload nbirth_payload;
	//get_next_payload(&nbirth_payload);
	defaultPayload(nbirth_payload);
	try
	{
		// Create the NBIRTH payload
		string strAppName = EnvironmentInfo::getInstance().getDataFromEnvMap("AppName");
		if(strAppName.empty())
		{
			DO_LOG_ERROR("App name is empty");
			return;
		}

		nbirth_payload.uuid = (char*) strAppName.c_str();

		// Add getNBirthTopicsome device metrics
		add_simple_metric(&nbirth_payload, "Name", false, 0,
				METRIC_DATA_TYPE_STRING, false, false,
				CCommon::getInstance().getNodeName().c_str(), CCommon::getInstance().getNodeName().length()+1);

		add_simple_metric(&nbirth_payload, "bdSeq", false, 0, METRIC_DATA_TYPE_UINT64, false, false,
				&m_uiBDSeq, sizeof(m_uiBDSeq));
		
		bool bRebirth = false;
		add_simple_metric(&nbirth_payload, "Node Control/Rebirth", false, 0, METRIC_DATA_TYPE_BOOLEAN, false, false,
				&bRebirth, sizeof(bRebirth));
		
		// Add UDT definition
		addModbusTemplateDefToNbirth(nbirth_payload);
		CSparkPlugUDTManager::getInstance().addUDTDefsToNbirth(nbirth_payload);

		DO_LOG_INFO("Publishing nbirth message ...");
		publishSparkplugMsg(nbirth_payload, CCommon::getInstance().getNBirthTopic(), true);

		nbirth_payload.uuid = NULL;
	}
	catch(std::exception& ex)
	{
		DO_LOG_ERROR(ex.what());
	}
	free_payload(&nbirth_payload);
}

/**
 * Subscribe to required topics for SCADA MQTT
 * @param none
 * @return none
 */
void CSCADAHandler::subscribeTopics()
{
	try
	{
		m_MQTTClient.subscribe(CCommon::getInstance().getNCmdTopic());
		m_MQTTClient.subscribe(CCommon::getInstance().getDCmdTopic() + "/+");

		DO_LOG_DEBUG("Subscribed with topics from SCADA master");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a callback function which gets called when subscriber is connected with MQTT broker
 * @param a_sCause :[in] reason for connect
 * @return None
 */
void CSCADAHandler::connected(const std::string &a_sCause)
{
	try
	{
		DO_LOG_INFO("INFO: Connected: " + a_sCause);
		// Publish the NBIRTH and DBIRTH Sparkplug messages
		sem_post(&m_semSCADAConnSuccess);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a callback function which gets called when subscriber is disconnected with MQTT broker
 * @param a_sCause :[in] reason for disconnect
 * @return None
 */
void CSCADAHandler::disconnected(const std::string &a_sCause)
{
	try
	{
		DO_LOG_ERROR("INFO: Disconnected: " + a_sCause);
		++m_uiBDSeq;
		setInitStatus(false);
		prepareNodeDeathMsg(false);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * This is a callback function which gets called when a msg is received
 * @param a_pMsg :[in] pointer to received message
 * @return None
 */
void CSCADAHandler::msgRcvd(mqtt::const_message_ptr a_pMsg)
{
	try
	{
		QMgr::getScadaSubQ().pushMsg(a_pMsg);

		DO_LOG_DEBUG("Pushed MQTT message in queue");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
}

/**
 * Prepare and publish a DDATA message in sparkplug format for a device in a_stRefAction
 * @param a_stRefAction :[in] device and respective data-points which need to be
 * published on External MQTT broker
 * @return true/false based on success/failure
 */
bool CSCADAHandler::publishMsgDDATA(const stRefForSparkPlugAction& a_stRefAction)
{
	//prepare and publish one sparkplug msg for this device
	org_eclipse_tahu_protobuf_Payload sparkplug_payload;
	defaultPayload(sparkplug_payload);
	try
	{
		if(enMSG_DATA != a_stRefAction.m_enAction)
		{
			return false;
		}
		//get this device name to add in topic
		std::string strDeviceName{a_stRefAction.m_refSparkPlugDev.get().getSparkPlugName()};

		if (strDeviceName.size() == 0)
		{
			DO_LOG_ERROR("Device name is blank");
			return false;
		}

		string strMsgTopic = CCommon::getInstance().getDDataTopic() + "/" + strDeviceName;

		if(true == a_stRefAction.m_refSparkPlugDev.get().prepareDdataMsg(sparkplug_payload, a_stRefAction.m_mapChangedMetrics))
		{
			//publish sparkplug message
			publishSparkplugMsg(sparkplug_payload, strMsgTopic);
			a_stRefAction.m_refSparkPlugDev.get().setPublishedStatus(enDEVSTATUS_UP);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	free_payload(&sparkplug_payload);
	return true;
}

/**
 * Prepare and publish a DDEATH message in sparkplug format for a device in a_stRefAction
 * @param a_stRefAction :[in] device and respective data-points which need to be
 * published on External MQTT broker
 * @return true/false based on success/failure
 */
bool CSCADAHandler::publishMsgDDEATH(const stRefForSparkPlugAction& a_stRefAction)
{
	//prepare and publish one sparkplug msg for this device
	org_eclipse_tahu_protobuf_Payload sparkplug_payload;
	//get_next_payload(&sparkplug_payload);
	defaultPayload(sparkplug_payload);
	
	try
	{
		if(enMSG_DEATH != a_stRefAction.m_enAction)
		{
			return false;
		}
		//get this device name to add in topic
		std::string strDeviceName{a_stRefAction.m_refSparkPlugDev.get().getSparkPlugName()};

		if (strDeviceName.size() == 0)
		{
			DO_LOG_ERROR("Device name is blank");
			return false;
		}

		string strMsgTopic = CCommon::getInstance().getDDeathTopic() + "/" + strDeviceName;

		sparkplug_payload.has_timestamp = true;
		sparkplug_payload.timestamp = a_stRefAction.m_refSparkPlugDev.get().getDeathTime();

		//publish sparkplug message
		publishSparkplugMsg(sparkplug_payload, strMsgTopic);
		a_stRefAction.m_refSparkPlugDev.get().setPublishedStatus(enDEVSTATUS_DOWN);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	free_payload(&sparkplug_payload);
	return true;
}

/**
 * Handles process to publish new UDTs
 * @param none
 * @return true/false based on success/failure
 */
bool CSCADAHandler::publishNewUDTs()
{
	try
	{
		if(false == getInitStatus())
		{
			DO_LOG_ERROR("Node init is not done. SparkPlug message publish is not done");
			return false;
		}
		// New UDTs can be published as a part of NBIRTH message.
		// Steps:
		// Initiate NBIRTH process

		// As per discussion - there is no need to publish NDEATH message.
		//prepareNodeDeathMsg(true);
		setInitStatus(false);
		connected("Dummy start");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Prepare a MQTT message with sparkplug format for devices mentioned in a_stRefActionVec
 * @param a_stRefActionVec :[in] devices and respective data-points which need to be
 * published on External MQTT broker
 * @return true/false based on success/failure
 */
bool CSCADAHandler::prepareSparkPlugMsg(std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{
	try
	{
		if(false == getInitStatus())
		{
			DO_LOG_ERROR("Node init is not done. SparkPlug message publish is not done");
			return false;
		}
		//for loop having all the devices for which to publish sparkplug message
		for (auto &itr : a_stRefActionVec)
		{
			//depending on action, call the topic name
			switch (itr.m_enAction)
			{
			case enMSG_BIRTH:
				publish_device_birth(itr.m_refSparkPlugDev.get().getSparkPlugName(), false);
				break;
			case enMSG_DEATH:
				publishMsgDDEATH(itr);
				break;
			case enMSG_DATA:
				publishMsgDDATA(itr);
				break;
			case enMSG_UDTDEF_TO_SCADA:
				publishNewUDTs();
				break;
			default:
				DO_LOG_ERROR("Invalid message type received");
				return false;
			}
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Prepare and publish a DDEATH message in sparkplug format for a device 
 * @param a_sDevName :[in] device name
 * @return true/false based on success/failure
 */
bool CSCADAHandler::publishMsgDDEATH(const std::string &a_sDevName)
{
	//prepare and publish one sparkplug msg for this device
	org_eclipse_tahu_protobuf_Payload sparkplug_payload;
	//get_next_payload(&sparkplug_payload);
	defaultPayload(sparkplug_payload);
	try
	{
		if (a_sDevName.size() == 0)
		{
			DO_LOG_ERROR("Device name is blank");
			return false;
		}

		string strMsgTopic = CCommon::getInstance().getDDeathTopic() + "/" + a_sDevName;

		sparkplug_payload.has_timestamp = true;
		sparkplug_payload.timestamp = get_current_timestamp();

		//publish sparkplug message
		publishSparkplugMsg(sparkplug_payload, strMsgTopic);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	free_payload(&sparkplug_payload);
	return true;
}

/**
 * Process DCMD message received from external MQTT broker
 * @param a_msg :[in] mqtt message to be processed
 * @return none
 */
bool CSCADAHandler::processDCMDMsg(CMessageObject a_msg, std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{
	org_eclipse_tahu_protobuf_Payload dcmd_payload = org_eclipse_tahu_protobuf_Payload_init_zero;
	bool bRet = false;

	try
	{
		int msgLen = a_msg.getMqttMsg()->get_payload().length();

		if(decode_payload(&dcmd_payload, (uint8_t* )a_msg.getMqttMsg()->get_payload().data(), msgLen) >= 0)
		{
			bRet = CSparkPlugDevManager::getInstance().processExternalMQTTMsg(a_msg.getTopic(),
					dcmd_payload, a_stRefActionVec);
		}
		else
		{
			DO_LOG_ERROR("Failed to decode the sparkplug payload");
		}

		bRet = true;
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		bRet = false;
	}
	free_payload(&dcmd_payload);
	return bRet;
}

/**
 * Process NCMD message received from external MQTT broker
 * @param a_msg :[in] mqtt message to be processed
 * @return none
 */
bool CSCADAHandler::processNCMDMsg(CMessageObject a_msg, std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{
	org_eclipse_tahu_protobuf_Payload ncmd_payload = org_eclipse_tahu_protobuf_Payload_init_zero;
	bool bRet = false;

	try
	{
		int msgLen = a_msg.getMqttMsg()->get_payload().length();

		if(decode_payload(&ncmd_payload, (uint8_t* )a_msg.getMqttMsg()->get_payload().data(), msgLen) >= 0)
		{
			for (pb_size_t i = 0; i < ncmd_payload.metrics_count; i++)
			{
				if(NULL == ncmd_payload.metrics[i].name)
				{
					DO_LOG_DEBUG("Metric name is not present in DCMD message. Ignored.");
					return false;
				}

				std::string sName{ncmd_payload.metrics[i].name};
				// Check if metric type is boolean and metric is "Node Control/Rebirth"
				if((METRIC_DATA_TYPE_BOOLEAN == ncmd_payload.metrics[i].datatype) &&
					(sName == "Node Control/Rebirth"))
				{
					auto bVal = (bool)ncmd_payload.metrics[i].value.boolean_value;
					if(bVal)
					{
						// Initiate NDEATH and NBIRTH
						publishNewUDTs();
						bRet = true;
					}
				}
			}
		}
		else
		{
			DO_LOG_ERROR("Failed to decode the sparkplug payload");
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		bRet = false;
	}
	free_payload(&ncmd_payload);
	return bRet;
}

/**
 * Process message received from external MQTT broker
 * @param a_msg :[in] mqtt message to be processed
 * @return none
 */
bool CSCADAHandler::processExtMsg(CMessageObject a_msg, std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{
	bool bRet = false;

	try
	{
		if(a_msg.getTopic() == CCommon::getInstance().getNCmdTopic())
		{
			// This is NCMD message
			bRet = processNCMDMsg(a_msg, a_stRefActionVec);
		}
		else if(std::string::npos != a_msg.getTopic().find(CCommon::getInstance().getDCmdTopic()))
		{			
			// This is DCMD message
			bRet = processDCMDMsg(a_msg, a_stRefActionVec);
		}
		else
		{
			// Unknown message
			DO_LOG_ERROR(a_msg.getTopic() + ": Unknown message received from external MQTT broker");
			bRet = false;
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		bRet = false;
	}
	return bRet;
}

/**
 * Prepares a sparkplug formatted metric for Modbus device
 * @param a_rMetric :[out] sparkplug metric
 * @param a_sName :[in] sparkplug metric name
 * @param a_sValue :[in] sparkplug metric value
 * @param a_bIsBirth :[in] indicates whether it is a birth message
 * @param a_uiPollFreq :[in] poll interval
 * @param a_bIsRealTime :[in] tells whether it is a RT message
 * @return true/false
 */
bool CSCADAHandler::addModbusMetric(org_eclipse_tahu_protobuf_Payload_Metric &a_rMetric, const std::string &a_sName, 
        CValObj &a_oValObj, bool a_bIsBirth, uint32_t a_uiPollInterval, bool a_bIsRealTime, double a_dScale)
{
    try
    {
        a_rMetric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
        uint32_t datatype = a_oValObj.getDataType();
        var_t objVal = a_oValObj.getValue();
        int ret = 0;

        switch(datatype)
        	{
        		case METRIC_DATA_TYPE_BOOLEAN:
        		{	
        			bool scaledValBool = std::get<bool>(objVal);
        			ret = init_metric(&a_rMetric, a_sName.c_str(), false,
        					0, METRIC_DATA_TYPE_BOOLEAN, false, false, (bool *)&scaledValBool, sizeof(bool));
        			if (ret < 0){
        				DO_LOG_ERROR("Init Metric Failure for datatype boolean");
        				return false;
        			}
        			break;
        		}
        		case METRIC_DATA_TYPE_INT16:
        		{
        			int16_t scaledValInt16 = std::get<int16_t>(objVal);
        			ret = init_metric(&a_rMetric, a_sName.c_str(), false,
        					0, METRIC_DATA_TYPE_INT16, false, false, (int16_t *)&scaledValInt16, sizeof(int16_t));
        			if (ret < 0){
        				DO_LOG_ERROR("Init Metric Failure for datatype int16_t");
        				return false;
        			}
        			break;
        		}
        		case METRIC_DATA_TYPE_INT32:
        		{
        			int32_t scaledValInt32 = std::get<int32_t>(objVal);
        			ret = init_metric(&a_rMetric, a_sName.c_str(), false,
        					0, METRIC_DATA_TYPE_INT32, false, false, (int32_t *)&scaledValInt32, sizeof(int32_t));
        			if (ret < 0){
        				DO_LOG_ERROR("Init Metric Failure for datatype int32_t");
        				return false;
        			}

        			break;
        		}

        		case METRIC_DATA_TYPE_INT64:
        		{
        			int64_t scaledValInt64 = std::get<int64_t>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_INT64, false, false, (int64_t *)&scaledValInt64, sizeof(int64_t));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype int64_t");
						return false;
					}
        			break;
        		}

        		case METRIC_DATA_TYPE_UINT16:
        		{
        			uint16_t scaledValUInt16 = std::get<uint16_t>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_UINT16, false, false, (uint16_t *)&scaledValUInt16, sizeof(uint16_t));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype uint16_t");
						return false;
					}
        			break;
        		}

        		case METRIC_DATA_TYPE_UINT32:
        		{
        			uint32_t scaledValUInt32 = std::get<uint32_t>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_UINT32, false, false, (uint32_t *)&scaledValUInt32, sizeof(uint32_t));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype uint32_t");
						return false;
					}

        			break;
        		}

        		case METRIC_DATA_TYPE_UINT64:
        		{
        			uint64_t scaledValUInt64 = std::get<uint64_t>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_UINT64, false, false, (uint64_t *)&scaledValUInt64, sizeof(uint64_t));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype uint64_t");
						return false;
					}

        			break;
        		}

        		case METRIC_DATA_TYPE_FLOAT:
        		{
        			float scaledValFloat = std::get<float>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_FLOAT, false, false, (float *)&scaledValFloat, sizeof(float));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype float");
						return false;
					}

        			break;
        		}
        		case METRIC_DATA_TYPE_DOUBLE:
        		{

        			double scaledValDouble = std::get<double>(objVal);
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_DOUBLE, false, false, (double *)&scaledValDouble, sizeof(double));
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype double");
						return false;
					}
        			break;
        		}

        		case METRIC_DATA_TYPE_STRING:
        		{
        			std::string scaledValStr = std::get<std::string>(objVal);
					if (true == scaledValStr.empty())
					{						
						scaledValStr = "\0";
					}
					ret = init_metric(&a_rMetric, a_sName.c_str(), false,
							0, METRIC_DATA_TYPE_STRING, false, false, scaledValStr.c_str(), scaledValStr.length());
					if (ret < 0){
						DO_LOG_ERROR("Init Metric Failure for datatype double");
						return false;
					}
        			break;
        		}
        		default:

        			DO_LOG_ERROR("Init Metric Failure.");
        			return false;
        	};

       
        
        if(a_bIsBirth)
        {
            org_eclipse_tahu_protobuf_Payload_PropertySet prop = org_eclipse_tahu_protobuf_Payload_PropertySet_init_default;
            add_property_to_set(&prop, "Pollinterval", PROPERTY_DATA_TYPE_UINT32, &a_uiPollInterval, sizeof(a_uiPollInterval));
            add_property_to_set(&prop, "Realtime", PROPERTY_DATA_TYPE_BOOLEAN, &a_bIsRealTime, sizeof(a_bIsRealTime));
            // Added a new property Scale after reading from yml file
            add_property_to_set(&prop, "Scale", METRIC_DATA_TYPE_DOUBLE, &a_dScale, sizeof(a_dScale));
 

            add_propertyset_to_metric(&a_rMetric, &prop);
        }
    }
    catch(std::exception &ex)
    {
        DO_LOG_FATAL(ex.what());
        return false;
    }
    
    return true;
}
/**
 * Prepares a sparkplug formatted metric for Modbus device
 * @param a_rUdt :[out] sparkplug template UDT
 * @param a_sProtocolVal :[in] protocol value
 * @return true/false
 */
bool CSCADAHandler::addModbusPropForBirth(org_eclipse_tahu_protobuf_Payload_Template &a_rUdt, 
		const std::string &a_sProtocolVal)
{
	try
	{
		a_rUdt.parameters_count = 1;
		a_rUdt.parameters = (org_eclipse_tahu_protobuf_Payload_Template_Parameter *) calloc(1, sizeof(org_eclipse_tahu_protobuf_Payload_Template_Parameter));
		if(NULL != a_rUdt.parameters)
		{
			a_rUdt.parameters[0].has_type = true;
			a_rUdt.parameters[0].type = PARAMETER_DATA_TYPE_STRING;
			a_rUdt.parameters[0].which_value = org_eclipse_tahu_protobuf_Payload_Template_Parameter_string_value_tag;
			a_rUdt.parameters[0].name = strdup("Protocol");
			a_rUdt.parameters[0].value.string_value = strndup(a_sProtocolVal.c_str(), a_sProtocolVal.length());
		}
		else
		{
			a_rUdt.parameters_count = 0;
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	
	return true;
}

/**
 * Prepare template definitions to be published on SCADA system
 * @param a_rTahuPayload :[out] reference of spark plug message payload 
 * @return true/false depending on the success/failure
 */
bool CSCADAHandler::addModbusTemplateDefToNbirth(org_eclipse_tahu_protobuf_Payload& a_rTahuPayload)
{
	try
	{
		auto listYML = network_info::getDataPointsYMLList();
		for (auto& itr : listYML)
		{
			org_eclipse_tahu_protobuf_Payload_Template udt_template = org_eclipse_tahu_protobuf_Payload_Template_init_default;
			udt_template.version = strndup(itr.second.getVersion().c_str(), itr.second.getVersion().length());
			udt_template.metrics_count = itr.second.getDataPoints().size();
			udt_template.metrics = (org_eclipse_tahu_protobuf_Payload_Metric *) calloc(itr.second.getDataPoints().size(), sizeof(org_eclipse_tahu_protobuf_Payload_Metric));
			udt_template.template_ref = NULL;
			udt_template.has_is_definition = true;
			udt_template.is_definition = true;
			int iLoop = 0;
			std::string tempStr = "";

			if(udt_template.metrics != NULL)
			{
				for(auto &itrPoint: itr.second.getDataPoints())
				{
					CValObj otempValObj(tempStr);
					if(true != addModbusMetric(udt_template.metrics[iLoop], itrPoint.getID(), otempValObj, true, 0, false, itrPoint.getAddress().m_dScaleFactor))
					{
						DO_LOG_ERROR(itrPoint.getID() + ":Could not add metric to template definition.");
					}
					udt_template.metrics[iLoop].timestamp = get_current_timestamp();
					udt_template.metrics[iLoop].has_timestamp = true;
					++iLoop;
				}
			}

			// YML file name example - iou_datapoints.yml
			std::string sYMLFilename{itr.second.getYMLFileName()};
			{
				std::size_t found = itr.second.getYMLFileName().rfind(".");
				if(found!=std::string::npos)
				{
					sYMLFilename.assign( itr.second.getYMLFileName().substr(0, found) );
				}
			}

			// Add protocl property
			addModbusPropForBirth(udt_template, "");

			// Create the root UDT definition and add the UDT definition value which includes the UDT members and parameters
			org_eclipse_tahu_protobuf_Payload_Metric metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
			init_metric(&metric, sYMLFilename.c_str(), false, 0, METRIC_DATA_TYPE_TEMPLATE, false, false, &udt_template, sizeof(udt_template));
			metric.timestamp = get_current_timestamp();
			metric.has_timestamp = true;

			// Add the UDT to the payload
			add_metric_to_payload(&a_rTahuPayload, &metric);
		}
	}
	catch(std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}
