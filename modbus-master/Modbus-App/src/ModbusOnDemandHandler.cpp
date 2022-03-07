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

#include "Logger.hpp"
#include "YamlUtil.hpp"
#include <thread>
#include <mutex>
#include <functional>
#include "NetworkInfo.hpp"
#include "ConfigManager.hpp"
#include <sys/msg.h>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include "eii/utils/json_config.h"
#include "ModbusOnDemandHandler.hpp"
#include <string>
#include <fenv.h>
/// stop thread flag
extern std::atomic<bool> g_stopThread;
/// reference if to store request data

/**
 * Constructor
 */
onDemandHandler::onDemandHandler() : m_bIsWriteInitialized(false)
{
	try
	{
		m_bIsWriteInitialized = true;
	}
	catch(const std::exception& e)
	{
		DO_LOG_FATAL("Unable to initiate write instance");
		DO_LOG_FATAL(e.what());
	}
}

/**
 * Function to compare two strings.
 * @param stBaseString	:[in] original string from which comparison can be made
 * @param strToCompare	:[in] string value to compare with base string
 * @return 	true : on success,
 * 			false : on error
 */
bool onDemandHandler::compareString(const std::string stBaseString, const std::string strToCompare)
{
	return std::equal(stBaseString.begin(), stBaseString.end(),
			strToCompare.begin(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            });
}

/**
 * Function to generate error response if request json is invalid.
 * @param errorCode	:[in] Error code for invalid json
 * @param u8FunCode	:[in] Request function code
 * @param txID		:[in] Transaction id of request.
 * @param isRT		:[in] Flag to differentiate between RT/Non-RT requests.
 * @param isWrite	:[in] Flag to differentiate between read and write for on-demand request
 */
void onDemandHandler::createErrorResponse(eMbusAppErrorCode errorCode,
		uint8_t  u8FunCode,
		unsigned short txID,
		bool isRT,
		bool isWrite)
{
	stMbusAppCallbackParams_t objCallback;
	eMbusCallbackType operationType = MBUS_CALLBACK_ONDEMAND_READ;
	std::string strResponseTopic = "";
	timespec ts{0};
	objCallback.m_u8FunctionCode = u8FunCode;
	objCallback.m_u8ExceptionExcCode = errorCode;
	objCallback.m_u8ExceptionExcStatus = 0;
	objCallback.m_u16TransactionID = txID;
	objCallback.m_objTimeStamps.tsReqRcvd = ts;
	objCallback.m_objTimeStamps.tsReqSent = ts;
	objCallback.m_objTimeStamps.tsRespRcvd = ts;
	objCallback.m_objTimeStamps.tsRespSent = ts;

	if(true == isRT && true == isWrite)
	{
		operationType = MBUS_CALLBACK_ONDEMAND_WRITE_RT;
		strResponseTopic = PublishJsonHandler::instance().getSWriteResponseTopicRT();
	}
	else if(false == isRT && true == isWrite)
	{
		operationType = MBUS_CALLBACK_ONDEMAND_WRITE;
		strResponseTopic = PublishJsonHandler::instance().getSWriteResponseTopic();
	}
	else if(true == isRT && false == isWrite)
	{
		operationType = MBUS_CALLBACK_ONDEMAND_READ_RT;
		strResponseTopic = PublishJsonHandler::instance().getSReadResponseTopicRT();
	}
	else
	{
		operationType = MBUS_CALLBACK_ONDEMAND_READ;
		strResponseTopic = PublishJsonHandler::instance().getSReadResponseTopic();
	}

	/// handle response function to send response on EII.
	CPeriodicReponseProcessor::Instance().handleResponse(&objCallback,
															operationType,
															strResponseTopic,
															isRT);
}

/**
* Handler function to start the processing of on-demand requests.
* @param a_pstMbusApiPram	:[in] Structure to read data received from ZMQ
* @param topic				:[in] topic for zmq listening
* @param vpCallback			:[in] set the stack callback as per the operation
* @param a_bIsWriteReq		:[in] flag used to distinguish read/write request for further processing
* @return 	eMbusAppErrorCode : Error code
*/
eMbusAppErrorCode onDemandHandler::onDemandInfoHandler(MbusAPI_t *a_pstMbusApiPram,
		const string a_sTopic,
		void *vpCallback,
		bool a_IsWriteReq)
{	
	eMbusAppErrorCode eFunRetType = APP_SUCCESS;
	unsigned char  m_u8FunCode;

	// check for NULL
	if(NULL == a_pstMbusApiPram || NULL == vpCallback)
	{
		DO_LOG_ERROR("NULL pointer received..discarding the request");
		return APP_INTERNAL_ERORR;
	}

	try
	{
		/// Transaction ID
		a_pstMbusApiPram->m_u16TxId = PublishJsonHandler::instance().getTxId();
		 
		/// Function called to parse request JSON and fill structure
		eFunRetType = jsonParserForOnDemandRequest(*a_pstMbusApiPram,
				m_u8FunCode,
				a_pstMbusApiPram->m_u16TxId,
				a_IsWriteReq);

		/// inserting structure to map for retry and to create response JSON.
		if(false == common_Handler::insertReqData(a_pstMbusApiPram->m_u16TxId, *a_pstMbusApiPram))
		{
			DO_LOG_ERROR("Failed to add MbusAPI_t data to map.");
		}

		if(APP_SUCCESS == eFunRetType && MBUS_MIN_FUN_CODE != m_u8FunCode)
		{
			eFunRetType = (eMbusAppErrorCode)Modbus_Stack_API_Call(
					m_u8FunCode,
					a_pstMbusApiPram,
					vpCallback);

			if(APP_SUCCESS != eFunRetType)
			{
				DO_LOG_ERROR("Failed to initiate request from stack");
				eFunRetType = APP_ERROR_REQUEST_SEND_FAILED;
			}
			else
			{
				DO_LOG_DEBUG("Request is successfully sent to end device");
			}
		}
		else
		{
			if(APP_ERROR_UNKNOWN_SERVICE_REQUEST != eFunRetType)
			{
				std::string strTopic = "";
				/// error response if request JSON is invalid
				createErrorResponse(eFunRetType,
						m_u8FunCode,
						a_pstMbusApiPram->m_u16TxId,
						a_pstMbusApiPram->m_stOnDemandReqData.m_isRT,
						a_IsWriteReq);
			}
		}
	}
	catch(const std::exception &e)
	{
		eFunRetType = APP_JSON_PARSING_EXCEPTION;
		DO_LOG_FATAL(e.what());
	}

	return eFunRetType;
}

/**
 * Function to convert values from character to integer.
 * @param input :[in] char to convert to integer
 * @return int	:converted integer value
 */
int char2int(char input)
{
	//DO_LOG_DEBUG("Start");
	if(input >= '0' && input <= '9')
		return input - '0';
	if(input >= 'A' && input <= 'F')
		return input - 'A' + 10;
	if(input >= 'a' && input <= 'f')
		return input - 'a' + 10;

	//DO_LOG_DEBUG("End");
	throw std::invalid_argument("Invalid input string");
}

/**
 * Function to convert hex value from string to uint8_t array
 * @param src		:[in] hex string to convert to uint8_t
 * @param iOpLen	:[in] length of the value
 * @param target	:[out] to store converted value
 * @return int		: -1 if error else position of output character
 */
int hex2bin(const std::string &src, int iOpLen, uint8_t* target)
{
	DO_LOG_DEBUG("Start");
	int iOpCharPos = 0;
	int i = 0;
	try
	{
		int iLen = src.length();
		if(0 != (iLen%2))
		{
			DO_LOG_ERROR("input string is not proper");
			return -1;
		}
		// Check if hex string starts with 0x. If yes ignore first 2 letters
		if( ('0' == src[0]) && (('X' == src[1]) || ('x' == src[1])) )
		{
			i = 2;
		}
		iLen = iLen - i;
		if((iLen / 2) != iOpLen)
		{
			DO_LOG_ERROR("width mismatch " + std::to_string(iLen) + "!=" + std::to_string(iOpLen));
			return -1;
		}
		iLen = iLen + i;
		while (i+1 < iLen)
		{
			unsigned char byte1 = char2int(src[i])*16 + char2int(src[i+1]);
			i = i + 2;
			if ((i+1) < iLen)
			{
				unsigned char byte2 = char2int(src[i])*16 + char2int(src[i+1]);
				target[iOpCharPos] = byte2;
				iOpCharPos++;
			}
			target[iOpCharPos] = byte1;
			iOpCharPos++;
			i = i + 2;
		}
	}
	catch(std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		return -1;
	}
	DO_LOG_DEBUG("End");
	return iOpCharPos;
}

/**
 * Function to validate request JSON for wellhead & command
 * @param stSourcetopic	:[in] source topic
 * @param stWellhead	:[in] wellhead name
 * @param stCommand		:[in] command name
 * @return 	true : on success,
 * 			false : on error
 */
bool onDemandHandler::validateInputJson(std::string stSourcetopic, std::string stWellhead, std::string stCommand)
{
	bool retValue = false;
	try
	{
		std::size_t found=std::string::npos;
		std::size_t found1=std::string::npos;
		std::size_t found2=std::string::npos;
		std::size_t found3=std::string::npos;
		std::string strSearchStrin = "/", tempWellhead, tempCommand;
		found = stSourcetopic.find(strSearchStrin);
		if (found!=std::string::npos)
		{
			found1 = stSourcetopic.find(strSearchStrin.c_str(), found+1);
		}
		if (found1!=std::string::npos)
		{
			found2 = stSourcetopic.find(strSearchStrin.c_str(), found1+1);
		}
		if (found2!=std::string::npos)
		{
			tempWellhead = stSourcetopic.substr(found1+1, found2-found1-1);
			found3 = stSourcetopic.find(strSearchStrin.c_str(), found2+1);
		}
		if (found3!=std::string::npos)
		{
			tempCommand = stSourcetopic.substr(found2+1, found3-found2-1);
		}

		if(tempWellhead == stWellhead && tempCommand == stCommand)
		{
			retValue = true;
		}
	}
	catch(const std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		DO_LOG_DEBUG("i/p json wellhead or command mismatch with sourcetopic");
	}

	return retValue;
}


/**
 * Function to reverseScaledValue to Hex String
 * @param a_sDataType		:[in] Datatype of datapoint specified in yml file
 * @param a_iWidth		 	:[in] Width of the datatype as specfied in yml file
 * @param a_dScaleFactor    :[in] Scale Factor as specified in yml file
 * @param a_ScaledValue     :[in] Scaled Value received from EIS
 * @param a_HexValue		:[out] HexValue to be sent to modbus-stack
 * @return true/false depending on the success and failure of the function
 */
bool onDemandHandler::reverseScaledValueToHex(std::string a_sDataType, int a_iWidth,
		double a_dscaleFactor, var_hex a_ScaledValue, std::string &a_HexValue)
{
	std::transform(a_sDataType.begin(), a_sDataType.end(), a_sDataType.begin(), ::tolower);
	eYMlDataType enDtype = common_Handler::getDataType(a_sDataType);
	bool ret = true;
	reverseScaledData oreverseScaledData;
	int64_t variantVal;

	if (enSTRING == enDtype)
	{		
		std::string val;
		if (true  == std::holds_alternative<std::string>(a_ScaledValue))
		{
			val = std::get<std::string>(a_ScaledValue);		
		}		 
		a_HexValue = val;
		return ret;
	}

	switch (a_iWidth)
	{
		case WIDTH_ONE:
		{
			if (enBOOLEAN == enDtype)
			{
				bool val = true;
				if(true == std::holds_alternative<bool>(a_ScaledValue))
				{
					val = std::get<bool>(a_ScaledValue);					
					if (true == val)
					{
						a_HexValue = "0x01";
					}
					else
					{
						a_HexValue = "0x00";
					}				
				}
				else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if (enUINT == enDtype)
			{				
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{ 
					variantVal = std::get<int64_t>(a_ScaledValue);													
					int64_t originalVal = variantVal / a_dscaleFactor;				 
					// checks originalVal is less than min value of uint16_t		
				    if (originalVal < std::numeric_limits<unsigned short>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<unsigned short>::min();
					}
					// checks originalVal is greater than max value of uint16_t
					else if (originalVal > std::numeric_limits<unsigned short>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<unsigned short>::max();
					}
					oreverseScaledData.u16 = originalVal; 
					a_HexValue = convertToHexString(oreverseScaledData.u16, WIDTH_ONE);					
				}
				else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if(enINT == enDtype)			
			{				
				
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{ 
					variantVal = std::get<int64_t>(a_ScaledValue);									 
					int64_t originalVal = variantVal / a_dscaleFactor;	
					// checks originalVal is less than min value of int16	
					if (originalVal < std::numeric_limits<short>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<short>::min();
					}
					// checks originalVal is greater than max value of int16	
					else if (originalVal > std::numeric_limits<short>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<short>::max();
					}			 
					oreverseScaledData.i16 = originalVal;					 
					a_HexValue = convertToHexString(oreverseScaledData.u16, WIDTH_ONE);					
			    }
			    else
				{
					ret = false;
					return ret;
				}
				return ret;
			}			
			else
			{   
			    // Unknown datatype  
				ret = false;
				return ret;
			}
			break;
		}
		case WIDTH_TWO:
		{
			if (enUINT == enDtype)
			{								
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{				
					variantVal = std::get<int64_t>(a_ScaledValue);													 
					int64_t originalVal = variantVal / a_dscaleFactor;	
					// checks originalVal is less than min value of uint32_t		
				    if (originalVal < std::numeric_limits<unsigned int>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<unsigned int>::min();
					}
					// checks originalVal is greater than max value of uint32_t
					else if (originalVal > std::numeric_limits<unsigned int>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<unsigned int>::max();
					}			
					oreverseScaledData.u32 = originalVal;					 
					a_HexValue = convertToHexString(oreverseScaledData.u32, WIDTH_TWO);					
			    }
			    else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if(enINT == enDtype)
			{
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{
					variantVal = std::get<int64_t>(a_ScaledValue);									
					int64_t originalVal = variantVal / a_dscaleFactor;		
					// checks originalVal is less than min value of int32	
					if (originalVal < std::numeric_limits<int>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<int>::min();
					}
					// checks originalVal is greater than max value of int32
					else if (originalVal > std::numeric_limits<int>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<int>::max();
					}			
					oreverseScaledData.i32 = originalVal;				
					a_HexValue = convertToHexString(oreverseScaledData.u32, WIDTH_TWO);					
			    }
			    else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if (enFLOAT == enDtype)
			{
				fesetround(FE_TONEAREST);				
				double valDbl = 0;
				int64_t i64;
				if (true == std::holds_alternative<double>(a_ScaledValue))
				{ 
					valDbl = std::get<double>(a_ScaledValue);										 
				}
				else if (true == std::holds_alternative<int64_t>(a_ScaledValue)) 
				{					 
					i64 = std::get<int64_t>(a_ScaledValue);
					valDbl = (double)i64;					 
				}
				else
				{
					ret = false;
					return ret;
				}
				double originalVal = valDbl / a_dscaleFactor;								
				if (originalVal < std::numeric_limits<float>::lowest())
				{
					// Set originalVal to MIN
					originalVal = std::numeric_limits<float>::lowest();		
				}
				// checks scaledValue is greater than max value of float
				else if (originalVal > std::numeric_limits<float>::max())
				{
					// Set originalVal to MAX
					originalVal = std::numeric_limits<float>::max();		
				}
				// round off float 2 decimal places
				originalVal = round((originalVal * (double) 100)) / 100;
				std::feclearexcept(FE_ALL_EXCEPT);	
				oreverseScaledData.f = originalVal;				
				a_HexValue = convertToHexString(oreverseScaledData.u32, WIDTH_TWO);				
				return ret;
			}
			break;
		}
		case WIDTH_FOUR:
		{
			if (enUINT == enDtype)
			{				
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{
					variantVal = std::get<int64_t>(a_ScaledValue);
					uint64_t originalVal = variantVal / a_dscaleFactor;
					// checks originalVal is less than min value of uint64_t	
					if (originalVal < std::numeric_limits<unsigned long long>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<unsigned long long>::min();
					}
					// checks originalVal is greater than max value of uint64_t
					else if (originalVal > std::numeric_limits<unsigned long long>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<unsigned long long>::max();
					}
					oreverseScaledData.u64 = originalVal;				 
					a_HexValue = convertToHexString(oreverseScaledData.u64, WIDTH_FOUR);
			    }
			    else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if(enINT == enDtype)
			{
				int64_t val = 0;
				if (true  == std::holds_alternative<int64_t>(a_ScaledValue))
				{
					val = std::get<int64_t>(a_ScaledValue);
					int64_t originalVal = val / a_dscaleFactor;
					// checks originalVal is less than min value of int64	
					if (originalVal < std::numeric_limits<long long int>::min())
					{
						// Set originalVal to MIN
						originalVal = std::numeric_limits<long long int>::min();
					}
					// checks originalVal is greater than max value of int64
					else if (originalVal > std::numeric_limits<long long int>::max())
					{
						// Set originalVal to MAX
						originalVal = std::numeric_limits<long long int>::max();
					}
					oreverseScaledData.i64 = originalVal;				
					a_HexValue = convertToHexString(oreverseScaledData.u64, WIDTH_FOUR);
			    }
			    else
				{
					ret = false;
					return ret;
				}
				return ret;
			}
			else if (enDOUBLE == enDtype)
			{
				fesetround(FE_TONEAREST);
				double val = 0.0;
				int64_t i64;				 
				if (true == std::holds_alternative<double>(a_ScaledValue))
				{
					val = std::get<double>(a_ScaledValue);					 
				}
				else if (true == std::holds_alternative<int64_t>(a_ScaledValue))
				{					 
					i64 = std::get<int64_t>(a_ScaledValue);
					val = (double)i64;					 
				}
				else
				{
					ret = false;
					return ret;
				}				
				long double originalVal = val / a_dscaleFactor;
				// checks originalVal is less than min value of double
				if (originalVal < std::numeric_limits<double>::lowest())
				{
					// Set originalVal to MIN
					originalVal = std::numeric_limits<double>::lowest();			
				}
				// checks originalVal is greater than max value of double
				else if (originalVal > std::numeric_limits<double>::max())
				{
					// Set originalVal to MAX
					originalVal = std::numeric_limits<double>::max();			
				}
				originalVal = round(originalVal * (long double) 100) / 100;
 				std::feclearexcept(FE_ALL_EXCEPT);		 									
				oreverseScaledData.d = originalVal;
				a_HexValue = convertToHexString(oreverseScaledData.u64, WIDTH_FOUR);			
				return ret;
			}
			break;
		}
	}
	// Invalid here as no match for datatype and width
	ret = false;
	return ret;
}


/**
 * Function to parse request JSON and fill the structure.
 * @param stMbusApiPram		:[out] modbus API param structure to fill from received msg
 * @param funcCode			:[out] function code of the request
 * @param txID				:[in] request transaction id
 * @param a_IsWriteReq		:[out] boolean variable to differentiate between read/write request
 * @return appropriate error code
 */
eMbusAppErrorCode onDemandHandler::jsonParserForOnDemandRequest(MbusAPI_t& a_stMbusApiPram,
											unsigned char& funcCode,
											unsigned short txID,
											const bool a_IsWriteReq)
{
	// locals
	eMbusAppErrorCode eFunRetType = APP_SUCCESS;
	network_info::CDataPoint obj;
	string strSourceTopic, strValue;
	bool isValidJson = false;
	bool isScaledValue = false;
	try
	{
		/// Comparing sourcetopic for read/write request.
		strSourceTopic = a_stMbusApiPram.m_stOnDemandReqData.m_strTopic;

		/// to check all the values are present in request JSON.
		if(!a_stMbusApiPram.m_stOnDemandReqData.m_strMetric.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strWellhead.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strVersion.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strTopic.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strMqttTime.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strEiiTime.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_strAppSeq.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_sUsec.empty()
				&& !a_stMbusApiPram.m_stOnDemandReqData.m_sTimestamp.empty())
		{
			isValidJson = true;		

			if(true == a_IsWriteReq)
			{
				if(!a_stMbusApiPram.m_stOnDemandReqData.m_sValue.empty())
				{
					strValue = a_stMbusApiPram.m_stOnDemandReqData.m_sValue;
				}
				else 
				{					 
					isScaledValue = true;				 
			    }				 
			}
			if(isValidJson)
			{
				isValidJson = validateInputJson(strSourceTopic,
					a_stMbusApiPram.m_stOnDemandReqData.m_strWellhead,
					a_stMbusApiPram.m_stOnDemandReqData.m_strMetric);
			}
		}
		if(!isValidJson)
		{
			DO_LOG_ERROR(" Invalid input json parameter or topic.");
			eFunRetType = APP_ERROR_INVALID_INPUT_JSON;
		}

		string strSearchString = "/", stTopic = "";
		std::size_t found = strSourceTopic.find_last_of(strSearchString);
		if (found!=std::string::npos)
		{
			stTopic = strSourceTopic.substr(0, found);
		}
		if(stTopic.empty())
		{
			DO_LOG_ERROR("Topic is not found in request json.");
			eFunRetType = APP_ERROR_INVALID_INPUT_JSON;
		}

		const std::map<std::string, network_info::CUniqueDataPoint>& mpp = network_info::getUniquePointList();
		struct network_info::stModbusAddrInfo addrInfo;
		try
		{
			addrInfo = mpp.at(stTopic).getWellSiteDev().getAddressInfo();
			a_stMbusApiPram.m_i32Ctx = mpp.at(stTopic).getWellSiteDev().getCtxInfo();
		}
		catch(const std::out_of_range& oor)
		{
			DO_LOG_INFO(" Request is not for this application." + std::string(oor.what()));

			return APP_ERROR_UNKNOWN_SERVICE_REQUEST;
		}
		// Next section should be executed only if request is for this container and
		// request is valid
		if(APP_SUCCESS == eFunRetType)
		{
			obj = mpp.at(stTopic).getDataPoint();
#ifdef MODBUS_STACK_TCPIP_ENABLED
			a_stMbusApiPram.m_u8DevId = addrInfo.m_stTCP.m_uiUnitID;
#else
			a_stMbusApiPram.m_u8DevId = addrInfo.m_stRTU.m_uiSlaveId;
#endif
			a_stMbusApiPram.m_stOnDemandReqData.m_isByteSwap = obj.getAddress().m_bIsByteSwap;
			a_stMbusApiPram.m_stOnDemandReqData.m_isWordSwap = obj.getAddress().m_bIsWordSwap;

			a_stMbusApiPram.m_stOnDemandReqData.m_sDataType = obj.getAddress().m_sDataType;
			a_stMbusApiPram.m_stOnDemandReqData.m_dscaleFactor = obj.getAddress().m_dScaleFactor;

			a_stMbusApiPram.m_stOnDemandReqData.m_iWidth = obj.getAddress().m_iWidth;

			a_stMbusApiPram.m_u16StartAddr = (uint16_t)obj.getAddress().m_iAddress;
			a_stMbusApiPram.m_u16Quantity = (uint16_t)obj.getAddress().m_iWidth;

			// Include dataPersist flag's value in struct m_stOnDemandReqData in case of On Demand Request(Read and Write)
			// The same struct m_stOnDemandReqData is used while preparing JSON payload response for Read On Demand and Write On Demand.
			a_stMbusApiPram.m_stOnDemandReqData.m_bIsDataPersist = obj.getDataPersist();
			
			network_info::eEndPointType eType = obj.getAddress().m_eType;

			// Convert back the scaledValue to Hex depending on datatype, width, scalefactor of the specific datapoints

			if (isScaledValue)
			{
				if (false == reverseScaledValueToHex(a_stMbusApiPram.m_stOnDemandReqData.m_sDataType,
												a_stMbusApiPram.m_stOnDemandReqData.m_iWidth,
												a_stMbusApiPram.m_stOnDemandReqData.m_dscaleFactor,
												a_stMbusApiPram.m_stOnDemandReqData.m_ScaledValue,
												strValue))
				 
					{
						DO_LOG_ERROR("Error in reverse conversion of ScaledValue to Hex");
						return APP_ERROR_INVALID_INPUT_JSON;
					}			
 
			}
			/// to find function code of received requestS
			switch(eType)
			{
			case network_info::eEndPointType::eCoil:
				funcCode = a_IsWriteReq ? WRITE_SINGLE_COIL: READ_COIL_STATUS;
				break;
			case network_info::eEndPointType::eHolding_Register:
				funcCode = a_IsWriteReq ? a_stMbusApiPram.m_u16Quantity == 1 ? WRITE_SINGLE_REG: WRITE_MULTIPLE_REG : READ_HOLDING_REG;
				break;
			case network_info::eEndPointType::eInput_Register:
				funcCode = READ_INPUT_REG;
				break;
			case network_info::eEndPointType::eDiscrete_Input:
				funcCode = READ_INPUT_STATUS;
				break;
			default:
				DO_LOG_ERROR(" Invalid type in datapoint:: " + a_stMbusApiPram.m_stOnDemandReqData.m_strMetric);
				break;
			}

			if(a_IsWriteReq && (funcCode == READ_INPUT_REG || funcCode == READ_INPUT_STATUS))
			{
				funcCode = MBUS_MAX_FUN_CODE;
				return APP_ERROR_POINT_IS_NOT_WRITABLE;
			}

			if(WRITE_MULTIPLE_REG == funcCode)
			{
				a_stMbusApiPram.m_u16ByteCount = a_stMbusApiPram.m_u16Quantity*2;
			}
			else if(WRITE_MULTIPLE_COILS == funcCode)
			{
				uint8_t u8ByteCount = (0 != (a_stMbusApiPram.m_u16Quantity%8))
																?((a_stMbusApiPram.m_u16Quantity/8)+1)
																		:(a_stMbusApiPram.m_u16Quantity/8);

				a_stMbusApiPram.m_u16ByteCount = (uint8_t)u8ByteCount;
			}
			else if(WRITE_SINGLE_COIL == funcCode ||
					WRITE_SINGLE_REG == funcCode)
			{
				a_stMbusApiPram.m_u16ByteCount = MODBUS_SINGLE_REGISTER_LENGTH;
			}

			if(WRITE_SINGLE_COIL == funcCode)
			{
				// If value is 0x01, then write 0xFF00
				if( (0 == strValue.compare("0x00")) ||
						(0 == strValue.compare("0X00"))  ||
						(0 == strValue.compare("00")))
				{
					strValue = "0x0000";
				}
				else if( (0 == strValue.compare("0x01")) ||
						(0 == strValue.compare("0X01")) ||
						(0 == strValue.compare("01")))
				{
					strValue = "0xFF00";
				}
				else
				{
					eFunRetType = APP_ERROR_INVALID_INPUT_JSON;
				}
			}
			if(true == a_IsWriteReq && funcCode != WRITE_MULTIPLE_COILS)
			{
				if((true == obj.getAddress().m_bIsByteSwap || true == obj.getAddress().m_bIsWordSwap))
				{
					std::vector<uint8_t> tempVt;
					int i = 0;
					if( ('0' == strValue[0]) && (('X' == strValue[1]) || ('x' == strValue[1])) )
					{
						i = 2;
					}
					int iLen = strValue.length();
					while(i < iLen)
					{
						unsigned char byte1 = char2int(strValue[i])*16 + char2int(strValue[i+1]);
						tempVt.push_back(byte1);
						i = i+2;
					}
					strValue = common_Handler::swapConversion(tempVt,
							!obj.getAddress().m_bIsByteSwap,
							obj.getAddress().m_bIsWordSwap);
				}
				int retVal = hex2bin(strValue, a_stMbusApiPram.m_u16ByteCount, a_stMbusApiPram.m_pu8Data);
				if(-1 == retVal)
				{
					DO_LOG_FATAL("Invalid value in request json.");
					eFunRetType = APP_ERROR_INVALID_INPUT_JSON;
				}
			}
		}
	}
	catch(const std::exception &e)
	{
		eFunRetType = APP_JSON_PARSING_EXCEPTION;
		DO_LOG_FATAL(e.what());
	}

	return eFunRetType;
}

/**
 * Function will get the realtime parameters per operation required for on-demand operation
 * These parameters will be used for individual threads for on-demand operations
 * @param topicPrefix	:[in] topicPrefix for which to retrieve operation info
 * @param operation		:[out] operation info
 * @param vpCallback	:[out] set the stack callback as per the operation
 * @param a_iRetry		:[out] set the retry value to be used for application retry mechanism
 * @param a_lPriority	:[out] set the priority value to be used for stack priority queues
 * @param a_bIsWriteReq	:[out] flag used to distinguish read/write request for further processing
 * @param a_bIsRT		:[out] flag used to distinguish RT/NON-RT request for further processing
 * @return true/false based on success/error
 */
bool onDemandHandler::getOperation(string topicPrefix, globalConfig::COperation& operation,
		void **vpCallback,
		int& a_iRetry,
		long& a_lPriority,
		bool& a_bIsWriteReq,
		bool& a_bIsRT)
{
	bool bRet = true;
	if(regExFun(topicPrefix, READ_REQ))
	{
		operation = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getNonRTConfig();
		*vpCallback = (void *)OnDemandRead_AppCallback;

		// get retry value per operation to be used for retry mechanism
		a_iRetry = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getNonRTConfig().getRetries();

		/// get the priority
		a_lPriority = common_Handler::getReqPriority(
				globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getNonRTConfig());

		// set flag to distinguish read/write request
		a_bIsWriteReq = false;
		a_bIsRT = false;
	}
	else if(regExFun(topicPrefix, READ_REQ_RT))
	{
		operation = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getRTConfig();
		*vpCallback = (void *)OnDemandReadRT_AppCallback;

		// get retry value per operation to be used for retry mechanism
		a_iRetry = globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig().
				getRTConfig().getRetries();

		/// get the priority
		a_lPriority = common_Handler::getReqPriority(
				globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandReadConfig().getRTConfig());

		// set flag to distinguish read/write request
		a_bIsWriteReq = false;

		// set realtime to true
		a_bIsRT = true;
	}
	else if(regExFun(topicPrefix, WRITE_REQ))
	{
		operation = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getNonRTConfig();
		*vpCallback = (void *)OnDemandWrite_AppCallback;

		// get retry value per operation to be used for retry mechanism
		a_iRetry = globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig().
				getNonRTConfig().getRetries();

		/// get the priority
		a_lPriority = common_Handler::getReqPriority(
				globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getNonRTConfig());

		// set flag to distinguish read/write request
		a_bIsWriteReq = true;

		a_bIsRT = false;
	}
	else if(regExFun(topicPrefix, WRITE_REQ_RT))
	{
		operation = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getRTConfig();
		*vpCallback = (void *)OnDemandWriteRT_AppCallback;

		// get retry value per operation to be used for retry mechanism
		a_iRetry = globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getRTConfig().getRetries();

		/// get the priority
		a_lPriority = common_Handler::getReqPriority(
				globalConfig::CGlobalConfig::getInstance().
				getOpOnDemandWriteConfig().getRTConfig());

		// set flag to distinguish read/write request
		a_bIsWriteReq = true;

		// set realtime to true
		a_bIsRT = true;
	}
	else
	{
		DO_LOG_ERROR("Invalid topicPrefix name in config.json");
		bRet = false;
		a_iRetry = 0;
	}

	return bRet;
}

/**
 * Function to start thread to listen on all subTopics for on-demand requests
 */
void onDemandHandler::createOnDemandListener()
{
	bool bIsRT = false;
	void *vpCallback = NULL;
	int iRetry = 0;
	long a_lPriority;
	bool a_bIsWriteReq = false;

	//std::vector<std::string> stTopics = CcommonEnvManager::Instance().getTopicList();
	std::vector<std::string> stTopics;
	bool tempRet = zmq_handler::returnAllTopics("sub", stTopics);
	if(tempRet == false) {
		exit(1);
	} 
	for(std::vector<std::string>::iterator it = stTopics.begin(); it != stTopics.end(); ++it)
	{
		if(it->empty()) {
			DO_LOG_ERROR("SubTopics are not configured");
			continue;
		}
		globalConfig::COperation ops;
		if(!getOperation(*it, ops, &vpCallback, iRetry, a_lPriority, a_bIsWriteReq, bIsRT))
		{
			DO_LOG_ERROR("Invalid topic name in SubTopics..hence ignoring");

			// continue iterating next topic
			continue;
		}
	 
		// create separate thread per topic prefix mentioned in subscriber config section of config.json
		std::thread(&onDemandHandler::subscribeDeviceListener, this, *it, ops,
				bIsRT, vpCallback, iRetry,
				a_lPriority,
				a_bIsWriteReq).detach();
	}
	DO_LOG_DEBUG("End");
}

/**
 * Function to get OnDemand Handler reference.
 * @return Return reference of OnDemand Handler class
 */
onDemandHandler& onDemandHandler::Instance()
{
	static onDemandHandler _self;
	return _self;
}

/**
 * Function to get value from zmq message based on given key
 * @param msg	:	[in] actual message received from ZMQ
 * @param a_sKey:	[in] key to find
 * @return[string]  : on Success return actual value
 * 					: On failure - return empty string
 */
string onDemandHandler::getMsgElement(msg_envelope_t *a_Msg,
		string a_sKey)
{
	msg_envelope_elem_body_t* data = NULL;

	// check for NULL
	if(NULL == a_Msg)
	{
		DO_LOG_ERROR("NULL msg received from ZMQ");
		return "";
	}

	// get the value
	msgbus_ret_t msgRet = msgbus_msg_envelope_get(a_Msg, a_sKey.c_str(), &data);

	if(msgRet != MSG_SUCCESS)
	{
		DO_LOG_ERROR(a_sKey + " key not present in message: ");
	}
	else
	{
#ifdef INSTRUMENTATION_LOG
	DO_LOG_DEBUG(a_sKey +":" +data->body.string);
#endif

		// Since all the parameters are in string format, hence string is returned
		// If any other parameter is added with different data types in JSON payload then return value needs to changed
		if(MSG_ENV_DT_STRING == data->type)
		{
			return data->body.string;
		}
	}
	return "";
}


/**
 * Function to get value from zmq message based on given key
 * @param msg	:	[in] actual message received from ZMQ
 * @param a_sKey:	[in] key to find
 * @param [var_hex] : [in] on Success assign actual value which can be any of variant data types
 * return 			: On failure - return empty string
 */
bool onDemandHandler::getScaledValueElement(msg_envelope_t *a_Msg,
		string a_sKey, var_hex &a_ScaledValue)
{		 
		msg_envelope_elem_body_t* data = NULL;

		// check for NULL
		if(NULL == a_Msg)
		{
			DO_LOG_ERROR("NULL msg received from ZMQ");
			return false;
		}

		// get the value
		msgbus_ret_t msgRet = msgbus_msg_envelope_get(a_Msg, a_sKey.c_str(), &data);

		if(msgRet != MSG_SUCCESS)
		{
			DO_LOG_ERROR(a_sKey + " key not present in message: ");
		}
		else
		{
			if(MSG_ENV_DT_INT == data->type)
			{

#ifdef INSTRUMENTATION_LOG
DO_LOG_DEBUG(a_sKey + ":" + std::to_string(data->body.integer));
#endif				 
				a_ScaledValue = data->body.integer;
				return true;
			}
			else if(MSG_ENV_DT_FLOATING == data->type)
			{

#ifdef INSTRUMENTATION_LOG
DO_LOG_DEBUG(a_sKey + ":" + std::to_string(data->body.floating));
#endif			 
				a_ScaledValue = data->body.floating;
				return true;
			}
			else if(MSG_ENV_DT_STRING == data->type)
			{

#ifdef INSTRUMENTATION_LOG
DO_LOG_DEBUG(a_sKey + ":" + std::string(data->body.string));
#endif			
				a_ScaledValue = std::string(data->body.string);
				return true;
			}
			else if (MSG_ENV_DT_BOOLEAN == data->type)
			{

#ifdef INSTRUMENTATION_LOG
DO_LOG_DEBUG(a_sKey + ":" + std::to_string(data->body.boolean));
#endif
			 
				a_ScaledValue = data->body.boolean;
				return true;
			}
			else if (MSG_ENV_DT_NONE == data->type)
			{

#ifdef INSTRUMENTATION_LOG
DO_LOG_DEBUG(a_sKey + ":" +  "DT_NONE");
#endif
				 
				return false;
			}
			else
			{				 
				return false;
			}
		}
		return false;
}

/**
 * generic function to process message received from ZMQ.
 * @param msg			:[in] actual message received from zmq
 * @param topic			:[in] topic for zmq listening
 * @param a_bIsRT		:[in] flag used to distinguish RT/NON-RT request for further processing
 * @param vpCallback	:[in] set the stack callback as per the operation
 * @param a_iRetry		:[in] set the retry value to be used for application retry mechanism
 * @param a_lPriority	:[in] set the priority value to be used for stack priority queues
 * @param a_bIsWriteReq	:[in] flag used to distinguish read/write request for further processing
 * @return[bool] true: on Success
 * 				 false: On failure
 */
bool onDemandHandler::processMsg(msg_envelope_t *msg,
		std::string stTopic,
		bool a_bIsRT,
		void *vpCallback,
		const int a_iRetry,
		const long a_lPriority,
		const bool a_bIsWriteReq)
{	
	MbusAPI_t stMbusApiPram = {};
	timespec_get(&stMbusApiPram.m_stOnDemandReqData.m_obtReqRcvdTS, TIME_UTC);
	bool bRet = false;
	std::string retScaledVal="";

	if(NULL == msg || NULL == vpCallback)
	{
		DO_LOG_ERROR("NULL pointer received while processing msg. hence discarding the msg");

		if(msg != NULL)
		{
			msgbus_msg_envelope_destroy(msg);
		}
		msg = NULL;
		return false;
	}

#ifdef INSTRUMENTATION_LOG
		DO_LOG_DEBUG("On-demand request received on "+ stTopic + " realtime:: "+ std::to_string(a_bIsRT) + " with following parameters::");
#endif

	stMbusApiPram.m_stOnDemandReqData.m_strAppSeq = getMsgElement(msg, "app_seq");
	stMbusApiPram.m_stOnDemandReqData.m_strMetric = getMsgElement(msg, "command");	
	if (a_bIsWriteReq)
	{
		stMbusApiPram.m_stOnDemandReqData.m_sValue = getMsgElement(msg, "value");
		if (stMbusApiPram.m_stOnDemandReqData.m_sValue.empty())
		{		
			if (false == getScaledValueElement(msg, "scaledValue", stMbusApiPram.m_stOnDemandReqData.m_ScaledValue))
			{
				DO_LOG_ERROR("Invalid scaledValue received from message envelope of mqtt-bridge");
				return false;
			}		 
		}
	}	 
	stMbusApiPram.m_stOnDemandReqData.m_strWellhead = getMsgElement(msg, "wellhead");
	stMbusApiPram.m_stOnDemandReqData.m_strVersion = getMsgElement(msg, "version");
	stMbusApiPram.m_stOnDemandReqData.m_strTopic = getMsgElement(msg, "sourcetopic");
	stMbusApiPram.m_stOnDemandReqData.m_sTimestamp = getMsgElement(msg, "timestamp");
	stMbusApiPram.m_stOnDemandReqData.m_sUsec = getMsgElement(msg, "usec");
	stMbusApiPram.m_stOnDemandReqData.m_strMqttTime = getMsgElement(msg, "tsMsgRcvdFromMQTT");
	stMbusApiPram.m_stOnDemandReqData.m_strEiiTime = getMsgElement(msg, "tsMsgPublishOnEII");
	//In case when sparkplug is connected with EMB. 
	if( stMbusApiPram.m_stOnDemandReqData.m_strMqttTime.empty()){
		stMbusApiPram.m_stOnDemandReqData.m_strMqttTime = getMsgElement(msg, "tsMsgRcvdFromExtMQTT");
	}
	//In case when sparkplug is connected with EMB.
	if( stMbusApiPram.m_stOnDemandReqData.m_strEiiTime.empty()){
		stMbusApiPram.m_stOnDemandReqData.m_strEiiTime = getMsgElement(msg, "tsMsgPublishSPtoEMB");
	}
	stMbusApiPram.m_stOnDemandReqData.m_isRT = a_bIsRT;

	// fill retry and priority used for further processing
	stMbusApiPram.m_nRetry = a_iRetry;
	stMbusApiPram.m_lPriority = a_lPriority;
	
	onDemandInfoHandler(&stMbusApiPram, stTopic, vpCallback, a_bIsWriteReq);

	if(msg != NULL)
	{
		msgbus_msg_envelope_destroy(msg);
	}
	msg = NULL;

	return bRet;
} 

/**
* Thread to listen for on-demand request on ZMQ for all the topics mentioned in SubTopics section
* @param topic			:[in] topic for zmq listening
* @param operation		:[out] operation info used to set thread parameters
* @param a_bIsRT		:[out] flag used to distinguish RT/NON-RT request for further processing
* @param vpCallback	:[out] set the stack callback as per the operation
* @param a_iRetry		:[out] set the retry value to be used for application retry mechanism
* @param a_lPriority	:[out] set the priority value to be used for stack priority queues
* @param a_bIsWriteReq	:[out] flag used to distinguish read/write request for further processing
* @return Nothing
*/
void onDemandHandler::subscribeDeviceListener(const std::string stTopic,
		const globalConfig::COperation a_refOps,
		bool a_bIsRT,
		void *vpCallback,
		const int a_iRetry,
		const long a_lPriority,
		const bool a_bIsWriteReq)
{
	msg_envelope_t *msg = NULL;
	msgbus_ret_t ret = MSG_SUCCESS;

	if(NULL == vpCallback)
	{
		DO_LOG_ERROR("NULL callback value is received..");
		return;
	}

	//set the thread priority
	globalConfig::set_thread_sched_param(a_refOps);

	globalConfig::display_thread_sched_attr(stTopic + " subscribeDeviceListener param :: ");
	try
	{
		zmq_handler::stZmqContext& msgbus_ctx = zmq_handler::getCTX(stTopic);
		zmq_handler::stZmqSubContext& stsub_ctx = zmq_handler::getSubCTX(stTopic);

		while((msgbus_ctx.m_pContext != NULL) && (NULL != stsub_ctx.sub_ctx)
				&& (false == g_stopThread.load()))
		{
			ret = msgbus_recv_wait(msgbus_ctx.m_pContext, stsub_ctx.sub_ctx, &msg);
			if(ret != MSG_SUCCESS)
			{
				DO_LOG_ERROR("Failed to receive message errno ::" + std::to_string(ret));
				continue;
			}
			// process messages
			processMsg(msg, stTopic, a_bIsRT, vpCallback, a_iRetry, a_lPriority, a_bIsWriteReq);
		}
	}
	catch(const std::exception& e)
	{
		DO_LOG_FATAL(e.what());
	}
}

/**
* Function to convert decimal to its corresponding hexadecimal format 
* @param num  :[in] input decimal number which is required to be converted to hexadecimal
* @param width :[in] input width. Width will be used for padding with 0's
* @return string
*/
std::string onDemandHandler::convertToHexString(uint64_t num, uint8_t width)
{
   std::string hexVal {""};
   std::string output {"0x"};
   char zero = '0';
   while(num != 0)
   {
	int temp = 0;
	temp = num & 15;
	hexVal.insert(0, &hexDigits[temp], 1);
	num = num/16;
   }
  // padded with 0's so as to set fill depending on the width of the input number
  while(hexVal.length() < width * 4)
  {
  	hexVal.insert(0, &zero, 1);
  } 
   return "0x" + hexVal;
}
