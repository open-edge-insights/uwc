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

#include "PeriodicReadFeature.hpp"
#include "NetworkInfo.hpp"
#include "ZmqHandler.hpp"
#include "PublishJson.hpp"
#include "PeriodicRead.hpp"
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ModbusOnDemandHandler.hpp"
#include "YamlUtil.hpp"
#include "ConfigManager.hpp"
#include "Logger.hpp"
#include "CommonDataShare.hpp"
#include "ZmqHandler.hpp"


#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif

extern "C" {
#include <safe_lib.h>
}

std::mutex mtx;
std::condition_variable cv;
bool g_stop = false;

#define APP_VERSION "0.0.6.6"
#define TIMER_TICK_FREQ 1000 // in microseconds

/// flag to stop all running threads
extern std::atomic<bool> g_stopThread;

std::vector<string> m_vecEnv{"DEVICES_GROUP_LIST_FILE_NAME",
	"DEV_MODE",
	"NETWORK_TYPE",
	"AppName",
	"MY_APP_ID"
};

using namespace zmq_handler;

/**
 * Populate polling data
 */
void populatePollingRefData()
{
	DO_LOG_DEBUG("Start");

	using network_info::CUniqueDataPoint;
	using network_info::eEndPointType;
	// 1. get unique point list
	// 2. check if polling is enabled for that point
	// 3. check if zmqcontext is available
	// 4. if 2 and 3 are yes, create polling ref data

	const std::map<std::string, CUniqueDataPoint> &mapUniquePoint = network_info::getUniquePointList();
	int iCount = 1;
	for(auto &pt: mapUniquePoint)
	{
		const CUniqueDataPoint &a = mapUniquePoint.at(pt.first);
		if(0 == a.getDataPoint().getPollingConfig().m_uiPollFreq)
		{
			DO_LOG_INFO("Polling is not set for "+ a.getDataPoint().getID());
			// Polling frequency is not set
			continue; // go to next point
		}
		try
		{

			uint8_t uiFuncCode = 0;
			switch(a.getDataPoint().getAddress().m_eType)
			{
			case network_info::eEndPointType::eCoil:
				uiFuncCode = 1;
				break;
			case network_info::eEndPointType::eDiscrete_Input:
				uiFuncCode = 2;
				break;
			case network_info::eEndPointType::eHolding_Register:
				uiFuncCode = 3;
				break;
			case network_info::eEndPointType::eInput_Register:
				uiFuncCode = 4;
				break;
			}

			CRefDataForPolling objRefPolling{a, uiFuncCode};

			CTimeMapper::instance().insert(a.getDataPoint().getPollingConfig().m_uiPollFreq, objRefPolling);

			DO_LOG_INFO("Polling is set for " +
					a.getDataPoint().getID() +
					", FunctionCode " +
					std::to_string((unsigned)uiFuncCode) +
					", frequency " +
					std::to_string(a.getDataPoint().getPollingConfig().m_uiPollFreq) +
					", RT " +
					std::to_string(a.getDataPoint().getPollingConfig().m_bIsRealTime));
		}
		catch(std::exception &e)
		{
			DO_LOG_FATAL("Exception " +
					(string)e.what() +
					"in processing " +
					a.getDataPoint().getID());
		}
	}

	DO_LOG_DEBUG("End");
}

/**
 * Stops main thread execution
 * @return 	true : on success,
 * 			false : on error
 */
bool exitMainThread(){return g_stopThread;};

/**
 *
 * DESCRIPTION
 * This function is used to check keys from given JSON
 * @param root [in] argument count
 * @param a_sKeyName [in] key from JSON
 *
 * @return boolean [out] return true on success and false on failure
 */
bool isElementExistInJson(cJSON *root, std::string a_sKeyName)
{
	bool bRetVal = false;
	cJSON *pBaseptr = cJSON_GetObjectItem(root, a_sKeyName.c_str());
	if(NULL != pBaseptr)
	{
		// key found in json
		bRetVal = true;
	}
	return bRetVal;
}

/**
 * Function to read the device contexts
 */
void setDevContexts()
{
	DO_LOG_DEBUG("Start");

	using network_info::eNetworkType;
	using network_info::CWellSiteDevInfo;
	using network_info::CWellSiteInfo;
	// 1. Set context for each device - TCP and RTU
	// 2. For RTU, for each network context is obtained and then set in each RTU device.
	// 3. For TCP, for each device, a different context is set.
	auto &siteList = network_info::getWellSiteList();
	for(auto &site: siteList)
	{
		auto &listDev = site.second.getDevices();
		for(auto &dev : listDev)
		{
			if(network_info::eNetworkType::eTCP == dev.getAddressInfo().m_NwType)
			{
#ifdef MODBUS_STACK_TCPIP_ENABLED
				int iCtx = 0;
				/// create context for unique ip-address and port number.
				stCtxInfo objCtxInfo{0};
				unsigned char	u8IpAddr[4];
				CommonUtils::ConvertIPStringToCharArray(dev.getAddressInfo().m_stTCP.m_sIPAddress, &(u8IpAddr[0]));

				objCtxInfo.u16Port = dev.getAddressInfo().m_stTCP.m_ui16PortNumber;
				objCtxInfo.pu8SerIpAddr = u8IpAddr;
				eStackErrorCode retValue = getTCPCtx(&iCtx, &objCtxInfo);
				if(STACK_NO_ERROR != retValue)
				{
					DO_LOG_ERROR(dev.getID() + ": Unable to create context. Error: " + std::to_string(retValue));
				}
				else
				{
					dev.setCtxInfo(iCtx);
					DO_LOG_INFO(dev.getID() + ": Context is set");
				}
#endif
			}
			else
			{
#ifndef MODBUS_STACK_TCPIP_ENABLED
				eParity parity = eNone;
				const string sParity = dev.getRTUNwInfo().getParity();
				int iRTUCTX;
				if(!(sParity.empty() && dev.getRTUNwInfo().getPortName().empty()) && dev.getRTUNwInfo().getBaudRate() >= 0)
				{
					if(!(sParity == "N" || sParity == "n" ||
							sParity == "E" || sParity == "e" ||
							sParity == "O" || sParity == "o"))
					{
						DO_LOG_ERROR("Set Parity is wrong for RTU. Set correct parity N/E/O");
					}
					else
					{
						parity = (sParity == "N" || sParity == "n") ? eNone : (sParity == "O" || sParity == "o") ? eOdd : eEven;

						stCtxInfo objCtxInfo{0};
						objCtxInfo.m_eParity = parity;
						objCtxInfo.m_lInterframeDelay = dev.getRTUNwInfo().getInterframeDelay();
						objCtxInfo.m_lRespTimeout = dev.getRTUNwInfo().getResTimeout();
						objCtxInfo.m_u32baudrate = dev.getRTUNwInfo().getBaudRate();
						objCtxInfo.m_u8PortName = (uint8_t*)((dev.getRTUNwInfo().getPortName()).c_str());

						eStackErrorCode retValue = getRTUCtx(&iRTUCTX, &objCtxInfo);

						if(STACK_NO_ERROR != retValue)
						{
							DO_LOG_ERROR("RTU: Unable to create context. Error: " + std::to_string(retValue));
						}
						else
						{
							dev.setCtxInfo(iRTUCTX);
							DO_LOG_INFO(dev.getID() + ": Context is set");
						}
					}
				}
				else
				{
					DO_LOG_ERROR("RTU: configuration is not proper.");
				}
#endif
			}
		}
	}

	DO_LOG_DEBUG("End");
}

/**
 *
 * DESCRIPTION
 * Function to initialise the structure values of stUWCComnDataVal_t of uwc-lib
 *
 * @param strDevMode	[in] describes is devMode enabled
 * @param strAppName	[in] application name
 *
 * @return
 */
void initializeCommonData(string strDevMode, string strAppName)
{
	stUWCComnDataVal_t stUwcData;
	stUwcData.m_devMode = false;
	stUwcData.m_sAppName = strAppName;
	stUwcData.m_isCommonDataInitialised = true;
	transform(strDevMode.begin(), strDevMode.end(), strDevMode.begin(), ::toupper);
	if("TRUE" == strDevMode)
	{
		stUwcData.m_devMode = true;
	}
	CcommonEnvManager::Instance().ShareToLibUwcCmnData(stUwcData);
}

/**
 * Function to read all common environment variables
 * @return
 */
void readEnvData()
{
	if(false == EnvironmentInfo::getInstance().readCommonEnvVariables(m_vecEnv))
	{
		DO_LOG_ERROR("Error while reading the common environment variables");
		exit(1);
	}
	string strDevMode = EnvironmentInfo::getInstance().getDataFromEnvMap("DEV_MODE");

	string strAppName = EnvironmentInfo::getInstance().getDataFromEnvMap("AppName");
	initializeCommonData(strDevMode, strAppName);
	if(strAppName.empty())
	{
		exit(1);
	}

	PublishJsonHandler::instance().setAppName(strAppName);
}

/**
 *
 * DESCRIPTION
 * This function is entry point for application
 *
 * @param argc [in] argument count
 * @param argv [in] argument value
 *
 * @return int [out] return 1 on success
 */
int main(int argc, char* argv[])
{
	try
	{	
		CLogger::initLogger(std::getenv("Log4cppPropsFile"));
		DO_LOG_DEBUG("Starting Modbus_App ...");

		DO_LOG_INFO("Modbus container app version is set to :: " + std::string(APP_VERSION));

		// load global configuration for container real-time setting
		bool bRetVal = globalConfig::loadGlobalConfigurations();
		if(!bRetVal)
		{
			DO_LOG_INFO("Global configuration is set with some default parameters");
		}
		else
		{
			DO_LOG_INFO("Global configuration is set successfully");
		}

		readEnvData();

		string cutOff;
		if(!CommonUtils::readEnvVariable("CUTOFF_INTERVAL_PERCENTAGE", cutOff))
		{
			DO_LOG_INFO("CUTOFF_INTERVAL_PERCENTAGE env variables are not set; setting it to default i.e. 90");
			PublishJsonHandler::instance().setCutoffIntervalPercentage(90);
		}
		else
		{
			PublishJsonHandler::instance().setCutoffIntervalPercentage(atoi(cutOff.c_str()));
		}
		DO_LOG_INFO("Cutoff is set to: " + std::to_string(PublishJsonHandler::instance().getCutoffIntervalPercentage()));

		int num_of_publishers = zmq_handler::getNumPubOrSub("pub");
		// Initializing all the pub/sub topic base context for ZMQ
		if(num_of_publishers >= 1)
		{
			if(true != zmq_handler::prepareCommonContext("pub"))
			{
				DO_LOG_ERROR("Context creation failed for pub topic ");
				// exit the application if the context creation fails
				return -1; 
			}
			else
			{
				std::vector<std::string> vecTopics;
				bool tempRet = zmq_handler::returnAllTopics("pub", vecTopics);
				if(tempRet == false) {
					return -1;
				} 

				for(auto eachTopic : vecTopics) {
					PublishJsonHandler::instance().setTopicForOperation(eachTopic);
				} 
			}
		}
		else
		{
			DO_LOG_ERROR("could not find any Publishers in publisher Configuration ");
		}

		int num_of_subscribers = zmq_handler::getNumPubOrSub("sub");
		if(num_of_subscribers >= 1)
		{
			if( true != zmq_handler::prepareCommonContext("sub"))
			{
				DO_LOG_ERROR("Context creation failed for sub topic ");
			}
			else
			{
				std::vector<std::string> vecTopics;
				bool tempRet = zmq_handler::returnAllTopics("sub", vecTopics);
				if(tempRet == false) {
					return -1;
				} 

				for(auto eachTopic : vecTopics) {
					PublishJsonHandler::instance().setTopicForOperation(eachTopic);
				} 
				//CcommonEnvManager::Instance().splitString(static_cast<std::string>(pcSubTopic),',');
			}
		}

#ifdef MODBUS_STACK_TCPIP_ENABLED
		// Setting TCP mode
		if(!EnvironmentInfo::getInstance().getDataFromEnvMap("DEVICES_GROUP_LIST_FILE_NAME").empty())
		{
			network_info::buildNetworkInfo(EnvironmentInfo::getInstance().getDataFromEnvMap("NETWORK_TYPE"),
					EnvironmentInfo::getInstance().getDataFromEnvMap("DEVICES_GROUP_LIST_FILE_NAME"),
					EnvironmentInfo::getInstance().getDataFromEnvMap("MY_APP_ID"));
			DO_LOG_INFO("Modbus container application is set to TCP mode");
		}
		else
		{
			DO_LOG_ERROR("Devices group list is not present");
		}

#else

		// Setting RTU mode
		if(!EnvironmentInfo::getInstance().getDataFromEnvMap("DEVICES_GROUP_LIST_FILE_NAME").empty())
		{
			network_info::buildNetworkInfo(EnvironmentInfo::getInstance().getDataFromEnvMap("NETWORK_TYPE"),
					EnvironmentInfo::getInstance().getDataFromEnvMap("DEVICES_GROUP_LIST_FILE_NAME"),
					EnvironmentInfo::getInstance().getDataFromEnvMap("MY_APP_ID"));

			DO_LOG_INFO("Modbus container application is set to RTU mode");
		}
		else
		{
			DO_LOG_ERROR("Devices group list is not present");
		}

#endif

		// set TCP/RTU context.
		setDevContexts();

		// get interframe delay and response timeout
		long lInterfameDelay = 0, lRespTimeout = 80;
		auto &siteList = network_info::getWellSiteList();
		for(auto &site: siteList)
		{
			auto &listDev = site.second.getDevices();
			for(auto &dev : listDev)
			{
				if(network_info::eNetworkType::eTCP == dev.getAddressInfo().m_NwType)
				{
					lInterfameDelay = dev.getTcpMasterInfo().m_lInterframeDelay;
					lRespTimeout = dev.getTcpMasterInfo().m_lResTimeout;
				}
			}
		}
		stDevConfig_t stDevConf;
		stDevConf.m_lInterframedelay = lInterfameDelay;
		stDevConf.m_lResponseTimeout = lRespTimeout;
		if(STACK_NO_ERROR != AppMbusMaster_SetStackConfigParam(&stDevConf))
		{
			DO_LOG_ERROR("Error :: Exiting. Failed to set stack  config parameters");
			return -1;
		}
		else
		{
			DO_LOG_INFO("Success :: modbus stack set config successful");
		}

		uint8_t	u8ReturnType = AppMbusMaster_StackInit();
		if(0 != u8ReturnType)
		{
			DO_LOG_ERROR("Error :: Exiting. Failed to initialize modbus stack:" +
					std::to_string(u8ReturnType));

			exit(1);
		}
		else
		{
			DO_LOG_INFO("Success :: modbus stack initialization successful");
		}

		if(false == onDemandHandler::Instance().isWriteInitialized())
		{
			DO_LOG_ERROR("modWriteHandler is not initialized");
		}
		else
		{
			DO_LOG_DEBUG("modWriteHandler is properly initialized");
			/// On-Demand request initializer thread.
			onDemandHandler::Instance().createOnDemandListener();
		}

		// ZMQ contexts are built
		// Network device data and unique point data are also available
		// Lets build: reference data for polling
		populatePollingRefData();

		if(false == CPeriodicReponseProcessor::Instance().isInitialized())
		{
			DO_LOG_ERROR("CPeriodicReponseProcessor is not initialized");
		}
		else
		{
			DO_LOG_INFO("CPeriodicReponseProcessor is properly initialized");
			CPeriodicReponseProcessor::Instance().initRespHandlerThreads();
		}


#ifdef UNIT_TEST
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
#endif

		DO_LOG_INFO("Configuration done. Starting operations.");
		CTimeMapper::instance().initTimerFunction();

		// Get best possible polling frequency
		uint32_t ulMinFreq = CTimeMapper::instance().getMinTimerFrequency();
		DO_LOG_INFO("Best possible minimum frequency in milliseconds: " + std::to_string(ulMinFreq));
		DO_LOG_INFO("Starting periodic timer");
		PeriodicTimer::timer_start(ulMinFreq);
		DO_LOG_INFO("Timer is started..");

		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck,exitMainThread);

		DO_LOG_INFO("Condition variable is set for application exit.");
		DO_LOG_WARN("Exiting the Modbus application gracefully.");

		return EXIT_SUCCESS;
	}
	catch (const std::exception &e)
	{
		DO_LOG_FATAL("Exception in main :: fatal::Error in getting arguments: " +
				(string)e.what());

		return EXIT_FAILURE;
	}
	DO_LOG_DEBUG("End");

	return EXIT_SUCCESS;
}
