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

#include "ControlLoopHandler.hpp"
#include "QueueMgr.hpp"
#include "Common.hpp"
#include "KPIAppConfigMgr.hpp"
#include <algorithm>
#include "eii/msgbus/msg_envelope.h"
#include "ZmqHandler.hpp"

extern std::atomic<bool> g_stopThread;

/**
 * This function posts a dummy analysis message in case of error
 * @param a_oPollData	[in]: Poll data message
 * @param a_sAppSeq	[in]: Probable app-seq number for write op
 * @param a_sError	[in]: Error info 
 * @return none
 */
void CControlLoopOp::postDummyAnalysisMsg(struct stPollWrData &a_oPollData, const std::string &a_sAppSeq, const std::string &a_sError) const
{
	try
	{
		std::string sDummyErrorRep;
		commonUtilKPI::addFieldToMsg(sDummyErrorRep, "app_seq", a_sAppSeq, false);
		commonUtilKPI::addFieldToMsg(sDummyErrorRep, "data_topic", m_sWritePointFullPath + "/writeResponse", false);
		commonUtilKPI::addFieldToMsg(sDummyErrorRep, "error_code", a_sError + "/writeResponse", true);
		sDummyErrorRep = "{" + sDummyErrorRep + "}";
		CMessageObject oDummyMsg{"errorDummyTopic", sDummyErrorRep};
		CKPIAppConfig::getInstance().getControlLoopMapper().pushAnalysisMsg(a_oPollData, oDummyMsg);
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR((std::string)ex.what());
	}
}

/**
 * This function posts a dummy analysis message in case of error
 * @param a_sAppSeq	[in]: Probable app-seq number for write op
 * @param a_sError	[in]: Error info 
 * @return none
 */
void CControlLoopOp::postDummyAnalysisMsg(const std::string &a_sAppSeq, const std::string &a_sError) const
{
	try
	{
		struct stPollWrData oTempData{};
		if(true == CMapOfReqMapper::getInstace().getForProcessing(a_sAppSeq, oTempData))
		{
			postDummyAnalysisMsg(oTempData, a_sAppSeq, a_sError);
		}
		else
		{
			DO_LOG_ERROR(a_sAppSeq + ": Control-loop Data associated with this sequence number not found.");
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR((std::string)ex.what());
	}
}

/**
 * Thread function for a control loop to check polling message and send a write request
 * @return true/false based on success/failure
 */
void CControlLoopOp::threadPollMonitoring()
{
	std::cout << "Started new thread for : " << m_sPolledTopic 
		<< ", write point: " << m_sWritePointFullPath 
		<< ", ID: " << m_sId << std::endl;
	
	DO_LOG_INFO("Started new thread for : " + m_sPolledTopic 
		+ ", write point: " + m_sWritePointFullPath);
	std::string sLastWrSeqVal{""};
	uint32_t interval = m_uiDelayMs*1000*1000;
	try
	{
		while (false == g_stopThread.load())
		{
			CMessageObject recvdMsg{};
			if(true == m_q.isMsgArrived(recvdMsg))
			{								
				struct timespec ts;
				int rc = clock_gettime(CLOCK_MONOTONIC, &ts);
				// Message is received
				// Check if writeResponse for last message was received
				std::cout<<"\nCheck if writeResponse for last message was received\n";
				if(true == CMapOfReqMapper::getInstace().isPresent(m_sId, sLastWrSeqVal))
				{
					postDummyAnalysisMsg(sLastWrSeqVal, "WrRespNotRcvd");
				}
				sLastWrSeqVal.clear();

				// Get driver sequence number for sending a write request
				std::string sWrSeqVal{commonUtilKPI::getValueofKeyFromJSONMsg(recvdMsg.getStrMsg(), "driver_seq")};
				if(true == sWrSeqVal.empty())
				{
					DO_LOG_ERROR(recvdMsg.getStrMsg() + ": driver_seq key not found. Ignoring the message");
					continue;
				}
				sWrSeqVal.append("-" + EnvironmentInfo::getInstance().getDataFromEnvMap("AppName")+ "-" + m_sId);
				std::string sPollStatus{commonUtilKPI::getValueofKeyFromJSONMsg(recvdMsg.getStrMsg(), "status")};
				transform(sPollStatus.begin(), sPollStatus.end(), sPollStatus.begin(), ::toupper);
				if("BAD" == sPollStatus)
				{
					//DO_LOG_ERROR(recvdMsg.getStrMsg() + ": Poll message status is BAD. Write request not sent.");
					struct timespec tsStartWrReqCreate;
					timespec_get(&tsStartWrReqCreate, TIME_UTC);
					struct stPollWrData oTemp{recvdMsg, tsStartWrReqCreate};
					postDummyAnalysisMsg(oTemp, sWrSeqVal, "WrReqNotSent");
					continue;
				}
				// Sleep for configured delay
				if(0 == rc)
				{
					struct timespec sleepts;
					unsigned long next_tick = (ts.tv_sec * 1000000000L + ts.tv_nsec) + interval;
					sleepts.tv_sec = next_tick / 1000000000L;
					sleepts.tv_nsec = next_tick % 1000000000L;
					do
					{
						rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepts, NULL);
					} while(EINTR == rc);
				}

				CKPIAppConfig::getInstance().getControlLoopMapper().publishWriteReq(*this, sWrSeqVal, recvdMsg);
				sLastWrSeqVal.assign(sWrSeqVal);
			}
		}
		std::cout << "Exiting thread::threadPollMonitoring - id " << m_sId << "\n";
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Starts control loop thread
 * @return true/false based on success/failure
 */
bool CControlLoopOp::startThread()
{
	//
	m_thread = std::thread(&CControlLoopOp::threadPollMonitoring, this);
	m_thread.detach();
	return true;
}

/**
 * Stops control loop thread
 * @return true/false based on success/failure
 */
bool CControlLoopOp::stopThread()
{
	try
	{
		m_q.breakWaitOnQ();
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}
	catch (std::exception &ex)
	{
		DO_LOG_ERROR((std::string)ex.what());
	}
	return true;
}

/**
 * Verifies whether point path is in correct format
 * @param a_sFullPath 		[in]: Point's full path
 * @param a_sDevName 		[out]: extracted dev name
 * @param a_sWellHeadName 	[out]: extracted well head name
 * @param a_sPointName 		[out]: extracted point name
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::verifyPointFullPath(const std::string &a_sFullPath, std::string &a_sDevName, 
						std::string &a_sWellHeadName, std::string &a_sPointName)
{
	//
	bool bRet{false};
	try
	{
		do 
		{
			// Format: /dev_name/wellhead/point-name e.g. /iou/PL0/AVal
			// Split it to get individual fields
			std::string delimiter = "/";
			size_t last = 0;
			size_t next = 0;
			std::vector<std::string> vsTopicParts;
			int iDelimiterCnt = 0;
			// Extract dev name
			while ((next = a_sFullPath.find(delimiter, last)) != std::string::npos)
			{
				++iDelimiterCnt;
				std::string sTemp(a_sFullPath.substr(last, next - last));
				if(false == sTemp.empty())
				{
					vsTopicParts.push_back(sTemp);
				}
				last = next + 1;
			}
			vsTopicParts.push_back(a_sFullPath.substr(last));
			if((vsTopicParts.size() == 3) && (3 == iDelimiterCnt))
			{
				a_sDevName.assign(vsTopicParts[0]);
				a_sWellHeadName.assign(vsTopicParts[1]);
				a_sPointName.assign(vsTopicParts[2]);

				bRet = true;
			}		
			
		} while(0);
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());		
	}

	return bRet;
}

/**
 * Adds a control loop data entry into map being maintained
 * @param a_sPolledTopic	:[in] Topic to be monitored for polling
 * @param a_sWriteTopic		:[in] Topic to be used for sending write request
 * @param a_uiDelayMs		:[in] Delay to be used before sending a write request
 * @param a_sVal			:[in] Value to be sent in the write request
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::insertControlLoopData(const std::string &a_sPolledTopic, const std::string &a_sWriteTopic, 
					uint32_t a_uiDelayMs, const std::string &a_sVal)
{
	try
	{
		// Verify point names first
		std::string sDevName{""}, sWellHeadName{""}, sPointName{""};
		bool bRet = verifyPointFullPath(a_sPolledTopic, sDevName, sWellHeadName, sPointName);
		if(false == bRet)
		{
			// Log error 
			DO_LOG_ERROR(a_sPolledTopic + ": Incorrect polling point name.");
			DO_LOG_ERROR(a_sPolledTopic + ": " + a_sWriteTopic + ": Control loop cannot be executed.");
			return false;
		}
		sDevName.clear(); sWellHeadName.clear(); sPointName.clear();
		bRet = verifyPointFullPath(a_sWriteTopic, sDevName, sWellHeadName, sPointName);
		if(false == bRet)
		{
			// Log error 
			DO_LOG_ERROR(a_sWriteTopic + ": Incorrect write point name.");
			DO_LOG_ERROR(a_sPolledTopic + ": " + a_sWriteTopic + ": Control loop cannot be executed.");
			return false;
		}
		
		std::string sPollKey{a_sPolledTopic + "/update"}; 
		auto itr = m_oControlLoopMap.find(sPollKey);
		if (m_oControlLoopMap.end() == itr)
		{
			std::vector<CControlLoopOp> oTemp;
			m_oControlLoopMap.emplace(sPollKey, oTemp);
			itr = m_oControlLoopMap.find(sPollKey);
		}
		if (m_oControlLoopMap.end() != itr)
		{
			++m_uiCtrlLoopCnt;
			CControlLoopOp oLoop{m_uiCtrlLoopCnt, sPollKey, a_sWriteTopic, sDevName, sWellHeadName, sPointName, a_uiDelayMs, a_sVal};
			itr->second.push_back(oLoop);
			m_vsPollTopics.push_back(sPollKey);
			m_vsWrRspTopics.push_back(a_sWriteTopic + "/writeResponse");
			CMapOfReqMapper::getInstace().createNewControlLoopMap(oLoop.getMyID());
		}
	}
	catch(std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * Trigger associated control loops on receiving a polling message
 * @param a_sPolledPoint	:[in] Point being polled
 * @param a_oMsg			:[in] Received message
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::triggerControlLoops(std::string& a_sPolledPoint, CMessageObject &a_oMsg)
{
	try
	{
		auto itrLoop = m_oControlLoopMap.find(a_sPolledPoint);
		if(m_oControlLoopMap.end() != itrLoop)
		{
			auto &vControlLoop = itrLoop->second;
			for (auto &itr : vControlLoop) 
			{
				itr.getQueue().pushMsg(a_oMsg);
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

/*
 * This macro UWC_HIGH_PERFORMANCE_PROCESSOR should be enabled when high performance processor(i7/i10) is used.
 * To enable this  macro, add -DUWC_HIGH_PERFORMANCE_PROCESSOR into the kpi-tactic makefile where kpi app sources are build.
 * When this macro is enabled below version of configControlLoopOps() function should be used.
 * When high performance processor is used then create multiple threads for Logs analysis message (threadAnalysisMsg)
 * and create multiple threads for write request creation (threadWriteReq) for achieving concurrent/parallel operations after reading single queue.
 * When high performance processor is not enabled then create single thread for Logs analysis message (threadAnalysisMsg)
 * and single thread for write request creation (threadWriteReq) for achieving sequential processing (reading queue and proceesing).
 */

#ifdef UWC_HIGH_PERFORMANCE_PROCESSOR

/**
 * This function create and start threadPollMonitoring (control loop monitoring) threads,
 * threadAnalysisMsg (Logs analysis message) threads and threadWriteReq (write request creation) threads.
 * Number of threads (threadPollMonitoring, threadAnalysisMsg, threadWriteReq) depends on number of control loops configured with unique data points.
 *
 * @param a_bIsRTWrite	[in]: It indicates whether write op is RT
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::configControlLoopOps(bool a_bIsRTWrite)
{
	// m_oControlLoopMap keeps track of all the control loops configured in ConfigControlLoop.yml for every unique datapoint.
	// This outer for loop iterates over each entry of unique datapoint present in m_oControlLoopMap. 
	// The inner for loop iterates over the list of control loops for the same datapoint (or duplicate entries of the same datapoint).
	for (auto& itr : m_oControlLoopMap) 
	{
		try
		{
			// listControlLoop will fetch the list of control loops for each iteration of unique datapoint running in outer loop.
			auto &listControlLoop = itr.second;
			DO_LOG_INFO(itr.first + " - Polled Topic, Control loops:" + std::to_string(listControlLoop.size()));
	 		
			// In this nested for loop, iterate over the list of all control loops for the same datapoint (duplicate entries).
			// Iterate over each item of list listControlLoop to start monitoring the polling update messages 
			// and finally create write request message			
			for (auto& rCtrlLoop : listControlLoop) 
			{
				// Here start each control loop thread present in list listControlLoop.
				rCtrlLoop.startThread();
			}

			// Here multiple threads of threadAnalysisMsg are created depending upon the unique datapoints. 
			// Each of the threads threadAnalyisMsg will analyse write response messages and create analysis log.
			m_threadAnalysisLogger = std::thread(&CControlLoopMapper::threadAnalysisMsg, this);
			m_threadAnalysisLogger.detach();
			// Similarly multiple threads of write request are created depending upon the unique datapoints.
			// Each of the threads threadWriteReq will create write request message to be sent over either 0mq or mqtt.
			m_threadWrOp = std::thread(&CControlLoopMapper::threadWriteReq, this);
			m_threadWrOp.detach();
		}
		catch(const std::exception& e)
		{
			DO_LOG_ERROR(itr.first + ": error while creating control loop thread for polled point. " + e.what());
		}
	}
	std::cout << "Control loop threads are set. Now start listening\n";
	return true;
}

/*
 * If the macro UWC_HIGH_PERFORMANCE_PROCESSOR is not enabled, below version of configControlLoopOps function will be enabled.
 * In this modified function, single instance of threadAnalysisMsg (Logs analysis message) thread and
 * single instance of threadWriteReq (write request creation) thread is created.
 */

#else

/**
 * This function create and start multiple instances of threadPollMonitoring (control loop monitoring) thread.
 * Number of threadPollMonitoring threads depends on number of control loops configured with unique data points.
 * This function also create and start single instance of threadAnalysisMsg (Logs analysis message) thread and
 * single instance of threadWriteReq (write request creation) thread is created.
 *
 * @param a_bIsRTWrite	[in]: It indicates whether write op is RT
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::configControlLoopOps(bool a_bIsRTWrite)
{	
	// m_oControlLoopMap keeps track of all the control loops configured in ConfigControlLoop.yml for every unique datapoint.
	// This outer for loop iterates over each entry of unique datapoint present in m_oControlLoopMap. 
	// The inner for loop iterates over the list of control loops for the same datapoint (or duplicate entries of the same datapoint).
	for (auto& itr : m_oControlLoopMap) 
	{
		try
		{
			// listControlLoop will fetch the list of control loops for each iteration of unique datapoint running in outer loop.
			auto &listControlLoop = itr.second;
			DO_LOG_INFO(itr.first + " - Polled Topic, Control loops:" + std::to_string(listControlLoop.size()));
			
			// In this nested for loop, iterate over the list of all control loops for the same datapoint (duplicate entries).
			// Iterate over each item of list listControlLoop to start monitoring the polling update messages 
			// and finally create write request message	
			for (auto& rCtrlLoop : listControlLoop) 
			{
			    // Here start each control loop thread present in list listControlLoop.
				rCtrlLoop.startThread();
			}
		}
		catch(const std::exception& e)
		{
			DO_LOG_ERROR(itr.first + ": error while creating control loop thread for polled point. " + e.what());
		}
	}

    /*
	 * Create single instance of threads namely threadAnalysis and threadWriteReq.
	 * With only single instance of each of the threads, huge context switching among the threads are reduced 
	 * therby maintaining efficient CPU utilization.
	 *
	 * However, in order to test on higher CPU processor, one may enable the macro UWC_HIGH_PERFORMANCE_PROCESSOR to create multiple 
	 * threads of Analysis and WriteReqest for parallel processing of messages from queue. For now, with only single instance
	 * of each of threads threadAnalysisMsg and threadWriteReq, messages from queue are processed sequentially. 
	 * (i.e. one after the other) and not concurrently. 
	 */

	try
	{		
		// Create only single thread threadAnalysisMsg to analyse write response messages and create analysis log.
		m_threadAnalysisLogger = std::thread(&CControlLoopMapper::threadAnalysisMsg, this);
		m_threadAnalysisLogger.detach();
		// Create only single thread for sending write request to be sent on either mqtt or 0mq
		m_threadWrOp = std::thread(&CControlLoopMapper::threadWriteReq, this);
		m_threadWrOp.detach();
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR("Error in creating threadAnalysisMsg and threadWriteReq. " + std::string(e.what()));
	}
	std::cout << "Control loop threads are set. Now start listening\n";
	return true;
}

#endif

/**
 * For logging control loop analysis data, a separate logger is used.
 * This function creates the analysis message and sends it to logging thread.
 * @param a_stPollWrData[in]  polling and write data
 * @param a_msgWrResp	[in]  write response message
 * @return none
 */
void CControlLoopMapper::pushAnalysisMsg(struct stPollWrData &a_stPollWrData, CMessageObject &a_msgWrResp)
{
	try
	{
		stAnalysisMsg oMsgSt{a_stPollWrData, a_msgWrResp};
		m_qAnalysisMsg.pushMsg(oMsgSt);
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Thread function: Logs analysis message
 * @param none
 * @return none
 */
void CControlLoopMapper::threadAnalysisMsg()
{
	std::cout << "threadAnalysisMsg started\n";

	while(false == g_stopThread.load())
	{
		/// iterate Queue one by one to send message on Pl-bus
		do
		{
			try
			{
				stAnalysisMsg oMsgSt;
				if(true == m_qAnalysisMsg.isMsgArrived(oMsgSt))
				{
					if(false == g_stopThread.load())
					{
						commonUtilKPI::logAnalysisMsg(oMsgSt.m_stPollWrData, oMsgSt.m_msgWrResp);
					}
				}
			}
			catch(const std::exception& e)
			{
				DO_LOG_ERROR(e.what());
			}
		}while(0);
	}
}

/**
 * Thread function: Logs analysis message
 * @param none
 * @return none
 */
void CControlLoopMapper::threadWriteReq()
{
	std::cout << "threadWriteReq started\n";

	while(false == g_stopThread.load())
	{
		/// iterate Queue one by one to send message on Pl-bus
		do
		{
			try
			{
				stWrOpInputData oWrOpdata{};
				if(true == m_qWrOpData.isMsgArrived(oWrOpdata))
				{
					if(false == g_stopThread.load())
					{
						if(NULL != oWrOpdata.m_pCtrlLoop)
						{
							if(false == PlBusMgr::publishWriteReq(*(oWrOpdata.m_pCtrlLoop), oWrOpdata.m_sWrSeq))
							{
								DO_LOG_ERROR("Failed publishing for point: " + oWrOpdata.m_pCtrlLoop->getWritePoint());
								oWrOpdata.m_pCtrlLoop->postDummyAnalysisMsg(oWrOpdata.m_sWrSeq, "WrReqInitFailed");
							}
						}
					}
				}
			}
			catch(const std::exception& e)
			{
				DO_LOG_ERROR(e.what());
			}
		}while(0);
	}
}

/**
 * Destroys the sub contexts
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::destroySubCtx()
{
	bool retVal = false;
	try
	{
		// get sub topic list
		std::vector<std::string> vFullTopics;
		bool tempRet = zmq_handler::returnAllTopics("sub", vFullTopics);
		if(tempRet == false) {
			return false;
		}

		for(auto& sTopic : vFullTopics)
		{
			zmq_handler::stZmqContext& ctx = zmq_handler::getCTX(sTopic);
			zmq_handler::stZmqSubContext& subContext = zmq_handler::getSubCTX(sTopic);
			if(NULL != subContext.sub_ctx && NULL != ctx.m_pContext)
			{
				zmq_handler::removeSubCTX(sTopic);
				msgbus_recv_ctx_destroy(ctx.m_pContext, subContext.sub_ctx);
			}
			/// free msg bus context
			if(ctx.m_pContext != NULL)
			{
				zmq_handler::removeCTX(sTopic);
				msgbus_destroy(ctx.m_pContext);
				ctx.m_pContext = NULL;
			}
		}
		retVal = true;
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << "Exception while destroying contexts..." << std::endl;
		retVal = false;
	}

	return retVal;
}

/**
 * Stops control loop threads
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::stopControlLoopOps()
{
	//
	for (auto& itr : m_oControlLoopMap) 
	{
		try
		{
			auto &listControlLoop = itr.second;
			for (auto& rCtrlLoop : listControlLoop) 
			{
				rCtrlLoop.stopThread();
			}

			PlBusMgr::stopListeners();

			if (m_threadAnalysisLogger.joinable())
			{
				m_threadAnalysisLogger.join();
			}
			if (m_threadWrOp.joinable())
			{
				m_threadWrOp.join();
			}
		}
		catch(const std::exception& e)
		{
			DO_LOG_ERROR(itr.first + ": error while stopping control loop thread for polled point. " + e.what());
		}
	}

	std::cout << "Control loop threads are signalled for stopping.\n";
	return true;
}

/**
 * Checks whether given polling topic (e.g. "/iou/PL0/AVal/update")
 * is a part of one of the control loops
 * @param a_sPollTopic	[in]: Poll topic to be checked 
 * @return true = yes, false = no
 */
bool CControlLoopMapper::isControlLoopPollPoint(const std::string &a_sPollTopic)
{
	if (std::find(m_vsPollTopics.begin(), m_vsPollTopics.end(), a_sPollTopic) != m_vsPollTopics.end()) 
	{
		return true;
	}
	return false;
}

/**
 * Checks whether given write response topic (e.g. "/iou/PL0/AVal/writeResponse")
 * is a part of one of the control loops
 * @param a_sPollTopic	[in]: Write response topic to be checked 
 * @return true = yes, false = no
 */
bool CControlLoopMapper::isControlLoopWrRspPoint(const std::string &a_sWrRspTopic)
{
	if (std::find(m_vsWrRspTopics.begin(), m_vsWrRspTopics.end(), a_sWrRspTopic) != m_vsWrRspTopics.end()) 
	{
		return true;
	}
	return false;
}

/**
 *Publishes write request
 * @param a_rCtrlLoop	[in]: Control loop for which write needs to be published
 * @param a_sWrSeq		[in]: Sequence number to be used in write request
 * @param a_oPollMsg	[in]: Poll message for which write is being sent
 * @return true/false based on success/failure
 */
bool CControlLoopMapper::publishWriteReq(const CControlLoopOp& a_rCtrlLoop, 
			const std::string &a_sWrSeq, CMessageObject &a_oPollMsg)
{
	try
	{
		struct timespec tsStartWrReqCreate;
		timespec_get(&tsStartWrReqCreate, TIME_UTC);
		struct stPollWrData oTemp{a_oPollMsg, tsStartWrReqCreate};
		CMapOfReqMapper::getInstace().insertForTracking(a_rCtrlLoop.getMyID(), a_sWrSeq, oTemp);
		stWrOpInputData oWrOpdata{a_sWrSeq, &a_rCtrlLoop};
		m_qWrOpData.pushMsg(oWrOpdata);
		return true;
	}
	catch(const std::exception& e)
	{
		DO_LOG_ERROR(a_rCtrlLoop.getWritePoint() + ": Write request error: " +  e.what());
	}
	
	return false;
}

/**
 * Clean up, destroy semaphores, disables callback, disconnect from MQTT broker
 * @param None
 * @return None
 */
template <class T>
void CCtrlLoopInternalQueue<T>::cleanup()
{
	sem_destroy(&m_semaphore);
}

/**
 * Clear queue
 * @param None
 * @return None
 */
template <class T>
void CCtrlLoopInternalQueue<T>::clear()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	m_msgQ = {};
}

/**
 * Constructor of queue manager
 * @param isRead :[in] set if operational instance is for read operation
 * 					or write (toggle between read and write)
 * @param isRealTime :[in] set if operational instance is for real-time operation
 * 					or non-RT (toggle between RT and non-RT)
 * @return None
 */
template <class T>
CCtrlLoopInternalQueue<T>::CCtrlLoopInternalQueue()
{
	initSem();
}

/**
 * Destructor
 */
template <class T>
CCtrlLoopInternalQueue<T>::~CCtrlLoopInternalQueue()
{
	cleanup();
}

/**
 * Initialize semaphores
 * @param None
 * @return true on success; application exits if fails to initialize semaphore
 */
template <class T>
bool CCtrlLoopInternalQueue<T>::initSem()
{
	/* Initial value of zero*/
	if(-1 == sem_init(&m_semaphore, 0, 0))
	{
	   DO_LOG_ERROR("could not create semaphore, exiting");
	   std::cout << __func__ << ":" << __LINE__ << " Error : could not create semaphore, exiting" <<  std::endl;
	   exit(0);
	}

	DO_LOG_DEBUG("Semaphore initialized successfully");

	return true;
}

/**
 * Push message in operational queue
 * @param msg :[in] MQTT message to push in message queue
 * @return true/false based on success/failure
 */
template <class T>
bool CCtrlLoopInternalQueue<T>::pushMsg(T msg)
{
	try
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_msgQ.push(msg);

		sem_post(&m_semaphore);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Push message in operational queue
 * @param none
 * @return true/false based on success/failure
 */
template <class T>
bool CCtrlLoopInternalQueue<T>::breakWaitOnQ()
{
	try
	{
		sem_post(&m_semaphore);
	}
	catch(std::exception &ex)
	{
		DO_LOG_ERROR(ex.what());
		return false;
	}
	return true;
}

/**
 * Retrieve non-RT read message from message queue to publish on EII
 * @param msg :[in] reference to message to retrieve from queue
 * @return true/false based on success/failure
 */
template <class T>
bool CCtrlLoopInternalQueue<T>::getSubMsgFromQ(T& msg)
{
	try
	{
		if(m_msgQ.empty())
		{
			return false;
		}

		std::lock_guard<std::mutex> lock(m_queueMutex);
		msg = m_msgQ.front();
		m_msgQ.pop();
	}
	catch (const std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

/**
 * Checks if a new message has arrived and retrieves the message
 * @param msg :[out] reference to new message
 * @return	true/false based on success/failure
 */
template <class T>
bool CCtrlLoopInternalQueue<T>::isMsgArrived(T& msg)
{
	try
	{
		if((sem_wait(&m_semaphore)) == -1 && errno == EINTR)
		{
			// Continue if interrupted by handler
			return false;
		}
		else
		{
			if (false == getSubMsgFromQ(msg))
			{
				DO_LOG_INFO("No message to send to EII in queue");
				return false;
			}
		}
	}
	catch(std::exception &e)
	{
		DO_LOG_ERROR(e.what());
		return false;
	}
	return true;
}

