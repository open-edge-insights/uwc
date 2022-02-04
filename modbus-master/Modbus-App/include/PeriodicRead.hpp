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

/*** PeriodicRead.hpp is responsible for  stack response, queue operations, post response*/

#ifndef INCLUDE_INC_PERIODICREAD_HPP_
#define INCLUDE_INC_PERIODICREAD_HPP_

#include <semaphore.h>

#include <list>
#include <thread>
#include <queue>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <vector>
#include <numeric>
#include "Common.hpp"
#include "cjson/cJSON.h"
#include "PublishJson.hpp"
#include "ConfigManager.hpp"
#include "API.h"


/**node for response Q*/
struct stStackResponse
{
	stException_t m_stException; /** reference of struct stException_t*/
	std::vector<uint8_t> m_Value; /**vector of data values*/
	bool bIsValPresent; /** Value present or not(true or false) */
	uint8_t u8Reason; /** Reason value */
	uint16_t u16TransacID; /** Transaction ID number */
	stTimeStamps m_objStackTimestamps; /** reference of struct stTimeStamps*/
	long m_lPriority; /** msg priority */
	uint8_t  m_u8FunCode; /** Function code */
	eMbusCallbackType m_operationType; /** type of operation*/
	std::string m_strResponseTopic; /** Response topic name*/
	bool m_bIsRT; /** Real Time (true or false) */
};

class CRefDataForPolling; 

/*class for periodic response */
class CPeriodicReponseProcessor{
private:
	std::queue <stStackResponse> stackPollingResQ; /** polling Response queue*/
	std::queue <stStackResponse> stackRTPollingResQ; /** RT polling Response queue*/
	std::queue <stStackResponse> stackOnDemandReadResQ; /** On-Demand read response queue*/
	std::queue <stStackResponse> stackOnDemandRTReadResQ; /** On-Demand RT read response queue*/
	std::queue <stStackResponse> stackOnDemandWriteResQ; /** On-Demand write response queue*/
	std::queue <stStackResponse> stackOnDemandRTWriteResQ; /** On-Demand RT write response queue*/
	std::mutex __resQPollingMutex; /** polling response queue mutex*/
	std::mutex __resQRTPollingMutex; /** RT polling Response queue mutex*/
	std::mutex __resQODReadMutex; /** On-Demand read response queue mutex*/
	std::mutex __resQODRTReadMutex; /* On-Demand RT read response queue mutex*/
	std::mutex __resQODWriteMutex; /** On-Demand write response queue mutex*/
	std::mutex __resQODRTWriteMutex; /** On-Demand RT write response queue mutex*/
	sem_t semPollingRespProcess; /** semaphore for polling response*/
	sem_t semRTPollingRespProcess; /** semaphore for RT polling response*/
	sem_t semODReadRespProcess; /** semaphore for On-demand read response*/
	sem_t semRTODReadRespProcess; /** semaphore for RT On-demand read response*/
	sem_t semODWriteRespProcess; /** semaphore for On-demand write response*/
	sem_t semRTODWriteRespProcess; /** semaphore for RT On-demand write response*/
	bool m_bIsInitialized; /** true or false*/

	bool pushToQueue(struct stStackResponse &stStackResNode, eMbusCallbackType operationCallbackType);
	bool getDataToProcess(struct stStackResponse &a_stStackResNode, eMbusCallbackType operationCallbackType);
	bool checkForRetry(struct stStackResponse &a_stStackResNode, eMbusCallbackType operationCallbackType);
	void getCallbackForRetry(void** callbackFunc, eMbusCallbackType operationCallbackType);

	bool prepareResponseJson(std::string &a_rtOrNrt, std::string &a_responseMqttTopic, msg_envelope_t** a_pmsg, std::string &a_sValue, const CRefDataForPolling* a_objReqData, stStackResponse a_stResp, struct timespec *a_pstTsPolling);
	bool postResponseJSON(stStackResponse& a_stResp, const CRefDataForPolling* a_objReqData, struct timespec *a_pstTsPolling);
	bool postResponseJSON(stStackResponse& a_stResp);

	bool initSem();
	eMbusAppErrorCode respProcessThreads(eMbusCallbackType operationCallbackType,
			sem_t& a_refSem, globalConfig::COperation& a_refOps);

	CPeriodicReponseProcessor();
	CPeriodicReponseProcessor(CPeriodicReponseProcessor const&);             /// copy constructor is private
	CPeriodicReponseProcessor& operator=(CPeriodicReponseProcessor const&);  /// assignment operator is private
	std::string mapMqttToEMBRespTopic(std::string mqttRespTopic, bool isRealTime, std::string tcpOrRtu);	

public:
	static CPeriodicReponseProcessor& Instance();
	void handleResponse(stMbusAppCallbackParams_t *pstMbusAppCallbackParams,
						eMbusCallbackType operationCallbackType,
						std::string strResponseTopic, bool a_bIsRT);
	bool isInitialized() {return m_bIsInitialized;}
	void initRespHandlerThreads();
	bool postDummyBADResponse(CRefDataForPolling& a_objReqData, const stException_t m_stException, struct timespec *a_pstRefPollTime);
	bool postLastResponseForCutoff(CRefDataForPolling& a_objReqData);
	msg_envelope_elem_body_t* setScaledValue(std::string a_sValue, std::string a_sDataType,double dScaleFactor, int a_iWidth);
};


#endif /* INCLUDE_INC_PERIODICREAD_HPP_ */
