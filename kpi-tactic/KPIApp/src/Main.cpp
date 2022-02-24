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

#include <string>
#include <atomic>
#include<algorithm>
#include "Common.hpp"
#include "KPIAppConfigMgr.hpp"
#include "CommonDataShare.hpp"
#include "ConfigManager.hpp"
#include "cjson/cJSON.h"
#include <mutex>
#include "QueueMgr.hpp"
#ifdef UNIT_TEST
#include <gtest/gtest.h>
#endif


/// flag to stop all running threads
std::atomic<bool> g_stopThread(false);

std::vector<std::thread> g_vThreads;

/**
 * Thread function to read requests from queue filled up by MQTT and send data to EII
 * @param qMgr 	:[in] pointer to respective queue manager
 * @return None
 */
void postMsgsToWriteOnMQTT(CQueueHandler& qMgr)
{
	DO_LOG_DEBUG("Starting thread to send messages on EII");

	try
	{
		CControlLoopMapper& oCtrlLoopMapper = CKPIAppConfig::getInstance().getControlLoopMapper();
		while (false == g_stopThread.load())
		{
			CMessageObject recvdMsg;
			if(true == qMgr.isMsgArrived(recvdMsg))
			{
				std::string sTopic{recvdMsg.getTopic()};
				//if(CKPIAppConfig::getInstance().getControlLoopMapper().isControlLoopPollPoint(sTopic))
				{
					oCtrlLoopMapper.triggerControlLoops(sTopic, recvdMsg);
				}
			}
		}
		std::cout << "Exiting thread::postMsgsToWriteOnMQTT \n";
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Function to analyze the data of control loop
 * @param qMgr 	:[in] pointer to respective queue manager
 * @return None
 */
void analyzeControlLoopData(CQueueHandler& qMgr)
{
	try
	{
		CControlLoopMapper& oCtrlLoopMapper = CKPIAppConfig::getInstance().getControlLoopMapper();
		while (false == g_stopThread.load())
		{
			CMessageObject recvdMsg;
			if(true == qMgr.isMsgArrived(recvdMsg))
			{
				// Commented following code for optimization 
				/*if(false == oCtrlLoopMapper.isControlLoopWrRspPoint(recvdMsg.getTopic()))
				{
					DO_LOG_DEBUG(recvdMsg.getTopic() + ": Not a part of control loop. Ignored");
					continue;
				}*/

				std::string sAppSeqVal{commonUtilKPI::getValueofKeyFromJSONMsg(recvdMsg.getStrMsg(), "app_seq")};
				if(true == sAppSeqVal.empty())
				{
					DO_LOG_ERROR(recvdMsg.getStrMsg() + ": app_seq key not found. Ignoring the message");
					continue;
				}

				struct stPollWrData oTempData{};
				if(true == CMapOfReqMapper::getInstace().getForProcessing(sAppSeqVal, oTempData))
				{
					oCtrlLoopMapper.pushAnalysisMsg(oTempData, recvdMsg);
				}
				else
				{
					DO_LOG_DEBUG(sAppSeqVal + ": Waited writeRequest is not found in poll mapping");
				}
			}
		}
		std::cout << "Exiting thread::analyzeControlLoopData \n";
	}
	catch (const std::exception &e)
	{
		std::cout << "Exception occured while publishing data: "<<e.what() << std::endl;
	}

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
void initializeCommonData(std::string strDevMode, std::string strAppName)
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
 * Function to read all environment variables
 *
 * @return
 */
void setEnvData()
{
	std::vector<std::string> vecEnv{"AppName", "MQTT_URL", "DEV_MODE", "KPIAPPConfigFile"};

	if(false == EnvironmentInfo::getInstance().readCommonEnvVariables(vecEnv))
	{
		DO_LOG_ERROR("Error while reading the common environment variables");
		exit(1);
	}

	std::string strDevMode = EnvironmentInfo::getInstance().getDataFromEnvMap("DEV_MODE");
	std::string strAppName = EnvironmentInfo::getInstance().getDataFromEnvMap("AppName");
	if(strAppName.empty())
		{
			exit(1);
		}
	initializeCommonData(strDevMode, strAppName);
}

/**
 * This function is entry point for application
 * @param argc [in] argument count
 * @param argv [in] argument value
 * @return int [out] return 1 on success
 */
int main(int argc, char* argv[])
{
	try
	{
		CLogger::initLogger(std::getenv("Log4cppPropsFile"));
		
		DO_LOG_DEBUG("Starting KPI_App ...");

		setEnvData();

#ifdef UNIT_TEST
		::testing::InitGoogleTest(&argc, argv);
		return RUN_ALL_TESTS();
#endif

		if(false == CKPIAppConfig::getInstance().parseYMLFile(EnvironmentInfo::getInstance().getDataFromEnvMap("KPIAPPConfigFile")))
		{
			DO_LOG_ERROR("Error while loading the configuration file");
			return EXIT_FAILURE;

		}
		CKPIAppConfig::getInstance().getControlLoopMapper().configControlLoopOps(CKPIAppConfig::getInstance().isRTModeForWriteOp());

		PlBusMgr::initPlatformBusHandler(CKPIAppConfig::getInstance().isMQTTModeOn());

		//threads to send on-demand requests on EII
		g_vThreads.push_back(std::thread(postMsgsToWriteOnMQTT, std::ref(QMgr::PollMsgQ())));
		g_vThreads.push_back(std::thread(analyzeControlLoopData, std::ref(QMgr::WriteRespMsgQ())));

		DO_LOG_INFO("Configuration done. Starting operations.");
		auto uiTime = CKPIAppConfig::getInstance().getExecutionTime();
		if(0 != uiTime)
		{
			std::cout << "Starting the Timer for : " << uiTime << " minutes.\n";
			std::this_thread::sleep_for(std::chrono::minutes(uiTime));
			g_stopThread.store(true);

			if(!CKPIAppConfig::getInstance().isMQTTModeOn())
			{
				CKPIAppConfig::getInstance().getControlLoopMapper().destroySubCtx();
			}
			CKPIAppConfig::getInstance().getControlLoopMapper().stopControlLoopOps();
			QMgr::PollMsgQ().breakWaitOnQ();
			QMgr::WriteRespMsgQ().breakWaitOnQ();
			// give 1 second time for all threads to be signalled
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
		else
		{
			std::cout << "Execution limit time is not set. \n";
		}
		for (auto &th : g_vThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		std::cout << "Timeout done :)\n\n";
	}
	catch (const std::exception &e)
	{
		std::cout << "Exception in main:: fatal::Error in getting arguments: "<<e.what()<< std::endl;
		DO_LOG_FATAL("fatal::Error in getting arguments: " +
					(std::string)e.what());

		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

