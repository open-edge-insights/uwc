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

#include "InternalMQTTSubscriber.hpp"
#include "Common.hpp"
#include "ConfigManager.hpp"
#include "SparkPlugDevices.hpp"
#include "SCADAHandler.hpp"
#include <chrono>
#include <ctime>
#include <errno.h>
#define SUBSCRIBER_ID "SCADA_INT_MQTT_SUBSCRIBER"
#define RECONN_TIMEOUT_SEC (60)

extern std::atomic<bool> g_shouldStop;

std::atomic<bool> *loop;
/**
 * Constructor Initializes MQTT publisher
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param strClientID :[in] client ID with which to subscribe (this is topic name)
 * @param iQOS :[in] QOS value with which publisher will publish messages
 * @return None
 */
CIntMqttHandler::CIntMqttHandler(const std::string &strPlBusUrl, int iQOS):
	CMQTTBaseHandler(strPlBusUrl, SUBSCRIBER_ID, iQOS, (false == CcommonEnvManager::Instance().getDevMode()),
	"/run/secrets/rootca/cacert.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem",
	"/run/secrets/mymqttcerts/mymqttcerts_client_key.pem", "InternalMQTTListener"),
	m_enLastConStatus{enCON_NONE}, m_bIsInTimeoutState{false}
{
	try
	{
		m_appSeqNo = 0;

		DO_LOG_DEBUG("MQTT initialized successfully");
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Maintain single instance of this class
 * @param None
 * @return Reference of this instance of this class, if successful;
 * 			Application exits in case of failure
 */
CIntMqttHandler& CIntMqttHandler::instance()
{
	static bool bIsFirst = true;
	static string strPlBusUrl = EnvironmentInfo::getInstance().getDataFromEnvMap("INTERNAL_MQTT_URL");
	static int nQos = CCommon::getInstance().getMQTTQos();

	if(bIsFirst)
	{
		if(strPlBusUrl.empty())
		{
			DO_LOG_ERROR("Error :: MQTT_URL Environment variable is not set");
			throw std::runtime_error("Missing required config..");
		}
	}

	DO_LOG_DEBUG("Internal MQTT subscriber is connecting with QOS : " + std::to_string(nQos));
	static CIntMqttHandler handler(strPlBusUrl.c_str(), nQos);

	if(bIsFirst)
	{
		handler.init();
		handler.connect();
		bIsFirst = false;
	}
	return handler;
}

/**
 * Subscribe to required topics for SCADA MQTT
 * @param none
 * @return none
 */
void CIntMqttHandler::subscribeTopics()
{
	try
	{
		m_MQTTClient.subscribe("BIRTH/#");
		m_MQTTClient.subscribe("DATA/#");
		m_MQTTClient.subscribe("DEATH/#");
		m_MQTTClient.subscribe("/+/+/+/update");
		m_MQTTClient.subscribe("TemplateDef");

		DO_LOG_DEBUG("Subscribed topics with internal broker");
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
void CIntMqttHandler::connected(const std::string &a_sCause)
{
	try
	{
		sem_post(&m_semConnSuccess);
		setLastConStatus(enCON_UP);
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
void CIntMqttHandler::disconnected(const std::string &a_sCause)
{
	try
	{
		if(enCON_DOWN != getLastConStatus())
		{
			sem_post(&m_semConnLost);
		}
		else
		{
			// No action. Connection is not yet established.
			// This callback is being executed from reconnect
		}
		setLastConStatus(enCON_DOWN);
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
void CIntMqttHandler::msgRcvd(mqtt::const_message_ptr a_pMsg)
{
	try
	{
		// Push message in message queue for further processing
		CMessageObject oMsg{a_pMsg};
		QMgr::getDatapointsQ().pushMsg(oMsg);

		DO_LOG_DEBUG("Pushed MQTT message in queue");
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
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
bool CIntMqttHandler::init()
{
	// Initiate semaphore for requests
	int retVal = sem_init(&m_semConnSuccess, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for success connection. Errno: " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}

	retVal = sem_init(&m_semConnLost, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for lost connection  " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}

	retVal = sem_init(&m_semConnSuccessToTimeOut, 0, 0 /* Initial value of zero*/);
	if (retVal == -1)
	{
		DO_LOG_FATAL("Could not create unnamed semaphore for monitorig connection timeout " + std::to_string(errno) + " " + strerror(errno));
		return false;
	}

    std::thread{ std::bind(&CIntMqttHandler::handleConnMonitoringThread,
		std::ref(*this)) }.detach();
	std::thread{ std::bind(&CIntMqttHandler::handleConnSuccessThread,
		std::ref(*this)) }.detach();		
	

	return true;
}

/**
 * Thread function to handle SCADA connection success scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CIntMqttHandler::handleConnMonitoringThread()
{
	while(false == g_shouldStop.load())
	{
		try
		{
			do
			{
				if((sem_wait(&m_semConnLost)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_shouldStop.load())
				{
					break;
				}

				// Connection is lost
				// Now check for 1 min to see if connection is established
				struct timespec ts;
				int rc = clock_gettime(CLOCK_REALTIME, &ts);
				if(0 != rc)
				{
					DO_LOG_FATAL("Fatal error: clock_gettime failed: " + std::to_string(errno) + " " + strerror(errno));
					break;
				}
				// Wait for timeout seconds to declare connection timeout
				ts.tv_sec += RECONN_TIMEOUT_SEC;

				setConTimeoutState(true);
				while ((rc = sem_timedwait(&m_semConnSuccessToTimeOut, &ts)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				setConTimeoutState(false);

				if(true == m_MQTTClient.isConnected())
				{
					// Client is connected. So wait for connection-lost
					break;
				}

				// Check status
				if(-1 == rc)
				{
					if(ETIMEDOUT == errno)
					{
						DO_LOG_ERROR("Connection lost for timeout period. Informing SCADA");
						// timeout occurred and connection is not yet established
						// Inform SCADA handler
						CSCADAHandler::instance().signalIntMQTTConnLostThread();
					}
					else
					{
						// No action for now
					}
				}
			} while(0);
		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("failed to initiate request :: " + std::string(e.what()));
		}
	}
}

/**
 * Thread function to handle SCADA connection success scenario.
 * It listens on a semaphore to know the connection status.
 * @return none
 */
void CIntMqttHandler::handleConnSuccessThread()
{
	while(false == g_shouldStop.load())
	{

			
	try
		{
			do
			{
				if((sem_wait(&m_semConnSuccess)) == -1 && errno == EINTR)
				{
					// Continue if interrupted by handler
					continue;
				}
				if(true == g_shouldStop.load())
				{
					break;
				}

				if(true == m_MQTTClient.isConnected())
				{
					// Client is connected. So wait for connection-lost
					// Semaphore got signalled means connection is successful
					// Subscriber for MQTT topics
					if(zmq_handler::enable_EMB()==false){
						subscribeTopics();
					}
					// If disconnect state is monitoring connection timeout, then signal that
					if(true == getConTimeoutState())
					{
						sem_post(&m_semConnSuccessToTimeOut);
					}
				}
					//Send dbirth
					CSCADAHandler::instance().signalIntMQTTConnEstablishThread();

					// Subscription is done. Publish START_BIRTH_PROCESS if not done yet
					static bool m_bIsFirstConnectDone = true;
					if(true == m_bIsFirstConnectDone)
					{
						publishMsg("", "START_BIRTH_PROCESS");
						DO_LOG_INFO("START_BIRTH_PROCESS message is published");
						m_bIsFirstConnectDone = false;
					}
				
			} while(0);

		}
		catch (std::exception &e)
		{
			DO_LOG_ERROR("failed to initiate request :: " + std::string(e.what()));
		}
	}
}

/**
 * Destructor
 */
CIntMqttHandler::~CIntMqttHandler()
{
	sem_destroy(&m_semConnSuccess);
	sem_destroy(&m_semConnLost);
	sem_destroy(&m_semConnSuccessToTimeOut);
}

/**
 * Get app sequence no needed for write-on-demand
 * @return app sequence no in int format
 */
int CIntMqttHandler::getAppSeqNo()
{
	m_appSeqNo++;
	if(m_appSeqNo > 65535)
	{
		m_appSeqNo = 0;
	}

	return m_appSeqNo;
}

/**
* map MQTT topic to EMB topic
* @param mqttTopic :[in] Mqtt topic
* @return embtopic
*/
std::string CIntMqttHandler::mapMqttToEMBRespTopic(std::string mqttTopic){

	std::string delimeter = "/write";
	int size = mqttTopic.find(delimeter);
	std::string embTopic = mqttTopic.substr(0,size);
	std::string check_value = zmq_handler::get_RT_NRT(embTopic);
	if( check_value == "0"){
		check_value = "NRT";
	}else if (check_value == "1"){
		check_value = "RT";
	}else if (check_value.empty()){
		check_value = zmq_handler::get_RT_NRT("default");
		check_value = (check_value == "1")?"RT":"NRT";
	}
	embTopic = check_value+"/write"+ embTopic;
	return embTopic;
}
/**
* To publish msg on EMB for both VA and Real Device
* @param embMsg :[in] Msg payload
* @param mqttTopic: MQTT topic 
* @return true/false based on success/failure
*/
// /flowmeter/PL0/D13/read to RT|NRT/read/flowmeter/PL0/D13
bool CIntMqttHandler::publish_msg_to_emb(std::string embMsg,std::string mqttTopic)
{
	bool retVal = false;
	std::string delimeter = "/";
	int size = mqttTopic.find(delimeter);
	// To check if topic is of VA or Real Device
	std::string VA_check = mqttTopic.substr(0,size);
	std::string embTopic;	
	msg_envelope_elem_body_t* obj = NULL;
	msg_envelope_t *msg = NULL;
	cJSON *root = NULL;
	// In case of Vendor App
	if(VA_check == "CMD"){
		embTopic = mqttTopic;
		// removing additional square braces from payload
		delimeter = "]";
		int length = embMsg.size();
		size = embMsg.find(delimeter);
		embMsg = embMsg.substr(0,size);
		embMsg.replace(0,1,""); 
		obj = msgbus_msg_envelope_new_object();
	}else{// In case of Real Device
		embTopic = CIntMqttHandler::mapMqttToEMBRespTopic(mqttTopic);
	}
	DO_LOG_DEBUG("Topic for publishing is"+ embTopic);
	zmq_handler::prepareContext(true, (zmq_handler::getPubCtxCfg()).m_pub_msgbus_ctx, embTopic, (zmq_handler::getPubCtxCfg()).m_pub_config);
	try
	{
		msg = msgbus_msg_envelope_new(CT_JSON);
		if(msg == NULL)
		{
			DO_LOG_ERROR("could not create new msg envelope");
			return retVal;
		}
		root = cJSON_Parse(embMsg.c_str());
		if (NULL == root)
		{
			DO_LOG_ERROR("Could not parse value received from MQTT");

			if(msg != NULL)
			{
				msgbus_msg_envelope_destroy(msg);
			}

			return retVal;
		}
		// In case of Real Device
		auto addField = [&msg](const std::string &a_sFieldName, const std::string &a_sValue) {
			DO_LOG_DEBUG(a_sFieldName + " : " + a_sValue);
			msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_string(a_sValue.c_str());
			if((NULL != value) && (NULL != msg))
			{
				msgbus_msg_envelope_put(msg, a_sFieldName.c_str(), value);
			}
		};
		// In case of Vendor App
		auto addField_VA = [&msg](const std::string &a_sFieldName, const std::string &a_sValue,std::string VA_check,msg_envelope_elem_body_t* obj) {
			DO_LOG_DEBUG(a_sFieldName + " : " + a_sValue);
			msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_string(a_sValue.c_str());
			if((NULL != value) && (NULL != msg))
			{

				msgbus_msg_envelope_elem_object_put(obj, a_sFieldName.c_str() , value);	
			}
		};		
		
		cJSON *device = root->child;
		while (device)
		{
			if(cJSON_IsString(device))
			{        
				
				if(VA_check=="CMD"){
					// In case of Vendor App
					addField_VA(device->string, device->valuestring,VA_check,obj);

				}else{
					// In case of Real Device
					addField(device->string, device->valuestring);
				}
			}
			else if (cJSON_IsBool(device))
			{
				bool val = false;
				// Add bool in msg envelope
				if (cJSON_IsTrue(device))
				{
					val = true;
				}
				msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_bool(val);
				if (NULL != msg && NULL != value)
				{
					if(VA_check=="CMD"){
						// In case of Vendor App
						msgbus_msg_envelope_elem_object_put(obj, device->string , value);

					}else{
						// In case of Real Device
						msgbus_msg_envelope_put(msg, device->string, value);
					}					
				}
			}
			else if (cJSON_IsNumber(device))
			{
				// Add number in msg envelope
				msg_envelope_elem_body_t *value = msgbus_msg_envelope_new_floating(device->valuedouble);
				if (NULL != msg && NULL != value)
				{
					
					if(VA_check=="CMD"){
						// In case of Vendor App
						msgbus_msg_envelope_elem_object_put(obj, device->string , value);

					}else{
						// In case of Real Device
						msgbus_msg_envelope_put(msg, device->string, value);
					}					
				}
			}
			else
			{
				throw std::string("Invalid JSON");
			}
			// get and print key
			device = device->next;
		}

		if (root)
		{
			cJSON_Delete(root);
		}
		root = NULL;
		std::string time_stamp_value = CCommon::getInstance().get_timestamp();
		if(VA_check=="CMD"){
			// In case of Vendor App
			addField_VA("tsMsgRcvdFromExtMQTTToSP", time_stamp_value.c_str(),VA_check,obj);
			addField_VA("sourcetopic", mqttTopic,VA_check,obj);
		}else{
			// In case of Real Device
			addField("tsMsgRcvdFromExtMQTTToSP", time_stamp_value.c_str());
			addField("sourcetopic", mqttTopic);

		}
		std::string strTsReceived{""};
		bool bRet = true;
		// In case of Real Device
		if(VA_check=="CMD"){
			auto p1 = std::chrono::system_clock::now();
			unsigned long uTime = (unsigned long)(std::chrono::duration_cast<std::chrono::microseconds>(p1.time_since_epoch()).count());
			std::string a_sUsec = std::to_string(uTime);
			msg_envelope_elem_body_t* ptUsec = msgbus_msg_envelope_new_string(a_sUsec.c_str());
			msgbus_msg_envelope_elem_object_put(obj, "tsMsgPublishSPtoEMB" , ptUsec);
			msgbus_msg_envelope_put(msg, "metrics", obj);
			
			
			if(true == zmq_handler::publishJson(strTsReceived, msg, embTopic, ""))
			{
				bRet = true;
			}
			else
			{
				DO_LOG_ERROR("Failed to publish write msg on EMB: " + embMsg);
				bRet = false;
			}

		}else{
		// In case of Real Device
		if(true == zmq_handler::publishJson(strTsReceived, msg, embTopic, "tsMsgPublishSPtoEMB"))
		{
			bRet = true;
		}
		else
		{
			DO_LOG_ERROR("Failed to publish write msg on EMB: " + embMsg);
			bRet = false;
		}
		}
		msgbus_msg_envelope_destroy(msg);
		msg = NULL;

		return bRet;
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}

	if(msg != NULL)
	{
		msgbus_msg_envelope_destroy(msg);
	}
	if (root)
	{
		cJSON_Delete(root);
	}

	return false;
}

/**
 * Prepare a message in CJSON format to be sent on Inetnal MQTT
 * for MQTT-Export
 * @param m_refSparkPlugDev :[in] ref of sparkplugDev class
 * @param m_mapChangedMetrics : [in] ref of a map containing metrics with values
 * for which to prepare the request
 * @return true/false based on success/failure
 */
bool CIntMqttHandler::prepareWriteMsg(std::reference_wrapper<CSparkPlugDev>& a_refSparkPlugDev,
		metricMapIf_t& a_mapChangedMetrics)
{
	cJSON *root = NULL;
	string strMsgTopic = "";

	try
	{
		//list of changed metrics for which to send CMD or write-on-demand CJSON request
		metricMapIf_t m_metrics = a_mapChangedMetrics;


		for(auto& metric : m_metrics)
		{
			root = cJSON_CreateObject();
			if (root == NULL)
			{
				DO_LOG_ERROR("Creation of CJSON object failed");
				return false;
			}

			if(false == a_refSparkPlugDev.get().getWriteMsg(strMsgTopic, root, metric, getAppSeqNo()))
			{
				DO_LOG_ERROR("Failed to prepare CJSON message for internal MQTT");
			}
			else
			{
				if((root != NULL) && (! strMsgTopic.empty()))
				{
					string strPubMsg = cJSON_Print(root);
					//Publisher for EMB 
            				if(zmq_handler::enable_EMB()==true){
						DO_LOG_INFO("Publishing on EMB msg bus");
						bool publish_status = CIntMqttHandler::publish_msg_to_emb(strPubMsg, strMsgTopic);
						if( publish_status != true ){
							DO_LOG_ERROR("Failed to publish msg on EMB");
						}
					}else{
   		    			        DO_LOG_INFO("Publishing on MQTT");
						publishMsg(strPubMsg, strMsgTopic); //Publisher for MQTT 
					}					
					
				}
			}
			if (root != NULL)
			{
				cJSON_Delete(root);
				root = NULL;
			}
		}

		if (root != NULL)
		{
			cJSON_Delete(root);
			root = NULL;
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		if (root != NULL)
		{
			cJSON_Delete(root);
			root = NULL;
		}
		return false;
	}
	return true;
}

/**
 * Prepare a message in CJSON format to be sent on Inetnal MQTT
 * for vendor app
 * @param m_refSparkPlugDev :[in] ref of sparkplugDev class
 * @param m_mapChangedMetrics : [in] ref of a map containing metrics with values
 * for which to prepare the request
 * @return true/false based on success/failure
 */
bool CIntMqttHandler::prepareCMDMsg(std::reference_wrapper<CSparkPlugDev>& a_refSparkPlugDev,
		metricMapIf_t& a_mapChangedMetrics)
{
	cJSON *root = NULL, *metricArray = NULL;
	string strMsgTopic = "";
	string strPubMsg;
	try
	{
		root = cJSON_CreateObject();
		if (root == NULL)
		{
			DO_LOG_ERROR("Creation of CJSON object failed");
			return false;
		}

		metricArray = cJSON_CreateArray();
		if (metricArray == NULL)
		{
			DO_LOG_ERROR("Creation of CJSON array failed");
			if (root != NULL)
			{
				cJSON_Delete(root);
				root = NULL;
			}
			return false;
		}
		if (false == a_refSparkPlugDev.get().getCMDMsg(strMsgTopic, a_mapChangedMetrics, metricArray))
		{
			DO_LOG_ERROR("Failed to prepare CJSON message for internal MQTT");
		}
		else
		{
			// Adding metrics to payload in case of MQTT 
			if(zmq_handler::enable_EMB()==false){
				cJSON_AddItemToObject(root, "metrics", metricArray);
				strPubMsg = cJSON_Print(root);
			}else{
				strPubMsg = cJSON_Print(metricArray);
			}
			DO_LOG_DEBUG("Publishing message on internal MQTT for CMD:");
			DO_LOG_DEBUG("Topic : " + strMsgTopic);
			DO_LOG_DEBUG("Payload : " + strPubMsg);
			// Publisher for EMB 
           		if(zmq_handler::enable_EMB()==true){
				DO_LOG_INFO("Publishing on EMB msg bus");
				fprintf(stderr, "\nPublishing on EMB msg bus\n");
				bool publish_status = CIntMqttHandler::publish_msg_to_emb(strPubMsg, strMsgTopic);
				if( publish_status != true ){
					DO_LOG_ERROR("Failed to publish msg on EMB");
                   		 }
			}else{
				// Publisher for MQTT
	   		    	DO_LOG_INFO("Publishing on MQTT");
				fprintf(stderr, "\nPublishing on MQTT\n");
				publishMsg(strPubMsg, strMsgTopic);
			}
		}

		if (root != NULL)
		{
			cJSON_Delete(root);
			root = NULL;
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		if (root != NULL)
		{
			cJSON_Delete(root);
			root = NULL;
		}
		return false;
	}
	return true;
}

/**
 * Prepare a message in CJSON format to be sent on Inetnal MQTT
 * for vendor app and MQTT-Export
 * @param a_stRefActionVec :[in] vector of structure containing metrics with values
 * for which to prepare the request
 * @return true/false based on success/failure
 */
bool CIntMqttHandler::prepareCJSONMsg(std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{
	try
	{
		//there should be only one device while forming CMD message from DCMD msg
		for (auto &itr : a_stRefActionVec)
		{
			//get this device name to add in topic
			string strDeviceName = "";
			strDeviceName.append(itr.m_refSparkPlugDev.get().getSparkPlugName());
			if (strDeviceName.size() == 0)
			{
				DO_LOG_ERROR("Device name is blank");
				return false;
			}
			//parse the site name from the topic
			vector<string> vParsedTopic = { };
			CSparkPlugDevManager::getInstance().getTopicParts(strDeviceName, vParsedTopic, "-");
			if (vParsedTopic.size() != 2)
			{
				DO_LOG_ERROR("Invalid device name found while preparing request for internal MQTT");
				return false;
			}
			//list of changed metrics for which to send CMD or write-on-demand CJSON request
			if(itr.m_refSparkPlugDev.get().isVendorApp())
			{
				prepareCMDMsg(itr.m_refSparkPlugDev, itr.m_mapChangedMetrics);
			}
			else
			{
				prepareWriteMsg(itr.m_refSparkPlugDev, itr.m_mapChangedMetrics);
			}
		}//action structure ends
	}
	catch (std::exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

