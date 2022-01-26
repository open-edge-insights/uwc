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
std::atomic<bool> sp_shouldStop(false);
vector<std::thread> spThreads;
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

void sub_data_from_eii(std::string eachTopic,zmq_handler::stZmqContext context, zmq_handler::stZmqSubContext subContext){

  	ConfigMgr* sub_ch = NULL;
    recv_ctx_t* g_sub_ctx = NULL;
    void* g_msgbus_ctx = NULL;
    msg_envelope_t* msg = NULL;
	//std::string eachTopic="TCP_PolledData";
    //msgbus_ret_t ret;   
    DO_LOG_INFO("subscriber");
	fprintf(stderr, "GREEN.........2\n\n\n");
    int num_of_subscriber = zmq_handler::getNumPubOrSub("sub");
	fprintf(stderr, "GREEN.........3\n\n\n");
	fprintf(stderr, "GREEN.........4\n\n\n");
	msgbus_ret_t ret;

    fprintf(stderr, "GREEN.........5\n\n\n");
    void *msgbus_ctx = context.m_pContext;
	recv_ctx_t *sub_ctx = subContext.sub_ctx;
    if((msgbus_ctx==NULL) || (sub_ctx==NULL))
    {
       	DO_LOG_ERROR("MSG Bus or Subscriber context is Null");
			//return false;
    }
	fprintf(stderr, "GREEN.........6\n\n\n");
	
	
	fprintf(stderr, "GREEN.........7\n\n\n");
    while ((false == sp_shouldStop.load()) && (msgbus_ctx != NULL) && (sub_ctx != NULL)) {  
	fprintf(stderr, "GREEN.........8\n\n\n");	
	ret = msgbus_recv_wait(msgbus_ctx, sub_ctx, &msg);
	if (ret != MSG_SUCCESS)
	{
       	// Interrupt is an acceptable error
		if (ret == MSG_ERR_EINTR)
		{
	    	DO_LOG_ERROR("received MSG_ERR_EINT");
		}
		DO_LOG_ERROR("Failed to receive message errno: " + std::to_string(ret));

    }
	fprintf(stderr, "GREEN.........9\n\n\n");
	msg_envelope_elem_body_t* data;
	msgbus_ret_t msgRet = msgbus_msg_envelope_get(msg, "data_topic", &data);
	fprintf(stderr, "GREEN.........10\n\n\n");
	if(msgRet != MSG_SUCCESS)
	{ 
	    DO_LOG_ERROR("topic key not present in zmq message");
				//return false;o
    } 
	std::string sRcvdTopic{data->body.string}; 
	msg_envelope_serialized_part_t *parts = NULL;
	fprintf(stderr, "GREEN.........11\n\n\n");
    int num_parts = msgbus_msg_envelope_serialize(msg, &parts);
	fprintf(stderr, "GREEN.........12\n\n\n");
    if(NULL != parts[0].bytes)
	{
		std::string sMsgBody(parts[0].bytes);
        fprintf(stderr, "msg is");
        fprintf(stderr, sMsgBody.c_str());
        fprintf(stderr, "topic is");
		fprintf(stderr, sRcvdTopic.c_str());
		CMessageObject oMsg{sRcvdTopic,sMsgBody};
		QMgr::getDatapointsQ().pushMsg(oMsg);
	

    }else{
		fprintf(stderr, "GREEN.........13\n\n\n");
	}

	}
	fprintf(stderr, "GREEN.........15555\n\n\n");
    
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
            if(zmq_handler::eii_enable()==true){
                fprintf(stderr, "GREEN.........-1\n\n\n");	
				std::vector<std::string> vecTopics;
        		bool tempRet = zmq_handler::returnAllTopics("sub", vecTopics);
				fprintf(stderr, "GREEN.........0\n\n\n");	
				for(std::string eachTopic : vecTopics) {
					fprintf(stderr, "GREEN.........1\n\n\n");	
					fprintf(stderr, "__________________________ADITI______________________");                       
    				fprintf(stderr, eachTopic.c_str());
    				fprintf(stderr, "___________________________________________________");					
					// loop = new std::atomic<bool>;
                    // *loop = true;
					zmq_handler::stZmqContext& context = zmq_handler::getCTX(eachTopic);
					zmq_handler::stZmqSubContext& subContext = zmq_handler::getSubCTX(eachTopic);
					fprintf(stderr, "GREEN.........1a\n\n\n");
					spThreads.push_back(std::thread(sub_data_from_eii,eachTopic,context,subContext));
					//std::thread(sub_data_from_eii,eachTopic).detach();
                    //spThreads.push_back(std::thread{ std::bind(sub_data_from_eii,std::ref(eachTopic)) });
				    //delete loop;
					fprintf(stderr, "GREEN.........14\n\n\n");	
					//sub_data_from_eii(eachTopic);
					for (auto &th : spThreads)
		            {
						fprintf(stderr, "GREEN.........3\n\n\n");	
						if (th.joinable())
						{
							th.join();
						}
					}	

			}}else{
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
					// As a process first subscribe to topics
					subscribeTopics();

					// If disconnect state is monitoring connection timeout, then signal that
					if(true == getConTimeoutState())
					{
						sem_post(&m_semConnSuccessToTimeOut);
					}
				}}
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
 * Publish the message on EII message bus
 * for MQTT-Export
 * @param strPubMsg :[in] payload to be published
 * @param strMsgTopic : [in] Topic on which payload is received
 * @return true/false based on success/failure
 */
bool CIntMqttHandler::publish_msg_to_eii(string strPubMsg,string strMsgTopic)
{
    int num_of_publishers = zmq_handler::getNumPubOrSub("pub");
        if (num_of_publishers >= 1)
        {
                if (true != zmq_handler::prepareCommonContext("pub"))
                {
                        DO_LOG_ERROR("Context creation failed for pub topic ");
			return false;
                }
        }
    msg_envelope_t* g_msg = msgbus_msg_envelope_new(CT_JSON);
    msg_envelope_elem_body_t* topic;
    topic = msgbus_msg_envelope_new_string(strMsgTopic.c_str());
    msgbus_msg_envelope_put(g_msg, strPubMsg.c_str() , topic);
    std::string strTsReceived{""};
    if (true != zmq_handler::publishJson(strTsReceived, g_msg, zmq_handler::getTopics()[0].c_str(),""))
    {
        DO_LOG_INFO("Message Published on EII message bus");
	return false;
    }
    return true;
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
            		                if(zmq_handler::eii_enable()==true){
						DO_LOG_INFO("Publishing on EII msg bus");
						bool publish_status = CIntMqttHandler::publish_msg_to_eii(strPubMsg, strMsgTopic);
						if( publish_status != true ){
							DO_LOG_ERROR("Failed to publish msg on EII");
						}
					}else{
   		    			        DO_LOG_INFO("Publishing on MQTT");
						publishMsg(strPubMsg, strMsgTopic);
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
			cJSON_AddItemToObject(root, "metrics", metricArray);

			string strPubMsg = cJSON_Print(root);

			DO_LOG_DEBUG("Publishing message on internal MQTT for CMD:");
			DO_LOG_DEBUG("Topic : " + strMsgTopic);
			DO_LOG_DEBUG("Payload : " + strPubMsg);
                        if(zmq_handler::eii_enable()==true){
				DO_LOG_INFO("Publishing on EII msg bus");
				fprintf(stderr, "\nPublishing on EII msg bus\n");
				bool publish_status = CIntMqttHandler::publish_msg_to_eii(strPubMsg, strMsgTopic);
				if( publish_status != true ){
					DO_LOG_ERROR("Failed to publish msg on EII");
                                 }

			}else{
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

