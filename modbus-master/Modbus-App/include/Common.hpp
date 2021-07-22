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
/***Common.hpp responsible API calls and app call backs for RT , non-RT for read and write functionalities */

#ifndef INCLUDE_INC_COMMON_HPP_
#define INCLUDE_INC_COMMON_HPP_

#include "ZmqHandler.hpp"
#include "ConfigManager.hpp"
#include <string>
#include <iostream>
#include <variant>
#include "EnvironmentVarHandler.hpp"
#include <vector>
#ifdef __cplusplus
extern "C" {
#include "API.h"
}
#endif
#include <map>
#include <inttypes.h>

#ifdef __linux

#define TRUE true
#define FALSE false
#endif

#define MODBUS_SINGLE_REGISTER_LENGTH (2)

#define WIDTH_ONE 	1
#define WIDTH_TWO 	2
#define WIDTH_FOUR 	4

using namespace std;
using var_hex = std::variant<std::monostate, bool, uint16_t, uint32_t, uint64_t, int16_t, int32_t, int64_t, float, double, std::string>;

/** This structure defines the parameters for On Demand Request **/
struct stOnDemandRequest
{
	std::string m_strAppSeq; /** app sequence number **/
	std::string m_strWellhead; /** well head name **/
	std::string m_strMetric; /** Metric name **/
	std::string m_strVersion; /** version number **/
	std::string m_strTopic; /** Topic name **/
	std::string m_sValue; /** data value **/
	var_hex 	m_ScaledValue; /** ScaledValue **/
	std::string m_sUsec; /** Seconds number **/
	std::string m_sTimestamp; /** TimeStamp value **/
	bool m_isByteSwap; /** ByteSwap(true or false)**/
	bool m_isWordSwap; /** WordSwap(true or false) **/
	bool m_isRT; /** Real Time(true or false) **/
	struct timespec m_obtReqRcvdTS; /** Timestamp showing when a request is received **/
	std::string m_strMqttTime; /** value of mqtt time **/
	std::string m_strEiiTime; /** value of eii time **/
	std::string m_sDataType; /** data type*/
	double m_dscaleFactor;
	int m_iWidth;
	bool m_bIsDataPersist; /** Data Persist flag **/

};

/** This structure defines generic parameters for modbus common API **/
typedef struct MbusAPI
{
	unsigned char	m_u8DevId; /** Device ID number **/
	unsigned short  m_u16TxId; /** Transaction ID number **/
	unsigned short  m_u16Quantity; /** Quantity value **/
	unsigned short  m_u16StartAddr; /** start address value **/
	unsigned short  m_u16ByteCount; /** Byte count number **/
	unsigned char  m_pu8Data[260]; /**  Data value **/
	long m_lPriority; /** Holds the Msg Priority  */
	int m_nRetry; /** number of retries*/
	int32_t m_i32Ctx; /** context*/
	stOnDemandRequest m_stOnDemandReqData; /** object of struct stOnDemandRequest*/

	// constructor
	MbusAPI(unsigned char	a_u8DevId = 0,
			unsigned short  a_u16TxId = 0,
			unsigned short  a_u16Quantity = 0,
			unsigned short  a_u16StartAddr = 0,
			unsigned short  a_u16ByteCount = 0,
			long a_lPriority = 0,
			int a_nRetry = 0,
			int32_t a_i32Ctx = 0,
			stOnDemandRequest a_stOnDemandReqData = {{0}}):
			m_u8DevId{a_u8DevId},
			m_u16TxId{a_u16TxId},
			m_u16Quantity{a_u16Quantity},
			m_u16StartAddr{a_u16StartAddr},
			m_u16ByteCount{a_u16ByteCount},
			m_lPriority{a_lPriority},
			m_nRetry{a_nRetry},
			m_i32Ctx{a_i32Ctx},
			m_stOnDemandReqData{a_stOnDemandReqData}{}
}MbusAPI_t;

/** Function to get function code*/
unsigned char GetFunctionCode(uint8_t u8ReadWrite,
								  uint16_t u16Quantity,
								  uint8_t u8MbudTable);

/**This enumerator defines modbus app error codes
  * SUCCESS: code 0
  * DUMMY_RESPONSE: code 100
  * REQUEST_SEND_FAILED: code 101
  * INVALID_INPUT_JSON: code 102
  * CUTOFF_TIME_INTERVAL: code 103
  * INVALID_FUNCTION_CODE: code 104
  * EMPTY_DATA_RECVD_FROM_STACK: code 105
  * JSON_PARSING_EXCEPTION: code 106
  * UNKNOWN_SERVICE_REQUEST: code 107
  * POINT_IS_NOT_WRITABLE: code 108
  * INTERNAL_ERORR: code 109
  * INVALID_CTX: code 110
  * CODE_MAX: code 111
 */
typedef enum MbusAppErrorCode
{
	APP_SUCCESS = 0,
	APP_ERROR_DUMMY_RESPONSE = 100,
	APP_ERROR_REQUEST_SEND_FAILED,
	APP_ERROR_INVALID_INPUT_JSON,
	APP_ERROR_CUTOFF_TIME_INTERVAL,
	APP_ERROR_INVALID_FUNCTION_CODE,
	APP_ERROR_EMPTY_DATA_RECVD_FROM_STACK,
	APP_JSON_PARSING_EXCEPTION,
	APP_ERROR_UNKNOWN_SERVICE_REQUEST,
	APP_ERROR_POINT_IS_NOT_WRITABLE,
	APP_INTERNAL_ERORR,
	APP_ERROR_INVALID_CTX,
	APP_ERROR_CODE_MAX
}eMbusAppErrorCode;

/**This enumerator defines modbus app Request type code
 * NONE: code 0
 * READ: code 1
 * READRT: code 2
 * WRITE: code 3
 * WRITERT: code 4
*/
enum eMbusRequestType
{
	MBUS_REQUEST_NONE,
	MBUS_REQUEST_READ,
	MBUS_REQUEST_READRT,
	MBUS_REQUEST_WRITE,
	MBUS_REQUEST_WRITERT,
};

/**This enumerator defines the Modbus app call back type code
 * POLLING: code 0
 * POLLING_RT: code 1
 * ONDEMAND_READ: code 2
 * ONDEMAND_READ_RT: code 3
 * ONDEMAND_WRITE: code 4
 * ONDEMAND_WRITE_RT:code 5

*/
enum eMbusCallbackType
{
	MBUS_CALLBACK_POLLING,
	MBUS_CALLBACK_POLLING_RT,
	MBUS_CALLBACK_ONDEMAND_READ,
	MBUS_CALLBACK_ONDEMAND_READ_RT,
	MBUS_CALLBACK_ONDEMAND_WRITE,
	MBUS_CALLBACK_ONDEMAND_WRITE_RT,
};

/** Enumerator specifying datatype of datapoints in yml files*/
enum eYMlDataType
{
	enBOOLEAN = 0,
	enUINT,
	enINT,
	enFLOAT,
	enDOUBLE,
	enSTRING,
	enUNKNOWN
};
/* Union of unsigned long long int and float */
typedef union
{
	unsigned long long int hexValue;
	float             actualFltVal;
} hexStrToFlt;

/* Union of unsigned long long int and double */
typedef union
{
	unsigned long long int hexValue;
	double             actualDblVal;
} hexStrToDbl;

/** function to call Modbus stack APIs */
uint8_t Modbus_Stack_API_Call(unsigned char u8FunCode,
								MbusAPI_t *pstMbusApiPram,void* vpCallBackFun);

void OnDemandRead_AppCallback(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);
void OnDemandReadRT_AppCallback(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);
void OnDemandWrite_AppCallback(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);
void OnDemandWriteRT_AppCallback(stMbusAppCallbackParams_t *pstMbusAppCallbackParams, uint16_t uTxID);

/**This namespace handles the common functionality of getting the request Json, inserting entry in map
 * updating the map and removing the entry from the map
*/
namespace common_Handler
{

std::string swapConversion(std::vector<unsigned char> vt, bool a_bIsByteSwap = false, bool a_bIsWordSwap = false);

bool getReqData(unsigned short seqno, MbusAPI_t& reqData);

bool insertReqData(unsigned short, MbusAPI_t&);

bool updateReqData(unsigned short, MbusAPI_t&);

void removeReqData(unsigned short);

long getReqPriority(const globalConfig::COperation a_Ops);

//Convert hex string to unsigned short int
unsigned short int  hexBytesToUShortInt(std::string str);

//Convert hex string to short int
short int hexBytesToShortInt(std::string str);

//Convert hex string to int
int hexBytesToInt(std::string str);

//Convert hex string to unsigned  int
unsigned int hexBytesToUnsignedInt(std::string str);

//Convert hex string to unsigned long int
long int hexBytesToLongInt(std::string str);

//Convert hex string to unsigned long int
unsigned long int hexBytesToUnsignedLongInt(std::string str);

//Convert hex string to long long int
long long int hexBytesToLongLongInt(std::string str);

//Convert hex string to unsigned long long int
unsigned long long int hexBytesToUnsignedLongLongInt(std::string str);

//Convert hex string to unsigned short int
float hexBytesToFloat(std::string str);

//Convert hex string to double
double hexBytesToDouble(std::string str);

//Convert hex string to long double
long double hexBytesToLongDouble(std::string str);

//Convert hex string bool
bool hexBytesToBool(std::string str);

//Convert hex string to string
std::string hexBytesToString(std::string str);

// getDataType
eYMlDataType getDataType(std::string a_sDataType);
}

#endif /* INCLUDE_INC_COMMON_HPP_ */
