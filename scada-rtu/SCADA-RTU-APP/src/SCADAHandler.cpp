/************************************************************************************
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation. Title to the
 * Material remains with Intel Corporation.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or otherwise.
 ************************************************************************************/

#include "SCADAHandler.hpp"

/**
 * constructor Initializes MQTT m_subscriber
 * @param strPlBusUrl :[in] MQTT broker URL
 * @param iQOS :[in] QOS with which to get messages
 * @return None
 */
CSCADAHandler::CSCADAHandler(std::string strMqttURL, int iQOS) :
		m_subscriber(strMqttURL, SUBSCRIBERID)
{
	try
	{
		m_QOS = iQOS;

		prepareNodeDeathMsg();

		//connect options for async m_subscriber
		m_subscriberConopts.set_keep_alive_interval(20);
		m_subscriberConopts.set_clean_session(true);
		m_subscriberConopts.set_automatic_reconnect(1, 10);

		m_subscriber.set_callback(m_scadaSubscriberCB);

		connectSubscriber();

		//get values of all the data points and store in data points repository
		initDataPoints();

		// Create SparkPlug devices corresponding to Modbus devices
		CSparkPlugDevManager::getInstance().addRealDevices();

		// Initialize the sequence number for Sparkplug MQTT messages
		reset_sparkplug_sequence();

		DO_LOG_DEBUG("MQTT initialized successfully");
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		std::cout << __func__ << ":" << __LINE__ << " Exception : " << e.what() << std::endl;
	}
}

/**
 * Prepare a will message for subscriber. That will
 * act as a death message when this node goes down.
 * @param none
 * @return none
 */
void CSCADAHandler::prepareNodeDeathMsg()
{
	// Create the NDEATH payload
	org_eclipse_tahu_protobuf_Payload ndeath_payload;
	get_next_payload(&ndeath_payload);

	uint64_t badSeq_value = 0;
	add_simple_metric(&ndeath_payload, "bdSeq", false, 0,
			METRIC_DATA_TYPE_UINT64, false, false, &badSeq_value, sizeof(badSeq_value));

	size_t buffer_length = 1024;
	uint8_t *binary_buffer = (uint8_t *)malloc(buffer_length * sizeof(uint8_t));
	if(binary_buffer == NULL)
	{
		DO_LOG_ERROR("Failed to allocate new memory");
		return;
	}
	size_t message_length = encode_payload(binary_buffer, buffer_length, &ndeath_payload);

	// Publish the DDATA on the appropriate topic
	mqtt::message_ptr pubmsg = mqtt::make_message(CCommon::getInstance().getDeathTopic(), (void*)binary_buffer, message_length, 0, false);

	//connect options for async m_subscriber
	m_subscriberConopts.set_will_message(pubmsg);

	if(binary_buffer != NULL)
	{
		free(binary_buffer);
	}
	free_payload(&ndeath_payload);
}

/**
 * Publish node birth messages to SCADA
 * @param none
 * @return none
 */
void CSCADAHandler::publish_births()
{
	// Publish the NBIRTH
	publish_node_birth();

	//publish vendor app birth message
    vendor_app_birth_request();

	// Publish the DBIRTH
	for(auto &itrDevice : m_deviceDataPoints)
	{
		DO_LOG_DEBUG("Device : " + itrDevice.first);

		publish_device_birth(itrDevice.first, itrDevice.second);
	}
}

/**
 * Prepare device birth messages to be published on SCADA system
 * @param dbirth_payload :[out] reference of spark plug message payload in which to store birth messages
 * @param a_dataPoints :[in] map of datapoints corresponding to the device
 * @return true/false depending on the success/failure
 */
bool CSCADAHandler::prepareDBirthMessage(org_eclipse_tahu_protobuf_Payload& dbirth_payload, std::map<string, stRealDevDataPointRepo>& a_dataPoints, string& a_siteName)
{
	try
	{
		for(auto &dataPoint : a_dataPoints)
		{
			a_siteName = dataPoint.second.m_objUniquePoint.getWellSite().getID();

			string strDeviceName = dataPoint.second.m_objUniquePoint.getDataPoint().getID();

			if(strDeviceName.empty())
			{
				DO_LOG_ERROR("Device name is empty while forming DBIRTH message");
				return false;
			}

			uint64_t current_time = get_current_timestamp();

			org_eclipse_tahu_protobuf_Payload_Metric metric = {NULL, false, 0, true, current_time , true,
					METRIC_DATA_TYPE_STRING, false, 0, false, 0, false, true, false,
					org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
					false, org_eclipse_tahu_protobuf_Payload_PropertySet_init_default, 0, {0}};

			metric.name = (char*)malloc(strDeviceName.size());
			if(metric.name == NULL)
			{
				DO_LOG_ERROR("Failed to allocate new memory");
				return false;
			};
			strDeviceName.copy(metric.name, strDeviceName.size());
			metric.name[strDeviceName.size()] = '\0';
			metric.has_is_null = true;
			metric.is_null = true;

			org_eclipse_tahu_protobuf_Payload_PropertySet prop = org_eclipse_tahu_protobuf_Payload_PropertySet_init_default;

			int iAddr =  dataPoint.second.m_objUniquePoint.getDataPoint().getAddress().m_iAddress;
			add_property_to_set(&prop, "Addr", PROPERTY_DATA_TYPE_INT32, &iAddr, sizeof(iAddr));

			int iWidth = dataPoint.second.m_objUniquePoint.getDataPoint().getAddress().m_iWidth;
			add_property_to_set(&prop, "Width", PROPERTY_DATA_TYPE_INT32, &iWidth, sizeof(iWidth));

			string strDataType = dataPoint.second.m_objUniquePoint.getDataPoint().getAddress().m_sDataType;
			add_property_to_set(&prop, "DataType", PROPERTY_DATA_TYPE_STRING, strDataType.c_str(), strDataType.length());

			eEndPointType endPointType = dataPoint.second.m_objUniquePoint.getDataPoint().getAddress().m_eType;

			string strType = "";
			switch(endPointType)
			{
			case eEndPointType::eCoil:
				strType = "COIL";
				break;
			case eEndPointType::eDiscrete_Input:
				strType = "DISCRETE_INPUT";
				break;
			case eEndPointType::eHolding_Register:
				strType = "HOLDING_REGISTER";
				break;
			case eEndPointType::eInput_Register:
				strType = "INPUT_REGISTER";
				break;
			default:
				DO_LOG_ERROR("Invalid type of data-point in yml file");
				return false;
			}
			add_property_to_set(&prop, "Type", PROPERTY_DATA_TYPE_STRING, strType.c_str(), strType.length());

			uint32_t iPollingInterval = dataPoint.second.m_objUniquePoint.getDataPoint().getPollingConfig().m_uiPollFreq;
			add_property_to_set(&prop, "Pollinterval", PROPERTY_DATA_TYPE_UINT32, &iPollingInterval, sizeof(iPollingInterval));

			bool bVal = dataPoint.second.m_objUniquePoint.getDataPoint().getPollingConfig().m_bIsRealTime;
			add_property_to_set(&prop, "Realtime", PROPERTY_DATA_TYPE_BOOLEAN, &bVal, sizeof(bVal));

			add_propertyset_to_metric(&metric, &prop);

			add_metric_to_payload(&dbirth_payload, &metric);
		}

		add_simple_metric(&dbirth_payload, "Properties/Site info name", false, 0,
				METRIC_DATA_TYPE_STRING, false, false, a_siteName.c_str(), a_siteName.size()+1);
	}
	catch(exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

/**
 * Publish device birth message on SCADA
 * @param a_deviceName : [in] device for which to publish birth message
 * @param a_dataPointInfo : [in] device info
 * @return none
 */
void CSCADAHandler::publish_device_birth(string a_deviceName, std::map<string, stRealDevDataPointRepo>& a_dataPointInfo)
{
	// Create the DBIRTH payload
	org_eclipse_tahu_protobuf_Payload dbirth_payload;
	get_next_payload(&dbirth_payload);

	try
	{
		string strSiteName = "";

		if(a_dataPointInfo.empty())
		{
			DO_LOG_ERROR("No data points are available to publish DBIRTH message");
			return;
		}

		if(true == CSparkPlugDevManager::getInstance().prepareDBirthMessage(dbirth_payload, a_deviceName))
		{
			string strDBirthTopic = CCommon::getInstance().getDBirthTopic() + "/" + a_deviceName;

			CPublisher::instance().publishSparkplugMsg(dbirth_payload, strDBirthTopic);
			CSparkPlugDevManager::getInstance().setMsgPublishedStatus(enDEVSTATUS_UP, a_deviceName);
		}
	}
	catch(exception &ex)
	{
		DO_LOG_FATAL(ex.what());
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
	static string strMqttUrl = CCommon::getInstance().getExtMqttURL();
	static int nQos = CCommon::getInstance().getMQTTQos();

	if(strMqttUrl.empty())
	{
		DO_LOG_ERROR("EXTERNAL_MQTT_URL variable is not set in config file");
		std::cout << __func__ << ":" << __LINE__ << " Error : EXTERNAL_MQTT_URL variable is not set in config file" <<  std::endl;
		throw std::runtime_error("Missing required config..");
	}

	DO_LOG_DEBUG("External MQTT subscriber is connecting with QOS : " + to_string(nQos));
	static CSCADAHandler handler(strMqttUrl.c_str(), nQos);
	return handler;
}

/**
 * Connect m_subscriber with MQTT broker
 * @return true/false based on success/failure
 */
bool CSCADAHandler::connectSubscriber()
{
	bool bFlag = true;
	try
	{
		m_conntok = m_subscriber.connect(m_subscriberConopts, nullptr, m_listener);
		// Wait for 2 seconds to get connected
		if (false == m_conntok->wait_for(2000))
		 {
			 std::cout << "Error::Failed to connect m_subscriber to the platform bus\n";
			 bFlag = false;
		 }

		std::cout << __func__ << ":" << __LINE__ << " SCADA Subscriber connected successfully with MQTT broker" << std::endl;
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		std::cout << __func__ << ":" << __LINE__ << " Exception : " << e.what() << std::endl;
		bFlag = false;
	}

	DO_LOG_DEBUG("SCADA m_subscriber connected successfully with MQTT broker");

	return bFlag;
}

/**
 * Clean up, destroy semaphores, disables callback, disconnect from MQTT broker
 * @param None
 * @return None
 */
void CSCADAHandler::cleanup()
{
	DO_LOG_DEBUG("Destroying CSCADAHandler instance ...");

	m_subscriberConopts.set_automatic_reconnect(0);

	m_subscriber.disable_callbacks();

	if(m_subscriber.is_connected())
		m_subscriber.disconnect();

	DO_LOG_DEBUG("Destroyed CSCADAHandler instance");
}

/**
 * Destructor
 */
CSCADAHandler::~CSCADAHandler()
{
}

/**
 * Publish node birth message on SCADA
 * @param none
 * @return none
 */
void CSCADAHandler::publish_node_birth()
{
	// Create the NBIRTH payload
	string strAppName = CCommon::getInstance().getStrAppName();
	if(strAppName.empty())
	{
		DO_LOG_ERROR("App name is empty");
		return;
	}

	org_eclipse_tahu_protobuf_Payload nbirth_payload;
	get_next_payload(&nbirth_payload);

	nbirth_payload.uuid = (char*) strAppName.c_str();

	// Add getNBirthTopicsome device metrics
	add_simple_metric(&nbirth_payload, "Name", false, 0,
			METRIC_DATA_TYPE_STRING, false, false,
			CCommon::getInstance().getEdgeNodeID().c_str(), CCommon::getInstance().getEdgeNodeID().length()+1);

	std::cout << "Publishing nbirth message ..." << endl;
	CPublisher::instance().publishSparkplugMsg(nbirth_payload, CCommon::getInstance().getNBirthTopic());

	nbirth_payload.uuid = NULL;
	free_payload(&nbirth_payload);
}

/**
 * Populate data points from all the devices from all the sites
 * and store in data points repository
 * @param none
 * @return none
 */
void CSCADAHandler::populateDataPoints()
{
	using network_info::CUniqueDataPoint;

	const std::map<std::string, CUniqueDataPoint> &mapUniquePoint =
			network_info::getUniquePointList();

	for (auto &pt : mapUniquePoint)
	{
		stRealDevDataPointRepo stNewDataPoint(pt.second);

		std::string sUniqueDev(pt.second.getWellSiteDev().getID() + "-" + pt.second.getWellSite().getID());

		 m_deviceDataPoints[sUniqueDev].insert(
				 std::make_pair(stNewDataPoint.m_objUniquePoint.getDataPoint().getID(), stNewDataPoint));
	}
}

/**
 * Initialize data points repository reading data points
 * from yaml file
 * @param none
 * @return true/false depending on success/failure
 */
bool CSCADAHandler::initDataPoints()
{
	try
	{
		string strNetWorkType = CCommon::getInstance().getNetworkType();
		string strSiteListFileName = CCommon::getInstance().getSiteListFileName();

		if(strNetWorkType.empty() || strSiteListFileName.empty())
		{
			DO_LOG_ERROR("Network type or device list file name is not present");
			return false;
		}

		network_info::buildNetworkInfo(strNetWorkType, strSiteListFileName);

		//fill up data points repository
		populateDataPoints();
	}
	catch(exception &ex)
	{
		DO_LOG_FATAL(ex.what());
	}
	return true;
}

/**
 * Subscribe to topics to accept from SCADA master
 */
void CSCADAHandler::subscribeTopics()
{
	string strDCMDMsg = "+/+/DCMD/+/+";
	m_subscriber.subscribe(strDCMDMsg, m_QOS, nullptr, m_listener);

	DO_LOG_DEBUG("Subscribed with topics from SCADA master");
}

/**
 * Callback function to publish node and device birth messages
 */
void CSCADAHandler::connected()
{
	//as per specification, we need to subscribe to the topics first
    subscribeTopics();

    // Publish the NBIRTH and DBIRTH Sparkplug messages
    publish_births();

}

/**
 * Publish message on internal MQTT for vendor app
 * so that it will publish device birth message
 */
void CSCADAHandler::vendor_app_birth_request()
{
    //get birth details from vendor app by publishing messages for them
    //prepare CJSON message to publish on internal MQTT
	string strPubTopic = "START_BIRTH_PROCESS";
	string strBlankMsg = "";
	CPublisher::instance().publishIntMqttMsg(strBlankMsg, strPubTopic);
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
		//for loop having all the devices for which to publish sparkplug message
		for (auto &itr : a_stRefActionVec)
		{
			//prepare and publish one sparkplug msg for this device
			org_eclipse_tahu_protobuf_Payload sparkplug_payload;
			get_next_payload(&sparkplug_payload);

			//get this device name to add in topic
			std::string strDeviceName{itr.m_refSparkPlugDev.get().getSparkPlugName()};

			if (strDeviceName.size() == 0)
			{
				DO_LOG_ERROR("Device name is blank");
				return false;
			}

			string strMsgTopic = "";
			metricMap_t m_metricForMsg;
			eDevStatus enMsgPublishState{enDEVSTATUS_NONE};

			//depending on action, call the topic name
			switch (itr.m_enAction)
			{
			case enMSG_BIRTH:
				strMsgTopic = CCommon::getInstance().getDBirthTopic() + "/";
				if(true == CSparkPlugDevManager::getInstance().prepareDBirthMessage(sparkplug_payload, strDeviceName))
				{
					strMsgTopic.append(strDeviceName);
					CPublisher::instance().publishSparkplugMsg(sparkplug_payload, strMsgTopic);
					itr.m_refSparkPlugDev.get().setPublishedStatus(enDEVSTATUS_UP);
				}
				break;
			case enMSG_DEATH:
				strMsgTopic = CCommon::getInstance().getDDeathTopic() + "/";
				enMsgPublishState = enDEVSTATUS_DOWN;
				break;
			case enMSG_DATA:
				strMsgTopic = CCommon::getInstance().getDDataTopic() + "/";
				m_metricForMsg = std::ref(itr.m_mapChangedMetrics);
				enMsgPublishState = enDEVSTATUS_UP;
				break;
			default:
				DO_LOG_ERROR("Invalid message type received from internal MQTT broker");
				return false;
			}

			strMsgTopic.append(strDeviceName);

			if (itr.m_enAction == enMSG_DEATH)
			{
				sparkplug_payload.has_timestamp = true;
				sparkplug_payload.timestamp = itr.m_refSparkPlugDev.get().getDeathTime();
			}
			else if (itr.m_enAction != enMSG_BIRTH)
			{
			//these shall be part of a single sparkplug msg
			for (auto &itrMetric : m_metricForMsg)
			{
				uint64_t timestamp = itrMetric.second.getTimestamp();
				string strMetricName = itrMetric.second.getName();

					org_eclipse_tahu_protobuf_Payload_Metric metric =
							{ NULL, false, 0, true, timestamp, true,
									itrMetric.second.getValue().getDataType(), false, 0, false, 0, false,
									true, false,
						org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
									false,
											org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
									0,
									{ 0 } };

					metric.name = (char*) malloc(strMetricName.size());
				if(metric.name == NULL)
				{
					DO_LOG_ERROR("Failed to allocate new memory");
					return false;
				};
					strMetricName.copy(metric.name, strMetricName.size());
					metric.name[strMetricName.size()] = '\0';

				itrMetric.second.getValue().assignToSparkPlug(metric);

				add_metric_to_payload(&sparkplug_payload, &metric);

			}//metric ends
			}
			if (itr.m_enAction != enMSG_BIRTH)
			{
				//publish sparkplug message
				CPublisher::instance().publishSparkplugMsg(sparkplug_payload,
					strMsgTopic);

				itr.m_refSparkPlugDev.get().setPublishedStatus(enMsgPublishState);
			}

			free_payload(&sparkplug_payload);
		}
	}
	catch(exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		return false;
	}
	return true;
}

/**
 * Checks if external MQTT subscriber has been connected with the  MQTT broker
 * @param none
 * @return true/false as per the connection status of the external MQTT subscriber
 */
bool CSCADAHandler::isExtMqttSubConnected()
{
	return m_subscriber.is_connected();
}

/*
 * Helper function to disconnect MQTT client from External MQTT broker
 */
void CSCADAHandler::disconnect()
{
	cout << "Disconnecting from External MQTT ... " << endl;
	m_subscriber.disconnect();
}

/*
 * Helper function to connect MQTT client from External MQTT broker
 */
void CSCADAHandler::connect()
{
	cout << "Connecting to External MQTT ... " << endl;
	m_subscriber.connect();
}


/**
 * Process message received from external MQTT broker
 * @param a_msg :[in] mqtt message to be processed
 * @return none
 */
bool CSCADAHandler::processDCMDMsg(mqtt::const_message_ptr a_msg, std::vector<stRefForSparkPlugAction>& a_stRefActionVec)
{

	org_eclipse_tahu_protobuf_Payload dcmd_payload = org_eclipse_tahu_protobuf_Payload_init_zero;
	bool bRet = false;

	try
	{
		int msgLen = a_msg->get_payload().length();

		if(decode_payload(&dcmd_payload, (uint8_t* )a_msg->get_payload().data(), msgLen) >= 0)
		{
			bRet = CSparkPlugDevManager::getInstance().processExternalMQTTMsg(a_msg->get_topic(),
					dcmd_payload, a_stRefActionVec);
		}
		else
		{
			DO_LOG_ERROR("Failed to decode the sparkplug payload");
		}

		free_payload(&dcmd_payload);
		return bRet;
	}
	catch(exception &ex)
	{
		DO_LOG_FATAL(ex.what());
		free_payload(&dcmd_payload);
		return false;
	}
	return true;
}

/**
 * Push message in message queue to send on EIS
 * @param msg :[in] reference of message to push in queue
 * @return true/false based on success/failure
 */
bool CSCADAHandler::pushMsgInQ(mqtt::const_message_ptr msg)
{
	bool bRet = true;
	try
	{
		QMgr::getScadaSubQ().pushMsg(msg);

		DO_LOG_DEBUG("Pushed MQTT message in queue");
		bRet = true;
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		bRet = false;
	}
	return bRet;
}
