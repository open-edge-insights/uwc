/************************************************************************************
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation. Title to the
 * Material remains with Intel Corporation.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery of
 * the Materials, either expressly, by implication, inducement, estoppel or otherwise.
 ************************************************************************************/

#include "../Inc/Common_ut.hpp"

void Common_ut::SetUp()
{
	// Setup code
}

void Common_ut::TearDown()
{
	// TearDown code
}

#if 0 //In progr5ess
TEST_F(Common_ut, readCommonEnvVariables_DevModeIsTRUE)
{
	CCommon::getInstance().readCommonEnvVariables();

	EXPECT_EQ(true, CCommon::getInstance().isDevMode());
}
#endif
