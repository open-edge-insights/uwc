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

#include "ConfigManager.hpp"
#include "Common.hpp"
#include <cjson/cJSON.h>
#include <algorithm>
#include "EnvironmentVarHandler.hpp"

/**
 * Constructor initializes CCommon instance and retrieves common environment variables
 * @param None
 * @return None if successful;
 * 			In case of error or exception, application exits
 */
CCommon::CCommon()
{
	if(false == EnvironmentInfo::getInstance().readCommonEnvVariables(m_vecEnv))
	{
		DO_LOG_ERROR("Error while reading the common environment variables");
		exit(1);
	}
	std::string strDevMode = EnvironmentInfo::getInstance().getDataFromEnvMap("DEV_MODE");
	transform(strDevMode.begin(), strDevMode.end(), strDevMode.begin(), ::toupper);

	std::string strAppName = EnvironmentInfo::getInstance().getDataFromEnvMap("AppName");
	if(strAppName.empty())
	{
		DO_LOG_ERROR("Error :: AppName Environment Variable is not set");
		exit(1);
	}
	initializeCommonData(strDevMode, strAppName);
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
void CCommon::initializeCommonData(std::string strDevMode, std::string strAppName)
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
 * Destructor
 */
CCommon::~CCommon()
{
}

/**
 * Get current epoch time in string
 * @param strCurTime :[out] set current epoch time in string format
 * @return None
 */
void CCommon::getCurrentTimestampsInString(std::string &strCurTime)
{
	try
	{
		struct timespec tsMsgReceived;
		timespec_get(&tsMsgReceived, TIME_UTC);
		strCurTime = std::to_string(get_micros(tsMsgReceived));
	}
	catch(std::exception &e)
	{
		DO_LOG_FATAL("Cannot get current time in string :: " +
				std::string(e.what()));
	}
}

/**
 * Add current time stamp in message payload
 * @param a_sMsg 		:[in] message in which to add time
 * @param tsKey			:[in] key name against which to add time stamp
 * @param strTimestamp	:[in] time stamp in string format
 * @return true/false based on success/failure
 */
bool CCommon::addTimestampsToMsg(std::string &a_sMsg, std::string tsKey, std::string strTimestamp)
{
	cJSON *root = NULL;
	try
	{
		root = cJSON_Parse(a_sMsg.c_str());
		if (NULL == root)
		{
			DO_LOG_ERROR("ZMQ Message could not be parsed in json format");
			return false;
		}

		if(NULL == cJSON_AddStringToObject(root, tsKey.c_str(), strTimestamp.c_str()))
		{
			DO_LOG_ERROR("Could not add timestamp in message");
			if(root != NULL)
			{
				cJSON_Delete(root);
			}
			return false;
		}

		a_sMsg.clear();
		char *psNewJson = cJSON_Print(root);
		if(NULL != psNewJson)
		{
			a_sMsg.assign(psNewJson);
			free(psNewJson);
			psNewJson = NULL;
		}

		if(root != NULL)
		{
			cJSON_Delete(root);
		}

		DO_LOG_DEBUG("Added timestamp in payload for MQTT");
		return true;

	}
	catch (std::exception &ex)
	{
		DO_LOG_DEBUG("Failed to add timestamp in payload for MQTT" + std::string(ex.what()));

		if(root != NULL)
		{
			cJSON_Delete(root);
		}

		return false;
	}
}
