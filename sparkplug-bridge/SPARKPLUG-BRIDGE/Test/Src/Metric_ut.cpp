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

#include "../Inc/Metric_ut.hpp"

void Metric_ut::SetUp()
{
	// Setup code
}

void Metric_ut::TearDown()
{
	// TearDown code
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_BOOLEAN,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_BOOLEAN)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_BOOLEAN, CValObj_ins);

	B_Res = CValObj_main.assignToSparkPlug(a_metric);
	EXPECT_EQ( false, B_Res );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT16,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT16)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT16, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT8,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT8)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT8, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT8,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT8)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT8, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT16,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT16)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT16, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT32,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT32)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT32, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT32,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT32)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT32, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT64,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_UINT64)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_UINT64, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT64,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_INT64)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_INT64, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_BOOLEAN,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_FLOAT)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_FLOAT, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_BOOLEAN,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_DOUBLE)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_DOUBLE, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToSparkPlug() behaviour when m_uiDataType is METRIC_DATA_TYPE_STRING,
 * and invalide m_objVal
 * @param :[in] None
 * @param :[out] None
 * @return None
 */
TEST_F(Metric_ut, assignToSparkPlug_METRIC_DATA_TYPE_STRING)
{
	CValObj_main.assignNewDataTypeValue(METRIC_DATA_TYPE_STRING, CValObj_ins);
	EXPECT_EQ( false, CValObj_main.assignToSparkPlug(a_metric) );
}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_BOOLEAN,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_BOOLEAN)
{
	cJSON *root =  cJSON_CreateObject();
	std::string keyName = "key";
	var_t a_objVal{true};
	CValObj CValObj_obj{METRIC_DATA_TYPE_BOOLEAN, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(true, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT8,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_UINT8)
{
	cJSON *root =  cJSON_CreateObject();
	std::string keyName = "1234";
	var_t a_objVal{1};
	CValObj CValObj_obj{METRIC_DATA_TYPE_UINT8, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT16,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_UINT16)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{123};
	std::string keyName = "1234";
	CValObj CValObj_obj{METRIC_DATA_TYPE_UINT16, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT8,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_INT8)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{8};
	std::string keyName = "1000";
	CValObj CValObj_obj{METRIC_DATA_TYPE_INT8, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT16,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_INT16)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{166};
	std::string keyName = "1000";
	CValObj CValObj_obj{METRIC_DATA_TYPE_INT16, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT32,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_INT32)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{3233};
	std::string keyName = "400";
	CValObj CValObj_obj{METRIC_DATA_TYPE_INT32, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(true, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT32,
 * and invalide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_UINT32)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{3233};
	std::string keyName = "300";
	CValObj CValObj_obj{METRIC_DATA_TYPE_UINT32, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_UINT64,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_UINT64)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{344};
	std::string keyName = "";
	CValObj CValObj_obj{METRIC_DATA_TYPE_UINT64, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);

}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_INT64,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_INT64)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{54443};
	std::string keyName = "";
	CValObj CValObj_obj{METRIC_DATA_TYPE_INT64, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);
}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_FLOAT,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_FLOAT)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{5.4443};
	std::string keyName = "2000";
	CValObj CValObj_obj{METRIC_DATA_TYPE_FLOAT, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);
}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_DOUBLE,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_DOUBLE)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{54443};
	std::string keyName = "";
	CValObj CValObj_obj{METRIC_DATA_TYPE_DOUBLE, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);
}

/**
 * Test case to check assignToCJSON() behaviour when m_uiDataType is METRIC_DATA_TYPE_STRING,
 * and valide m_objVal
 * @return :bool
 */
TEST_F(Metric_ut, assignTo_json_METRIC_DATA_TYPE_STRING)
{
	cJSON *root =  cJSON_CreateObject();
	var_t a_objVal{"54443"};
	std::string keyName = "10000";
	CValObj CValObj_obj{METRIC_DATA_TYPE_STRING, a_objVal};
	bool result = CValObj_obj.assignToCJSON(root, keyName);
    EXPECT_EQ(false, result);
}

/**
 * Test case to check addMetricNameValue() behaviour
 * @return :bool
 */
TEST_F(Metric_ut, Add_MEtricNameValue)
{

	bool result = CMetric_obj.addMetricNameValue(a_metric);
	EXPECT_EQ(true, result);
}

/**
 * Test case to check addMetricForBirth() behavior
 * @return :bool
 */
TEST_F(Metric_ut, Add_MetricFor_Birth)
{
	bool result = CMetric_obj.addMetricForBirth(a_metric);
	EXPECT_EQ(true, result);
}


/**
 * Test case to check addMetricForBirth() behavior
 * @return :bool
 * bool CUDT::assignToCJSON(cJSON *a_cjMetric)
 *
 **/
TEST_F(Metric_ut, assignToCJSON)
{
	_assignToCJSON();
}



/**
 * Test case to check setValObj for CUDT
 * @return :bool
 *
 **/
TEST_F(Metric_ut, setValObjCUDT)
{


	org_eclipse_tahu_protobuf_Payload_Metric a_metric = { "TEST", false, 0, true, get_current_timestamp(), true,
			METRIC_DATA_TYPE_TEMPLATE, false, 0, false, 0, false,
												true, false,
									org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
												false,
														org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
												0,
												{ 0 } };


	org_eclipse_tahu_protobuf_Payload_Metric tmpMetric = { "TEST", false, 0, true, get_current_timestamp(), true,
				METRIC_DATA_TYPE_TEMPLATE, false, 0, false, 0, false,
													true, false,
										org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
													false,
    org_eclipse_tahu_protobuf_Payload_PropertySet_init_default, 0, {0}};

	std::string name{"test"};
	std::string version{"1.0"};
	a_metric.value.template_value.version = const_cast<char*>(version.c_str());
	a_metric.value.template_value.metrics_count = 1;
	a_metric.value.template_value.metrics = &tmpMetric;
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = false;
	CValObj obj;
	obj.initTestData(dtype, objVal);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric = std::make_shared<CMetric>(a_sName, obj, a_timestamp);
	metricMapIf_t mapMetrics;
	mapMetrics.emplace(a_sName, pCMetric);
	CUDT oUDT(a_sName, dtype, false, a_version);
	oUDT.initTestData(mapMetrics, mapMetrics, a_sUDTDefName, a_version, a_bIsDefinition);
	cJSON *cjRoot = NULL;
	try
		{
		   cjRoot = cJSON_CreateObject();
			if (NULL == cjRoot)
			{
				DO_LOG_ERROR(
							"Message received from MQTT could not be parsed in json format");
			}

		 }
	 catch (const std::exception &e)
		{
				DO_LOG_ERROR(std::string("Error:") + e.what());
		}

	 oUDT.setValObj(a_metric);
}


/**
 * Test case to check addMetricNameValue
 * @return : bool
 *
 **/
TEST_F(Metric_ut, addMetricNameValue)
{
	    org_eclipse_tahu_protobuf_Payload_Metric a_metric = { "test", false, 0, true, get_current_timestamp(), true,
				METRIC_DATA_TYPE_TEMPLATE, false, 0, false, 0, false,
													true, false,
										org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
													false,
															org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
													0,
													{ 0 } };

		org_eclipse_tahu_protobuf_Payload_Metric tmpMetric = { "test", false, 0, true, get_current_timestamp(), true,
					METRIC_DATA_TYPE_TEMPLATE, false, 0, false, 0, false,
														true, false,
											org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
														false,
																org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
														0,
														{ 0 } };

		std::string version{"1.0"};
		a_metric.value.template_value.version = const_cast<char*>(version.c_str());
		a_metric.value.template_value.metrics_count = 1;
		a_metric.value.template_value.metrics = &tmpMetric;
		uint32_t dtype = METRIC_DATA_TYPE_STRING;
		std::string objVal= "xyz";
		std::string a_version = "1.0";
		bool a_bIsDefinition = false;
		CValObj obj;
		obj.initTestData(dtype, objVal);
		std::string a_sName = "Properties/Version";
		const uint64_t a_timestamp = 1486144502122;
		std::string a_sUDTDefName ="UDT";
		std::shared_ptr<CIfMetric> pCMetric = std::make_shared<CMetric>(a_sName, obj, a_timestamp);
		metricMapIf_t mapMetrics;
		mapMetrics.emplace(a_sName, pCMetric);
		CUDT oUDT(a_sName, dtype, false, a_version);
		oUDT.initTestData(mapMetrics, mapMetrics, a_sUDTDefName, a_version, a_bIsDefinition);
		bool res = oUDT.addMetricNameValue(a_metric);
		EXPECT_EQ(true, res);
}

/**
 * Test case to compareValue
 * @return : bool
 *
 **/
TEST_F(Metric_ut, compareValue)
{
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal1= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = false;
	CValObj obj1;
	obj1.initTestData(dtype, objVal1);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric1 = std::make_shared<CMetric>(a_sName, obj1, a_timestamp);
	metricMapIf_t mapMetrics1;
	mapMetrics1.emplace(a_sName, pCMetric1);
	CUDT oUDT1(a_sName, dtype, false, a_version);
	oUDT1.initTestData(mapMetrics1, mapMetrics1, a_sUDTDefName, a_version, a_bIsDefinition);

	dtype = METRIC_DATA_TYPE_FLOAT;
	float objVal2= 45.23;
	std::string a_version2 = "1.0";
	a_bIsDefinition = true;
	CValObj obj;
	obj.initTestData(dtype, objVal2);
	a_sName = "Properties/Version";
	const uint64_t a_timestamp1 = 1486144502122;
	a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric2 = std::make_shared<CMetric>(a_sName, obj, a_timestamp1);
	metricMapIf_t mapMetrics2;
	mapMetrics2.emplace(a_sName, pCMetric2);
	CUDT oUDT2(a_sName, dtype, false, a_version);
	oUDT2.initTestData(mapMetrics2, mapMetrics2, a_sUDTDefName, a_version2, a_bIsDefinition);
	uint8_t res = oUDT1.compareValue(oUDT2);
	EXPECT_EQ(0, res);

}

/**
 * Test case to check assignNewValue
 * @return : bool
 *
 **/
TEST_F(Metric_ut, assignNewValue)
{
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal1= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = false;
	CValObj obj1;
	obj1.initTestData(dtype, objVal1);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric1 = std::make_shared<CMetric>(a_sName, obj1, a_timestamp);
	metricMapIf_t mapMetrics1;
	mapMetrics1.emplace(a_sName, pCMetric1);
	CUDT oUDT1(a_sName, dtype, false, a_version);
	oUDT1.initTestData(mapMetrics1, mapMetrics1, a_sUDTDefName, a_version, a_bIsDefinition);

	dtype = METRIC_DATA_TYPE_STRING;
	objVal1= "abc";
	std::string a_version2 = "1.0";
	a_bIsDefinition = true;
	CValObj obj;
	obj.initTestData(dtype, objVal1);
	a_sName = "Properties/Version";
	const uint64_t a_timestamp1 = 1486144502122;
	a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric2 = std::make_shared<CMetric>(a_sName, obj, a_timestamp1);
	metricMapIf_t mapMetrics2;
	mapMetrics2.emplace(a_sName, pCMetric2);
	CUDT oUDT2(a_sName, dtype, false, a_version);
	oUDT2.initTestData(mapMetrics2, mapMetrics2, a_sUDTDefName, a_version2, a_bIsDefinition);
	uint8_t res = oUDT1.assignNewValue(oUDT2);
	EXPECT_EQ(0, res);
}


/**
 * Test case to addMetricForBirth
 * @return : bool
 **/
TEST_F(Metric_ut, addMetricForBirth)
{
	org_eclipse_tahu_protobuf_Payload_Metric a_metric;
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal1= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = false;
	CValObj obj1;
	obj1.initTestData(dtype, objVal1);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric1 = std::make_shared<CMetric>(a_sName, obj1, a_timestamp);
	metricMapIf_t mapMetrics1;
	mapMetrics1.emplace(a_sName, pCMetric1);
	CUDT oUDT1(a_sName, dtype, false, a_version);
	oUDT1.initTestData(mapMetrics1, mapMetrics1, a_sUDTDefName, a_version, a_bIsDefinition);
    oUDT1.addMetricForBirth(a_metric);
}


/**
 * Test case to compareMetrics
 * @return : bool
 * bool CUDT::compareMetrics(const std::shared_ptr<CIfMetric> &a_pUDT)
 **/
TEST_F(Metric_ut, compareMetrics)
{
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal1= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = true;
	CValObj obj1;
	obj1.initTestData(dtype, objVal1);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric1 = std::make_shared<CMetric>(a_sName, obj1, a_timestamp);
	metricMapIf_t mapMetrics1;
	mapMetrics1.emplace(a_sName, pCMetric1);
	CUDT oUDT1(a_sName, dtype, false, a_version);
	oUDT1.initTestData(mapMetrics1, mapMetrics1, a_sUDTDefName, a_version, a_bIsDefinition);
	try{
	dtype = METRIC_DATA_TYPE_STRING;
	objVal1= "abc";
	std::string a_version2 = "1.0";
	a_bIsDefinition = true;
	bool a_bIsDefinition_1 = true;
	CValObj obj;
	obj.initTestData(dtype, objVal1);
	a_sName = "Properties/Version";
	const uint64_t a_timestamp1 = 1486144502122;
	a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric2 = std::make_shared<CMetric>(a_sName, obj, a_timestamp1);
	metricMapIf_t mapMetrics2;
	mapMetrics2.emplace(a_sName, pCMetric2);
	uint8_t res = 1;
	std::shared_ptr<CIfMetric> pUDT= std::make_shared<CUDT>(a_sName, dtype, false, a_version);
	if( pUDT == nullptr )
	{
		fprintf(stderr,"Buffer memory is not allocated");
		res = 1;
	}else{	
		pUDT->initTestData(mapMetrics2, mapMetrics2, a_sUDTDefName, a_version2, a_bIsDefinition_1);
		res = oUDT1.compareMetrics(pUDT);
	}
	EXPECT_EQ(0, res);
	}catch(std::exception &ex)
	{
		DO_LOG_ERROR(std::string("Error:") + ex.what());
	}
}



/**
 * Test case to validate
 * @return : bool
 * bool CUDT::validate
 **/
TEST_F(Metric_ut, validate)
{
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string objVal1= "xyz";
	std::string a_version = "1.0";
	bool a_bIsDefinition = false;
	CValObj obj1;
	obj1.initTestData(dtype, objVal1);
	std::string a_sName = "Properties/Version";
	const uint64_t a_timestamp = 1486144502122;
	std::string a_sUDTDefName ="UDT";
	std::shared_ptr<CIfMetric> pCMetric1 = std::make_shared<CMetric>(a_sName, obj1, a_timestamp);
	metricMapIf_t mapMetrics1;
	mapMetrics1.emplace(a_sName, pCMetric1);
	CUDT oUDT1(a_sName, dtype, false, a_version);
	dtype = METRIC_DATA_TYPE_STRING;
	objVal1= "abc";
	std::string a_version2 = "1.0";
	a_bIsDefinition = false;
	bool a_bIsDefinition_1 = false;
	CValObj obj;
	obj.initTestData(dtype, objVal1);
	a_sName = "Properties/Version";
	const uint64_t a_timestamp1 = 1486144502122;
	a_sUDTDefName ="UDT";
	try{
	std::shared_ptr<CIfMetric> pCMetric2 = std::make_shared<CMetric>(a_sName, obj, a_timestamp1);
	metricMapIf_t mapMetrics2;
	mapMetrics2.emplace(a_sName, pCMetric2);
	std::shared_ptr<CUDT> pUDT= std::make_shared<CUDT>(a_sName, dtype, false, a_version);
	bool res = false;	
	if( pUDT == nullptr )
	{
	    fprintf(stderr,"Buffer memory is not allocated");
	    res = false;
	}else{	
		pUDT->initTestData(mapMetrics2, mapMetrics2, a_sUDTDefName, a_version2, a_bIsDefinition_1);
		oUDT1.initTestData(mapMetrics1, mapMetrics1, a_sUDTDefName, a_version, a_bIsDefinition);
		oUDT1.initTestRef(pUDT);
		res = oUDT1.validate();
	}
	EXPECT_EQ(true, res);
	}catch(std::exception &ex)
	{
		DO_LOG_ERROR(std::string("Error:") + ex.what());
	}
}


/**
 * Test case to validate
 * @return : bool
 * bool CUDT::validate
 **/
TEST_F(Metric_ut, setValObjTestDataType)
{
	org_eclipse_tahu_protobuf_Payload_Metric a_metric = { "test", false, 0, true, get_current_timestamp(), true,
					METRIC_DATA_TYPE_STRING, false, 0, false, 0, false,
														true, false,
											org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
														false,
																org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
														0,
														{ 0 } };

	org_eclipse_tahu_protobuf_Payload_Metric tmpMetric = { "test", false, 0, true, get_current_timestamp(), true,
						METRIC_DATA_TYPE_STRING, false, 0, false, 0, false,
															true, false,
												org_eclipse_tahu_protobuf_Payload_MetaData_init_default,
															false,
																	org_eclipse_tahu_protobuf_Payload_PropertySet_init_default,
															0,
															{ 0 } };

	std::string version{"1.0"};
	a_metric.value.template_value.version = const_cast<char*>(version.c_str());
	a_metric.value.template_value.metrics_count = 1;
	a_metric.value.template_value.metrics = &tmpMetric;
	_setValObjTestDataType(a_metric);

}

/*
 * Test Case to validate assignToSparkPlug
 * @return
 */

TEST_F(Metric_ut, assignToSparkPlugTemplateParameter)
{
	org_eclipse_tahu_protobuf_Payload_Template_Parameter a_param;
	_assignToSparkPlug(a_param);
}


/*
 * Test Case to CUDT::readUDTRefData
 * @return: bool
 */
TEST_F(Metric_ut, readUDTRefData)
{
	cJSON *cjudt = cJSON_CreateObject();

	if (NULL == cjudt)
	{
		DO_LOG_ERROR("Failed to create Json Object");
		return;
	}

	std::string name = "custom_udt";
	std::string version = "1.0";
	cJSON_AddItemToObject(cjudt, "name", cJSON_CreateString(name.c_str()));
	cJSON_AddItemToObject(cjudt, "version", cJSON_CreateString(version.c_str()));

	cJSON *cjroot = cJSON_CreateObject();
	if (NULL == cjroot)
	{
		return;
	}

	cJSON_AddItemToObject(cjroot, "udt_ref", cjudt);

	_readUDTRefData(cjroot);
}

/*
 * Test Case to test CUDT::processMetric
 * return :
 */

TEST_F(Metric_ut, processMetric)
{

	cJSON *cjudt = cJSON_CreateObject();
	if (NULL == cjudt)
	{
		DO_LOG_ERROR("CJSON Object create failed");
		return;
	}
	std::string name = "Properties/Version";
	std::string datatype = "String";
	std::string value = "2.0.0.1";
	uint64_t timestamp = 1486144502122;
	cJSON_AddItemToObject(cjudt, "name", cJSON_CreateString(name.c_str()));
	cJSON_AddItemToObject(cjudt, "datatype", cJSON_CreateString(datatype.c_str()));
	cJSON_AddItemToObject(cjudt, "value", cJSON_CreateString(value.c_str()));
	cJSON_AddItemToObject(cjudt, "number", cJSON_CreateNumber(timestamp));
	uint32_t dtype = METRIC_DATA_TYPE_STRING;
	std::string a_sName = "Properties/Version";
	std::string a_version = "1.0";
	CUDT oUDT(a_sName, dtype, false, a_version);
	oUDT.processMetric(cjudt);
}















