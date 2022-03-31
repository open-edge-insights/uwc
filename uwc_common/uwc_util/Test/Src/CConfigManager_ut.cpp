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


#include "../include/CConfigManager_ut.hpp"

#include <iostream>
using namespace std;


void CConfigManager_ut::SetUp()
{
	// Setup code
}

void CConfigManager_ut::TearDown()
{
	// TearDown code
}


/**CConfigManager_ut::getInstance() should return Instance of class type CfgManager **/
TEST_F(CConfigManager_ut, getInstance)
{
	stUWCComnDataVal_t UWCDataToLib;
	UWCDataToLib.m_devMode = true;
	UWCDataToLib.m_sAppName = "Modbus_TCP_Master";
	UWCDataToLib.m_isCommonDataInitialised = true;

	CcommonEnvManager::Instance().ShareToLibUwcCmnData(UWCDataToLib);
	std::cout<<"1\n";
	CfgManager::Instance();
	std::cout<<"2\n";
	EXPECT_EQ(typeid(CfgManager), typeid(CfgManager::Instance()));
}

// /**Tests for CfgManager::Instance() client creation **/
// TEST_F(CConfigManager_ut, IsClientCreated_CriticalDataInitialised)
// {
// 	stUWCComnDataVal_t UWCDataToLib;
// 	UWCDataToLib.m_devMode = true;
// 	UWCDataToLib.m_sAppName = "uwc-util";
// 	UWCDataToLib.m_isCommonDataInitialised = true;

// 	CcommonEnvManager::Instance().ShareToLibUwcCmnData(UWCDataToLib);

// 	EXPECT_TRUE(CfgManager::Instance().IsClientCreated());
// }


/**Test for globalConfig::display_sched_attr()**/
TEST_F(CConfigManager_ut, display_sched_attr_test)
{
	struct sched_param param;
	globalConfig::display_sched_attr(2, param);
}


/**Test for globalConfig::display_thread_sched_attr()**/
TEST_F(CConfigManager_ut, display_thread_sched_attr_test)
{
	char msg;
	globalConfig::display_thread_sched_attr(&msg);

	// Please check log for test result
}

/**Test for globalConfig::set_thread_sched_param()**/
TEST_F(CConfigManager_ut, set_thread_param_true)
{
	globalConfig::COperation a_OpsInfo;
	globalConfig::eThreadScheduler a_eSched = globalConfig::threadScheduler::UNKNOWN;
	bool a_bIsOperation = true;
	globalConfig::set_thread_sched_param(a_OpsInfo, 1, a_eSched, a_bIsOperation);
}

/**Test for globalConfig::set_thread_sched_param()**/
TEST_F(CConfigManager_ut, set_thread_param_false)
{
	globalConfig::COperation a_OpsInfo;
	globalConfig::eThreadScheduler a_eSched = globalConfig::threadScheduler::UNKNOWN;
	bool a_bIsOperation = false;
	globalConfig::set_thread_sched_param(a_OpsInfo, 1, a_eSched, a_bIsOperation);
}

/**Test for globalConfig::setDefaultConfig()**/
TEST_F(CConfigManager_ut, setDefaultconfig_polling)
{
	globalConfig::eOperationType a_eOpType = globalConfig::eOperationType::UNKNOWN_OPERATION;
	globalConfig::setDefaultConfig(a_eOpType);
}

/**Test for globalConfig::loadGlobalConfigurations()**/
TEST_F(CConfigManager_ut, loadGlobalConfigurations_GlobalKeyPressent)
{

	EXPECT_EQ( true, globalConfig::loadGlobalConfigurations() );
}

