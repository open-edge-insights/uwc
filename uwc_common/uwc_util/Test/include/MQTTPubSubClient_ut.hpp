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

#ifndef MQTTPUBSUBCLIENT_UT_HPP_
#define MQTTPUBSUBCLIENT_UT_HPP_

#include <gtest/gtest.h>
#include "MQTTPubSubClient.hpp"
#include "Logger.hpp"
#include "CommonDataShare.hpp"
#include "EnvironmentVarHandler.hpp"
#define SUBSCRIBER_ID "_KPI_SUBSCRIBER"

class MQTTPubSubClient_ut : public::testing::Test
{
protected:
	virtual void SetUp();
	virtual void TearDown();

public:
	std::string strPlBusUrl = "";
	std::string ClientID = "";
		int iQOS = 1;
		CMQTTBaseHandler  CMQTTBaseHandler_obj{strPlBusUrl, EnvironmentInfo::getInstance().getDataFromEnvMap("AppName")+SUBSCRIBER_ID,
			iQOS, (false == CcommonEnvManager::Instance().getDevMode()),
			"/run/secrets/rootca/cacert.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem",
			"/run/secrets/mymqttcerts/mymqttcerts_client_key.pem", "MQTTSubListener"};


		CMQTTPubSubClient CMQTTPubSubClient_obj{strPlBusUrl, ClientID,
				iQOS,
				(false == CcommonEnvManager::Instance().getDevMode()), "/run/secrets/rootca/cacert.pem",
				"/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_key.pem",
				"MQTTSubListener"};
		CMQTTPubSubClient CMQTTPubSubClient_obj2{strPlBusUrl, ClientID,
						iQOS,
						true, "/run/secrets/rootca/cacert.pem",
						"/run/secrets/mymqttcerts/mymqttcerts_client_certificate.pem", "/run/secrets/mymqttcerts/mymqttcerts_client_key.pem","MQTTSubListener"};



};



#endif /* MQTTPUBSUBCLIENT_UT_HPP_ */
