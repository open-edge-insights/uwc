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

#include "YamlUtil.hpp"
#include<iostream>
#include "Logger.hpp"

namespace CommonUtils {

/** This function is used to load YAML file from local files
 *
 * @param filename : This variable is use to pass actual filename to load
 * @return : YAML node
 */
YAML::Node loadYamlFile(const std::string& filename)
{
	std::string sBasePath{BASE_PATH_YAML_FILE};
	std::string sfileToRead = sBasePath + filename;

	DO_LOG_DEBUG("YAML file to be read is :: " + sfileToRead);

	YAML::Node baseNode = YAML::LoadFile(sfileToRead);

	return baseNode;
}

/** This function is used to store YAML data from a node to a string list
 *
 * @param node : [in] YML node
 * @param a_slist : [out] string list
 * @return : true/false based on success or error
 */
bool convertYamlToList(YAML::Node &node, std::vector<std::string>& a_slist)
{
	// locals
	bool bRetVal = false;
	try
	{
		for (auto it : node)
		{
			if(it.second.IsSequence() && it.first.as<std::string>() == "devicegrouplist")
			{
				const YAML::Node& list =  it.second;

				DO_LOG_INFO("Number of sites in yaml file are ::" + list.size());

				for (auto element : list)
				{
					YAML::Node temp = element;
					DO_LOG_INFO("Wellsite is :: " + temp.as<std::string>());
					a_slist.push_back(temp.as<std::string>());
				}
				bRetVal = true;
			}
		}
	}
	catch (YAML::Exception &e)
	{
		DO_LOG_ERROR("Exception is :: " + std::string(e.what()));
		std::cerr << e.what() << "\n";
	}

	return bRetVal;
}

/** This function converts IP string to char array
 *
 * @param strIPaddr : IP string to convert
 * @param ptrIpAddr : char array to store tokenize key
 * @return : Nothing
 */
void ConvertIPStringToCharArray(std::string strIPaddr, unsigned char *ptrIpAddr)
{
	std::string delimiter = ".";
	size_t pos = 0;
	int8_t i8Index = 0;

	while((pos = strIPaddr.find(delimiter))!= std::string::npos)
	{
		ptrIpAddr[i8Index] = (uint8_t)std::stoi(strIPaddr.substr(0,pos).c_str());
		strIPaddr.erase(0,pos+delimiter.length());
		++i8Index;
	}
	ptrIpAddr[i8Index] = (uint8_t)std::stoi(strIPaddr.substr(0,pos).c_str());
}

/**
 * Reads environment variable and stores its value in givenn parameter
 * @param pEnvVarName:[in] Environment variable name
 * @param storeVal:[out] String to store value of environment variable
 * @return true on successfule reading, false otherwise
 */
bool readEnvVariable(const char *pEnvVarName, std::string &storeVal)
{
	if(NULL == pEnvVarName)
	{
		return false;
	}
	bool bRetVal = false;
	char *cEvar = getenv(pEnvVarName);
	if (NULL != cEvar)
	{
		bRetVal = true;
		std::string tmp (cEvar);
		storeVal = tmp;
		DO_LOG_INFO(std::string(pEnvVarName) + " environment variable is set to ::" + storeVal);
	}
	else
	{
		DO_LOG_ERROR(std::string(pEnvVarName) + " environment variable is not found");
	}
	return bRetVal;
}

} /* namespace CommonUtils */
