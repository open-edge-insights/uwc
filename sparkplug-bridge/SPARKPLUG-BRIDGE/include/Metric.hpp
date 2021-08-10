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

/*** Metric.hpp maintains spark plug information*/

#ifndef METRIC_HPP_
#define METRIC_HPP_

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <typeinfo>
#include <map>
#include <functional>
#include "Logger.hpp"
#include "NetworkInfo.hpp"

extern "C"
{
#include "cjson/cJSON.h"
}

#include <tahu.pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <inttypes.h>

extern "C"
{
	#include <tahu.h>
}

#define SAMEVALUE_OR_DTATYPE  (0)
#define DATATYPE_DIFFERENT (1)
#define VALUES_DIFFERENT   (2)
#define NO_CHANGE_IN_VALUE (3)
#define VALUE_ASSINED      (4)
#define SUBDEV_SEPARATOR_CHAR ("-")

using var_t = std::variant<std::monostate, bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double, std::string>;/**Type alias for metric value*/
using var_metric_ref_t = std::variant<std::monostate, std::reference_wrapper<const network_info::CUniqueDataPoint>>; /**Type alias for metric value*/

/** class for value object*/
class CValObj
{
	friend class CMetric; //friend class
	uint32_t m_uiDataType; /** data type*/
	var_t m_objVal; /** object value*/

	bool setValObj(std::string a_sDatatype, cJSON *a_cjValue);
	bool setValObj(org_eclipse_tahu_protobuf_Payload_Metric& a_metric);

public:
	/**default constructor*/
	CValObj() :
			m_uiDataType{0}, m_objVal{}
	{
	}

	CValObj(uint32_t a_uiDataType, var_t a_objVal) :
			m_uiDataType{ a_uiDataType }, m_objVal{ a_objVal }
	{
	}

	CValObj(const CValObj &a_obj) :
			m_uiDataType{ a_obj.m_uiDataType }, m_objVal{ a_obj.m_objVal }
	{
	}

	CValObj(std::string a_sVal) : m_uiDataType{METRIC_DATA_TYPE_STRING}, m_objVal{a_sVal}
	{
	}

	CValObj& operator=(const CValObj &a_obj);

	/** Function  to compare the data types*/
	uint8_t compareDataType(const CValObj &a_obj) const
	{
		if (a_obj.m_uiDataType == m_uiDataType)
		{
			/** Datatypes are equal*/
			return SAMEVALUE_OR_DTATYPE;
		}
		/** Datatypes are different*/
		return DATATYPE_DIFFERENT;
	}

	/** Function compares values*/
	uint8_t compareValue(const CValObj &a_obj) const
	{
		if (SAMEVALUE_OR_DTATYPE == compareDataType(a_obj))
		{
			/** Datatypes are equal*/
			if (a_obj.m_objVal == m_objVal)
			{
				// Values are same
				return SAMEVALUE_OR_DTATYPE;
			}
			// Values are different
			return VALUES_DIFFERENT;
		}
		// Datatypes are different
		return DATATYPE_DIFFERENT;
	}

	/** Function assigns value*/
	uint8_t assignValue(const CValObj &a_obj)
	{
		uint8_t uiRetVal = compareValue(a_obj);
		if (SAMEVALUE_OR_DTATYPE == uiRetVal)
		{
			return NO_CHANGE_IN_VALUE;
		}
		else if (VALUES_DIFFERENT == uiRetVal)
		{
			// assign the value
			m_objVal = a_obj.m_objVal;
			return VALUE_ASSINED;
		}
		// Datatypes are different
		return DATATYPE_DIFFERENT;
	}

	/**Function assigns new data type to value */
	void assignNewDataTypeValue(uint32_t a_uiDataType, const CValObj &a_obj) 
	{
		m_uiDataType = a_uiDataType;
		m_objVal = a_obj.m_objVal;
	}

	/** Function to read data type*/
	uint32_t getDataType() const
	{
		return m_uiDataType;
	}

    /*Function to read value */
	var_t& getValue()
	{
		return m_objVal;
	}

	/*Function to add value data to a Sparkplug metric */
	bool assignToSparkPlug(org_eclipse_tahu_protobuf_Payload_Metric &a_metric) const;
	/*Function to add value data to a Sparkplug parameter */
	bool assignToSparkPlug(org_eclipse_tahu_protobuf_Payload_Template_Parameter &a_param) const;

	/*Function to add value data to a CJSON object */
	bool assignToCJSON(cJSON *a_cjMetric, const std::string &a_sKeyName) const;

	/** function to print*/
	void print() const 
	{
		DO_LOG_INFO(" DataType: " + std::to_string(m_uiDataType));
		std::visit([](auto &&arg)
		{
			//std::monostate, bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double, std::string	
			using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
            	DO_LOG_INFO("bool with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, uint8_t>)
				DO_LOG_INFO("uint8_t with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, uint16_t>)
				DO_LOG_INFO("uint16_t with value" + std::to_string(arg));
			if constexpr (std::is_same_v<T, uint32_t>)
				DO_LOG_INFO("uint32_t with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, uint64_t>)
				DO_LOG_INFO("uint64_t with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, int8_t>)
				DO_LOG_INFO("int8_t with value  " + std::to_string(arg));
			if constexpr (std::is_same_v<T, int16_t>)
				DO_LOG_INFO("int16_t with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, int32_t>)
				DO_LOG_INFO("int32_t with value" + std::to_string(arg));
			if constexpr (std::is_same_v<T, int64_t>)
				DO_LOG_INFO("int64_t with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, float>)
				DO_LOG_INFO("float with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, double>)
				DO_LOG_INFO("double with value " + std::to_string(arg));
			if constexpr (std::is_same_v<T, std::string>)
				DO_LOG_INFO("string with value  " + arg);
		},
		m_objVal);
	}

#ifdef UNIT_TEST
	friend class CMetric_ut;
	void initTestData(uint32_t dataType, var_t objVal)
	{
		m_uiDataType = dataType;
		m_objVal	= objVal;
	}

	void callsetValObj(org_eclipse_tahu_protobuf_Payload_Metric& a_metric)
	{
		setValObj(a_metric);
	}
#endif

};
#ifdef UNIT_TEST
class CIfMetric;
class CUDT;
/** Map: Key = Metric name, Value = Metric reference*/
using metricMapIf_t = std::map<std::string, std::shared_ptr<CIfMetric>>; /** map with key as string and value as pointer to  CIfMetric*/
#endif

/** Base class to manage Sparkplug metrics */
class CIfMetric
{
private:
	CValObj m_objValue; 

protected:
	uint64_t m_timestamp; /** Time stamp value*/
	uint32_t m_uiDataType = METRIC_DATA_TYPE_UNKNOWN;
	std::string m_sName; /** site name*/
	std::string m_sSparkPlugName; /** spark plug name*/
	
public:
	/* Constructor **/
	CIfMetric(std::string a_sName, const uint64_t a_timestamp, uint32_t a_uiDataType) :
			m_timestamp{a_timestamp}, m_uiDataType{a_uiDataType}, m_sName{a_sName}, m_sSparkPlugName{a_sName}
	{
	}

	CIfMetric(std::string a_sName) : CIfMetric(a_sName, get_current_timestamp(), METRIC_DATA_TYPE_UNKNOWN)
	{
	}

	CIfMetric(std::string a_sName, uint32_t a_uiDataType) : CIfMetric(a_sName, get_current_timestamp(), a_uiDataType)
	{
	}

	CIfMetric(): CIfMetric("", get_current_timestamp(), METRIC_DATA_TYPE_UNKNOWN)
	{
	}

	/* Virtual Destructor*/
    virtual ~CIfMetric(){};

	/**Function to set name*/
	void setName(std::string a_sName)
	{
		m_sName = a_sName;
	}

	/** function to get name*/
	std::string getName() const
	{
		return m_sName;
	}
	
	/** function to get spark plug name*/
	std::string getSparkPlugName() const
	{
		return m_sSparkPlugName;
	}
	
	/** function to set spark plug name*/
	void setSparkPlugName(std::string a_sVal)
	{
		m_sSparkPlugName = a_sVal;
	}

	/** function to set time stamp value*/
	void setTimestamp(const uint64_t a_timestamp)
	{
		m_timestamp = a_timestamp;
	}

	/** function to get time stamp value*/
	uint64_t getTimestamp() const
	{
		return m_timestamp;
	}

	/** function to return data type*/
	uint32_t getDataType() const
	{
		return m_uiDataType;
	}

	/** function to return data type*/
	void setDataType(uint32_t a_uiDataType)
	{
		m_uiDataType = a_uiDataType;
	}

	/** function to get CValObj object*/
	virtual CValObj& getValue()
	{		
		return m_objValue;
	}

	/** function to compare metric value*/
	virtual uint8_t compareValue(CIfMetric &a_obj) const = 0;

	/** function to assign new metric value*/
	virtual uint8_t assignNewValue(CIfMetric &a_obj) = 0;

	/** function to create a metric from CJSon element*/
	virtual bool processMetric(cJSON *a_cjArrayElemMetric) = 0;

	/** function to set value of object for spark plug metric*/
	virtual bool setValObj(org_eclipse_tahu_protobuf_Payload_Metric& a_sparkplugMetric)
	{
		setTimestamp(a_sparkplugMetric.timestamp);
		return true;
	}

	/** function to set name and value of metric into Sparkplug object*/
	virtual bool addMetricNameValue(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) = 0;

	/** function to set name and value of parameter into Sparkplug object*/
	virtual bool addParameterNameValue(org_eclipse_tahu_protobuf_Payload_Template_Parameter& a_rParam) = 0;
	
	/** function to add metric information into Sparkplug object for Birth msg */
	virtual bool addMetricForBirth(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) = 0;

	/** function to add metric information into Sparkplug object for Birth msg for Modbus metric*/
	virtual bool addModbusMetric(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric, bool a_bIsBirth) = 0;

	/** function to create CJSON object for this metric */
	// change the prototype here
	virtual bool assignToCJSON(cJSON *a_cjMetric, bool a_bIsRealDevice) = 0;

	/** function to compare one metric with this metric */
	virtual bool compareMetrics(const std::shared_ptr<CIfMetric> &a_pUDT)
	{
		if(a_pUDT)
		{
			if((m_uiDataType == a_pUDT->m_uiDataType) &&
				(m_sName == a_pUDT->m_sName))
	{
				return true;
			}
		}
		return false;
	}

	/** function added for future scope. */
	virtual bool validate()
	{
		return true;
	}

	/** function to print metric information */
	virtual void print() const
	{
		DO_LOG_INFO("Name : " + m_sName + " m_uiDataType " + std::to_string(m_uiDataType));
	}
#ifdef UNIT_TEST
	friend class Metric_ut;
	virtual void initTestData(metricMapIf_t &a_metricMap, metricMapIf_t &a_paramsMap, std::string &a_sUDTDefName,
				std::string &a_sUDTDefVer, bool a_bIsDefinition, std::shared_ptr<CUDT> a_pCUDTDefRef = nullptr)
		{}
#endif

};

/** Map: Key = Metric name, Value = Metric reference*/
using metricMapIf_t = std::map<std::string, std::shared_ptr<CIfMetric>>; /** map with key as string and value as pointer to  CIfMetric*/

/** Class to manage sparkplug name and values*/
class CMetric : public CIfMetric
{
	CValObj m_objVal; /** object of class CValObj*/
	var_metric_ref_t m_rDirectProp; /** reference to var_metric_ref_t*/

public:
	/** constructor*/
	CMetric() : CIfMetric(), m_objVal{}, m_rDirectProp{}
	{
	}

	CMetric(std::string a_sName) : CIfMetric(a_sName),
			m_objVal{ }, m_rDirectProp{std::monostate{}}
	{
	}

	CMetric(std::string a_sName, uint32_t a_uiDataType) : CIfMetric(a_sName, a_uiDataType),
			m_objVal{ }, m_rDirectProp{std::monostate{}}
	{
	}

	CMetric(std::string a_sName, const CValObj &a_objVal, const uint64_t a_timestamp) :
			CIfMetric(a_sName, a_timestamp), 
			m_objVal{a_objVal}, m_rDirectProp{std::monostate{}}
	{
	}
	
	CMetric(const network_info::CUniqueDataPoint &a_rDirectPropRef) :
			CIfMetric(a_rDirectPropRef.getDataPoint().getID(), METRIC_DATA_TYPE_STRING),
			m_objVal{METRIC_DATA_TYPE_STRING, std::string("")}, m_rDirectProp{a_rDirectPropRef}
	{
	}
	
	/** function gets object value*/
	CValObj& getValue()  
	{
		return m_objVal;
	}

	/** function to set value*/
	void setValue(const CValObj &a_objVal)
	{
		m_objVal.assignValue(a_objVal);
	}

	/** function to compare metric value*/
	uint8_t compareValue(CIfMetric &a_obj) const override
	{
		CMetric *pOtherMetric = dynamic_cast<CMetric*>(&a_obj);
		if(pOtherMetric)
		{
			return m_objVal.compareValue(pOtherMetric->getValue());
		}
		return DATATYPE_DIFFERENT;
	}

	/** function to assign new metric value*/
	uint8_t assignNewValue(CIfMetric &a_obj) override
	{
		CMetric *pOtherMetric = dynamic_cast<CMetric*>(&a_obj);
		if(pOtherMetric)
	{
			return m_objVal.assignValue(pOtherMetric->getValue());
		}
		return 0;
	}

	/** function set value of object for spark plug metric*/
	bool setValObj(org_eclipse_tahu_protobuf_Payload_Metric& a_sparkplugMetric) override
	{
		CIfMetric::setValObj(a_sparkplugMetric);
		return m_objVal.setValObj(a_sparkplugMetric);
	}

	/** function to create CJSON object for this metric with a flag to indicate a real device */
	bool assignToCJSON(cJSON *a_cjMetric, bool a_bIsRealDevice) override
	{
		if (a_bIsRealDevice)
		{
			return m_objVal.assignToCJSON(a_cjMetric, "scaledValue");
		}
		return m_objVal.assignToCJSON(a_cjMetric, "value");
	}

	/** function to add metric name, value to Sparkplug object for this metric */
	bool addMetricNameValue(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) override;

	/** function to add name, value to Sparkplug object for this parameter */
	bool addParameterNameValue(org_eclipse_tahu_protobuf_Payload_Template_Parameter& a_rParam) override;
	
	/** function to create Sparkplug object for this metric for birth message */
	bool addMetricForBirth(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) override;
	bool addMetricForBirth(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric, bool a_bIsNBIRTH);

	/** function to create Sparkplug object for this metric if it is of type Modbus */
	bool addModbusMetric(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric, bool a_bIsBirth) override;

	/** function to create this metric from CJSON object for this metric */
	bool processMetric(cJSON *a_cjArrayElemMetric) override;

	/** function to compare 2 metrics */
	virtual bool compareMetrics(const std::shared_ptr<CIfMetric> &a_pUDT)
	{
		if(a_pUDT)
		{
			if(true == CIfMetric::compareMetrics(a_pUDT))
			{
				CMetric *pOtherMetric = dynamic_cast<CMetric*>(a_pUDT.get());
				if(pOtherMetric)
				{
					if(SAMEVALUE_OR_DTATYPE == m_objVal.compareDataType(pOtherMetric->m_objVal))
	{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** function to print*/
	virtual void print() const override 
	{
		CIfMetric::print();
		m_objVal.print();
	}
};

/*
 * Composite Class to group primitive data types as a single object
 * UDT = User Defined Type
 * */
class CUDT: public CIfMetric 
{
	metricMapIf_t m_mapMetrics; /** metric information*/
	metricMapIf_t m_mapParams; /** parameter information*/
	std::string m_sUDTDefName; /** UDT definition reference*/
	std::string m_sUDTDefVer; /** UDT definition version*/
	bool m_bIsDefinition; /** Indicates whether this instance is definition of UDT */
	std::shared_ptr<CUDT> m_pCUDTDefRef; /** reference to UDT definition instance */

	bool readUDTRefData(cJSON *a_cjArrayElemMetric);
	metricMapIf_t compareMetricMapValues(const metricMapIf_t &a_myMap, metricMapIf_t &a_newMap) const;
	
#ifdef UNIT_TEST
	friend class Metric_ut;
#endif
	
	public:
	/** constructor*/
	CUDT(std::string a_sName, uint32_t a_uiDataType, bool a_bIsDef = false, 
			std::string a_sVersion = "") : 
			CIfMetric(a_sName, a_uiDataType),
			m_sUDTDefName{""}, m_sUDTDefVer{a_sVersion}, m_bIsDefinition{a_bIsDef}, m_pCUDTDefRef{nullptr}
	{
		if(true == m_bIsDefinition)
		{
			m_sUDTDefName.assign(a_sName);
		}
	}
	/** Destructor*/
	virtual ~CUDT()
	{
		m_mapMetrics.clear();
		m_mapParams.clear();
		m_pCUDTDefRef = nullptr;
	}

	/** function to compare value*/
	virtual uint8_t compareValue(CIfMetric &a_obj) const override;

	/** function to assign new metric value*/
	virtual uint8_t assignNewValue(CIfMetric &a_obj) override;

	/** function to create a metric from CJSon element*/
	bool processMetric(cJSON *a_cjArrayElemMetric) override;

	/** function set value of object for spark plug metric*/
	virtual bool setValObj(org_eclipse_tahu_protobuf_Payload_Metric& a_sparkplugMetric) override;

	/** Function to store metric name, value data in Sparkplug object */	
	virtual bool addMetricNameValue(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) override;

	/** function to add name, value to Sparkplug object for this parameter */
	bool addParameterNameValue(org_eclipse_tahu_protobuf_Payload_Template_Parameter& a_rParam) override
	{
		// Parameters are not of type template
		return false;
	}
	
	/** Function to store metric data in Sparkplug object for birth message */
	bool addMetricForBirth(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric) override;
	bool addMetricForBirth(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric, bool a_bIsNBirth);

	/** Function to compare nested metrics in UDT */
	bool compareMetricMaps(metricMapIf_t &a_map1, metricMapIf_t &a_map2);
	
	virtual bool addModbusMetric(org_eclipse_tahu_protobuf_Payload_Metric& a_rMetric, bool a_bIsBirth) override
	{ return true; }
	
	/** Function to store metric data in CJSON object */
	virtual bool assignToCJSON(cJSON *a_cjMetric, bool a_bIsRealDevice) override;

	/** Function to compare 2 metrics */
	virtual bool compareMetrics(const std::shared_ptr<CIfMetric> &a_pUDT) override;

	/** Function to validate metric data, if any */
	virtual bool validate() override;

	/** Function to print metric */
	virtual void print() const override 
	{
		CIfMetric::print();
		for(auto &itr: m_mapMetrics)
		{
			if(itr.second)
			{
				itr.second.get()->print();
			}
		}
	}


#ifdef UNIT_TEST
	void initTestData(metricMapIf_t &a_metricMap, metricMapIf_t &a_paramsMap, std::string &a_sUDTDefName,
			std::string &a_sUDTDefVer, bool a_bIsDefinition)
	{
		    /** metric information*/
		    m_mapMetrics = a_metricMap;

			/** parameter information*/
		    m_mapParams = a_paramsMap;

			 /** UDT definition reference*/
		    m_sUDTDefName = a_sUDTDefName;

			/** UDT definition version*/
		    m_sUDTDefVer = a_sUDTDefVer;

			/** Indicates whether this instance is definition of UDT */
		    m_bIsDefinition = a_bIsDefinition;

	}

	void initTestRef(std::shared_ptr<CUDT>& a_pCUDTDefRef)
	{
		 /** reference to UDT definition instance */
			m_pCUDTDefRef = a_pCUDTDefRef;
	}
#endif

};

#endif
