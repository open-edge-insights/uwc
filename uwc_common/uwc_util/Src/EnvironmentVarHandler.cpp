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
#include "ConfigManager.hpp"
#include "EnvironmentVarHandler.hpp"

/**
 * function to read environment variables
 * @param vecEnvironment :[in] vector containing all environment variables
 * @return : true for success, false otherwise
 */
bool EnvironmentInfo::readCommonEnvVariables(std::vector<std::string> vecEnvironment)
{
	if(vecEnvironment.empty())
	{
		DO_LOG_ERROR("No Environment variable to read.");
		return false;
	}

	bool bRetVal = false;

	for(auto ele : vecEnvironment)
	{
		char *cEvar = getenv(ele.c_str());
		if (NULL != cEvar)
		{
			bRetVal = true;
			std::string strTempVal = std::string(cEvar);
			addDataToEnvMap(ele, strTempVal);
			DO_LOG_DEBUG(std::string(ele) + " environment variable is set to ::" + std::string(cEvar));
		}
		else
		{
			bRetVal = false;
			DO_LOG_ERROR(std::string(ele) + " environment variable is not found");
		}
	}

	return bRetVal;
}

/**
 * function to add environment variable data to Map
 * @param strKey :[in] key value containing the environment variables name
 * @param strVal :[in] value containing the value for each key
 * @return : true for success, false otherwise
 */
bool EnvironmentInfo::addDataToEnvMap(std::string strKey, std::string strVal)
{
	bool bRet = true;
	try
	{
		m_umapEnv.insert({strKey, strVal});
	}
	catch (std::exception &e)
	{
		DO_LOG_FATAL(e.what());
		bRet = false;
	}

	return bRet;
}

/**
 * function to read data from map
 * @param strKey :[in] key string used to get data from map
 * @return : string containing value of given key
 */
std::string EnvironmentInfo::getDataFromEnvMap(std::string strKey)
{
	std::string strVal = "";
	try
	{
		strVal = m_umapEnv.at(strKey);
	}
	catch(std::exception &e)
	{
		DO_LOG_FATAL(LOGDETAILS(e.what()));
	}

	return strVal;
}


