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

#include "Common.hpp"
#include "ConfigManager.hpp"
#include <iterator>
#include <vector>

#include "SCADAHandler.hpp"
#include "InternalMQTTSubscriber.hpp"
#include "SparkPlugDevMgr.hpp"

#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

vector<std::thread> g_vThreads;

std::atomic<bool> g_shouldStop(false);

#define APP_VERSION "0.0.6.6"

/**
 * Processes a message to be sent on internal MQTT broker
 * @param a_objCDataPointsMgr :[in] reference of data points manager class
 * @param a_qMgr :[in] reference of queue from which message is to be processed
 * @return none
 */
void processExternalMqttMsgs(CQueueHandler& a_qMgr)
{	
	while (false == g_shouldStop.load())
	{
		CMessageObject recvdMsg{};
		if(true == a_qMgr.isMsgArrived(recvdMsg))
		{			
			std::vector<stRefForSparkPlugAction> stRefActionVec;

			CSCADAHandler::instance().processExtMsg(recvdMsg, stRefActionVec);

			//prepare a sparkplug message only if there are values in map
			if(! stRefActionVec.empty())
			{				
				CIntMqttHandler::instance().prepareCJSONMsg(stRefActionVec);
			}
		}
	}
}


/**
 * Processes a message to be sent on external MQTT broker
 * @param a_objCDataPointsMgr :[in] reference of data points manager class
 * @param a_qMgr :[in] reference of queue from which message is to be processed
 * @return none
 */
void processInternalMqttMsgs(CQueueHandler& a_qMgr)
{
	string eiiTopic = "";

	try
	{
		while (false == g_shouldStop.load())
		{
			CMessageObject recvdMsg{};
			if(true == a_qMgr.isMsgArrived(recvdMsg))
			{
				std::vector<stRefForSparkPlugAction> stRefActionVec;
				CSparkPlugDevManager::getInstance().processInternalMQTTMsg(
						recvdMsg.getTopic(),
						recvdMsg.getStrMsg(),
						stRefActionVec);

				//prepare a sparkplug message only if there are values in map
				if(! stRefActionVec.empty())
				{
					CSCADAHandler::instance().prepareSparkPlugMsg(stRefActionVec);
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
	}
}

/**
 * Initialize data points repository by reading data points
 * from yaml file
 * @param none
 * @return true/false depending on success/failure
 */
bool initDataPoints()
{
	try
	{
		string strNetWorkType = EnvironmentInfo::getInstance().getDataFromEnvMap("NETWORK_TYPE");
		string strSiteListFileName = EnvironmentInfo::getInstance().getDataFromEnvMap("DEVICES_GROUP_LIST_FILE_NAME");
		string strAppId = "";

		if(strNetWorkType.empty() || strSiteListFileName.empty())
		{
			DO_LOG_ERROR("Network type or device list file name is not present");
			return false;
		}

		network_info::buildNetworkInfo(strNetWorkType, strSiteListFileName, strAppId);

		// Create SparkPlug devices corresponding to Modbus devices
		CSparkPlugDevManager::getInstance().addRealDevices();
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
	}
	return true;
}

/**
 * Main function of application
 * @param argc :[in] number of input parameters
 * @param argv :[in] input parameters
 * @return 	0/-1 based on success/failure
 */
int main(int argc, char *argv[])
{
	try
	{
		
		CLogger::initLogger(std::getenv("Log4cppPropsFile"));

		DO_LOG_DEBUG("Starting SparkPlug-Bridge ...");
		DO_LOG_INFO("SparkPlug-Bridge container app version is set to :: "+  std::string(APP_VERSION));
		if(!CCommon::getInstance().loadYMLConfig())
		{
			DO_LOG_ERROR("Please set the required config in sparkplug-bridge_config.yml file and restart the container");
		}

		//initialize CCommon class to get common variables
		string AppName = EnvironmentInfo::getInstance().getDataFromEnvMap("AppName");
		if(AppName.empty())
		{
			DO_LOG_ERROR("AppName Environment Variable is not set");
			return -1;
		}

		if(CCommon::getInstance().getExtMqttURL().empty() ||
				EnvironmentInfo::getInstance().getDataFromEnvMap("INTERNAL_MQTT_URL").empty())
		{
			while(true)
			{
				DO_LOG_INFO("Waiting to set EXTERNAL_MQTT_URL/ INTERNAL_MQTT_URL variable in scada_config.yml file...");
				std::this_thread::sleep_for(std::chrono::seconds(300));
			};
		}
		else
		{
				{
					CSparkPlugDevManager::getInstance();

					initDataPoints();

					CSCADAHandler::instance();
					CIntMqttHandler::instance();
				}
		}


#ifdef UNIT_TEST
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
#endif
		g_vThreads.push_back(std::thread(processInternalMqttMsgs, std::ref(QMgr::getDatapointsQ())));
		g_vThreads.push_back(std::thread(processExternalMqttMsgs, std::ref(QMgr::getScadaSubQ())));

		for (auto &th : g_vThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(" Exception : " + std::string(e.what()));
		return -1;
	}
	catch (...)
	{
		DO_LOG_FATAL("Exception : Unknown Exception Occurred. Exiting");
		return -1;
	}

	DO_LOG_WARN("Exiting SparkPlug-Bridge Container");
	return 0;
}
