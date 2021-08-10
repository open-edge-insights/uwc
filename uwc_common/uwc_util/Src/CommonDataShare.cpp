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
#include "CommonDataShare.hpp"
#include <chrono>

/** Constructor
 */
CcommonEnvManager::CcommonEnvManager()
{
}

/** Returns the single instance of this class
 *
 * @param  : nothing
 * @return : object of this class
 */
CcommonEnvManager& CcommonEnvManager::Instance()
{
	static CcommonEnvManager _self;
	return _self;;
}

/**
 * split string with given delimeter
 * @param str :[in] string to split
 * @param delim :[in] delimeter by which string needs to spit
 * @return
 */
void CcommonEnvManager::splitString(const std::string &str, char delim)
{
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		std::size_t pos = item.find('/');
		item = (item.substr(pos + 1));
		DO_LOG_INFO(" Topic:: " + std::string(item));
		addTopicToList(std::string(item));
	}
}

/**
 * Get time parameters
 * @param a_sTimeStamp	:[in] reference to store time stamp
 * @param a_sUsec		:[in] reference to store time in usec
 */
void CcommonEnvManager::getTimeParams(std::string &a_sTimeStamp, std::string &a_sUsec)
{
	a_sTimeStamp.clear();
	a_sUsec.clear();

	const auto p1 = std::chrono::system_clock::now();

	std::time_t rawtime = std::chrono::system_clock::to_time_t(p1);
	std::tm* timeinfo = std::gmtime(&rawtime);
	if(NULL == timeinfo)
	{
		return;
	}
	char buffer [80];

	std::strftime(buffer,80,"%Y-%m-%d %H:%M:%S",timeinfo);
	a_sTimeStamp.insert(0, buffer);

	{
		std::stringstream ss;
		ss << std::chrono::duration_cast<std::chrono::microseconds>(p1.time_since_epoch()).count();
		a_sUsec.insert(0, ss.str());
	}
}
