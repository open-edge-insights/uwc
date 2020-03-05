/************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation. Title to the
* Material remains with Intel Corporation.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
************************************************************************************/


#include "../include/Logger_ut.hpp"


void Logger_ut::SetUp()
{
	// Setup code
}

void Logger_ut::TearDown()
{
	// TearDown code
}

/*********In Logger functions there is nothing to check through the unit test cases
 therefore functions are just only called in the unit test cases for the seck of coverage of uncovered
 functions from Logger.cpp file.

 *********/


TEST_F(Logger_ut, logINFO)
{
	CLogger::getInstance().LogInfo(Infomsg);
}

TEST_F(Logger_ut, logWAR1)
{
	//CLogger::getInstance().LogWarn(Infomsg);
	CLogger::getInstance().log(WARN, "test");
	CLogger::getInstance().log((LogLevel)8, "test");
}

TEST_F(Logger_ut, logDEBUG)
{
	CLogger::getInstance().LogDebug(Debugmsg);
}

TEST_F(Logger_ut, logWARN)
{
	CLogger::getInstance().LogWarn(Warnmsg);
}

TEST_F(Logger_ut, logFATAL)
{
	CLogger::getInstance().LogFatal(Fatalmsg);
}

TEST_F(Logger_ut, logERROR)
{
	CLogger::getInstance().LogError(Errormsg);
}

