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
#include "CommonDataShare.hpp"
#include "LibErrCodeManager.hpp"

#define THREAD_PRIORITY_5 5
#define THREAD_PRIORITY_10 10
#define THREAD_PRIORITY_15 15
#define THREAD_PRIORITY_50 50
#define THREAD_PRIORITY_55 55
#define THREAD_PRIORITY_60 60

#ifndef SCADA_RTU
/** Constructor
 */
CfgManager::CfgManager()
{
	isClientCreated = false;
	try {
		m_eii_cfg = new ConfigMgr();
		isClientCreated = true;
	} catch (...) {
	    LOG_ERROR_0("Exception occured in creation of CfgManager of EII");
	    return;
	}
}

/** Returns the single instance of this class
 *
 * @param  : nothing
 * @return : Object of this class
 */
CfgManager& CfgManager::Instance()
{
	static CfgManager _self;
	return _self;
}

/** Returns the client status of creation
 *
 * @param : Nothing
 * @return: true/false based on status
 */
bool CfgManager::IsClientCreated()
{
	return isClientCreated;
}
#endif

/** Returns the single instance of this class
 *
 * @param  : nothing
 * @return : Object of this class
 */
globalConfig::CGlobalConfig& globalConfig::CGlobalConfig::getInstance()
{
	static globalConfig::CGlobalConfig  _self;
	return _self;
}

// default constructor to initialize default values
globalConfig::COperation::COperation()
{
	m_isRT = false;
	m_operationPriority = 0;
	m_qos = 0;
	m_retries = 0;
}

// default constructor to initialize default values
globalConfig::COperationInfo::COperationInfo()
{
	m_opType = UNKNOWN_OPERATION;
	m_defaultIsRT = false;
}

/**
 * Display scheduler attributes
 * @param policy : [in] scheduler policy
 * @param param : [in] param to display thread priority
 */
void globalConfig::display_sched_attr(int policy, struct sched_param& param)
{
	std::string sPolicy = "";
	if(policy == SCHED_FIFO)
	{
		sPolicy = "FIFO";
	}
	else if(policy == SCHED_RR)
	{
		sPolicy = "RR";
	}
	else if(policy == SCHED_OTHER)
	{
		sPolicy = "SCHED_OTHER";
	}
	else
	{
		sPolicy = "UNKNOWN";
	}
	DO_LOG_INFO("policy = " + sPolicy +
			" priority = "+ std::to_string(param.sched_priority));
}

/**
 * Display thread attributes
 * @param a_sMsg : [in] string msg
 */
void globalConfig::display_thread_sched_attr(const std::string a_sMsg)
{
	int policy, s;
	static int count = 1;
	struct sched_param param;

	s = pthread_getschedparam(pthread_self(), &policy, &param);
	if (s != 0)
	{
		handle_error_en(s, "pthread_getschedparam");
	}
	DO_LOG_INFO("Thread number :: " + std::to_string(count++) +" Thread Name :: " + a_sMsg);

	display_sched_attr(policy, param);
}

/**
 * Constructor - fill up values in map with priority values
 */
globalConfig::CPriorityMgr::CPriorityMgr()
{
	//initialize all the default parameters
	m_opPriorityThreadInfo.insert(std::make_pair(6, std::make_pair(THREAD_PRIORITY_60, RR)));
	m_opPriorityThreadInfo.insert(std::make_pair(5, std::make_pair(THREAD_PRIORITY_55, RR)));
	m_opPriorityThreadInfo.insert(std::make_pair(4, std::make_pair(THREAD_PRIORITY_50, RR)));
	m_opPriorityThreadInfo.insert(std::make_pair(3, std::make_pair(THREAD_PRIORITY_15, RR)));
	m_opPriorityThreadInfo.insert(std::make_pair(2, std::make_pair(THREAD_PRIORITY_10, RR)));
	m_opPriorityThreadInfo.insert(std::make_pair(1, std::make_pair(THREAD_PRIORITY_5, RR)));
}

/** Function to set thread parameters
 ** @param : a_OpsInfo [in]: Operation class reference,
 ** @param : a_iPriority [in]: optional parameter for priority other than defined operation priority
 ** @param : a_eSched [in]: optional parameter for scheduler other than defined operation priority
 ** param : a_bIsOperation [in]: optional value. (if this flag is set then only a_iPriority & a_eSched will be used)
 * @return: Nothing
 */
void globalConfig::set_thread_sched_param(const COperation a_OpsInfo,
		const int a_iPriority,
		const eThreadScheduler a_eSched,
		const bool a_bIsOperation)
{
	int iThreadPriority = 0;
	int operationPriority = 0;
	globalConfig::eThreadScheduler threadPolicy;

	if(!a_bIsOperation)
	{
		operationPriority = a_OpsInfo.getOperationPriority();
		iThreadPriority = CPriorityMgr::getInstance().getThreadPriority(operationPriority);
		threadPolicy = CPriorityMgr::getInstance().getThreadScheduler(operationPriority);
	}
	else
	{
		iThreadPriority = a_iPriority;
		threadPolicy = a_eSched;
	}

	//set priority
	sched_param param;

	if(iThreadPriority == -1 || threadPolicy == UNKNOWN)
	{
		DO_LOG_ERROR("Failed to set thread priority for this thread");
		DO_LOG_ERROR("Invalid operation priority received for thread set priority");
		return;
	}

	param.sched_priority = iThreadPriority;

	int result;
	result = pthread_setschedparam(pthread_self(), threadPolicy, &param);
	if(0 != result)
	{
		handle_error_en(result, "pthread_setschedparam");
		DO_LOG_ERROR("Cannot set thread priority to : " + std::to_string(iThreadPriority));
		std::cout << __func__ << ":" << __LINE__ << " Cannot set thread priority, result : " << result << std::endl;
	}
	else
	{
		DO_LOG_INFO("thread priority to set to: " + std::to_string(iThreadPriority));
	}
	//end of set priority for current thread
}

/** Function to set default value based on operation type
 ** @param : a_eOpType [in]: 	operation type (POLLING,ON_DEMAND_READ,	ON_DEMAND_WRITE)
 * @return: Nothing
 */
void globalConfig::setDefaultConfig(const enum eOperationType a_eOpType)
{
	/// set to default value based on operation type
	YAML::Node defaultNode;
	if(a_eOpType == SPARKPLUG_OPS)
	{
		CSparkplugData::buildSparkPlugInfo(defaultNode,
				globalConfig::CGlobalConfig::getInstance().getSparkPlugInfo());
	}
	else
	{
		COperationInfo::buildOperationInfo(defaultNode,
				globalConfig::CGlobalConfig::getInstance().getOpPollingOpConfig(),
				a_eOpType);
	}
}

/** Function to validate key in YAML file
 * @param : baseNode [in] : YAML node to read from
 * @param : a_sKey [in] : key to validate
 * @param : a_eDataType [in] : data type
 * @return: [int] : 0- success & -1 - if key is empty or absent & -2 if incorrect data type
 */
int globalConfig::validateParam(const YAML::Node& a_BaseNode,
		const std::string& a_sKey,
		const enum eDataType a_eDataType)
{
	int iRet = 0;
	try
	{
		if(!(a_BaseNode[a_sKey])  || a_BaseNode[a_sKey].as<std::string>() == "")
		{
			DO_LOG_ERROR(a_sKey + " is not present or empty !!");
			DO_LOG_ERROR("setting it to default !!");
			iRet = -1;
		}
		else
		{
			switch (a_eDataType)
			{
			case DT_BOOL:
			{
				a_BaseNode[a_sKey].as<bool>();
			}
			break;
			case DT_INTEGER:
			{
				a_BaseNode[a_sKey].as<int>();
			}
			break;
			case DT_UNSIGNED_INT:
			{
				a_BaseNode[a_sKey].as<unsigned>();
			}
			break;
			case DT_STRING:
			{
				a_BaseNode[a_sKey].as<std::string>();
			}
			break;
			case DT_MAP:
			{
				a_BaseNode[a_sKey].as<std::map<std::string,std::string>>();
			}
			break;
			case DT_DOUBLE:
			{
				a_BaseNode[a_sKey].as<double>();
			}
			break;
			default:
			{
				DO_LOG_ERROR("Invalid data type::" + std::to_string(a_eDataType));
				iRet = -3;
			}
			break;
			}
		}
	}
	catch (YAML::Exception &e)
	{
		DO_LOG_ERROR( a_sKey +
				" field data type is invalid, setting it to default, exception :: " +	std::string(e.what()));
		iRet = -2;
	}
	return iRet;
}
/** Populate COperation data structures
 *
 * @param : a_baseNode [in] : YAML node to read from
 * @param : a_refOpration [in] : data structure to be fill
 * @param : m_isRT [in] : RT/Noon-RT
 * @return: Nothing
 */
void globalConfig::COperation::build(const YAML::Node& a_baseNode,
		COperation& a_refOpration,
		const bool a_isRT)
{

	std::string ops = "";
	if(!a_isRT)
	{
		DO_LOG_INFO("non-realtime parameters: ");
		ops = "non-realtime";
	}
	else
	{
		DO_LOG_INFO("realtime parameters: ");
		ops = "realtime";
	}

	/// validate priority
	if (validateParam(a_baseNode[ops], "operation_priority", DT_INTEGER) != 0)
	{
		// default value
		a_refOpration.m_operationPriority = DEFAULT_OPERATION_PRIORITY;
	}
	else
	{
		if(a_baseNode[ops]["operation_priority"].as<int>() <= 0 ||
				a_baseNode[ops]["operation_priority"].as<int>() > 6)
		{
			DO_LOG_ERROR(
					"operation_priority parameter is out of range (i.e. expected value must be between 1-6 inclusive ) setting it to default (i.e. 1)");
			a_refOpration.m_operationPriority = DEFAULT_OPERATION_PRIORITY;
		}
		else
		{
			a_refOpration.m_operationPriority = a_baseNode[ops]["operation_priority"].as<int>();
		}
	}

	// validate retries
	if (validateParam(a_baseNode[ops], "retries", DT_INTEGER) != 0)
	{
		// default value
		a_refOpration.m_retries = DEFAULT_RETRIES;
	}
	else
	{
		if(a_baseNode[ops]["retries"].as<int>() < 0 ||
				a_baseNode[ops]["retries"].as<int>() > 4)
		{
			DO_LOG_ERROR(
					"retries parameter is out of range (i.e. expected value must be between 0-4 inclusive) setting it to default (i.e. 0)");
			a_refOpration.m_retries = DEFAULT_RETRIES;
		}
		else
		{
			a_refOpration.m_retries = a_baseNode[ops]["retries"].as<int>();
		}
	}

	// validate qos
	if (validateParam(a_baseNode[ops], "qos", DT_INTEGER) != 0)
	{
		// default value
		a_refOpration.m_qos = DEFAULT_QOS;
	}
	else
	{
		if(a_baseNode[ops]["qos"].as<int>() < 0 ||
				a_baseNode[ops]["qos"].as<int>() > 2)
		{
			DO_LOG_ERROR(
					"qos parameter is out of range (i.e. expected value must be between 0-2 inclusive) setting it to default (i.e. 0)");
			a_refOpration.m_qos = DEFAULT_QOS;
		}
		else
		{
			a_refOpration.m_qos = a_baseNode[ops]["qos"].as<int>();
		}
	}


	DO_LOG_INFO("operation priority :: " + std::to_string(a_refOpration.getOperationPriority()));
	DO_LOG_INFO("retries :: " + std::to_string(a_refOpration.getRetries()));
	DO_LOG_INFO("qos :: " + std::to_string(a_refOpration.getQos()));

}

/** Populate COperation data structures
 *
 * @param : a_baseNode [in] : YAML node to read from
 * @param : a_refOpInfo [in] : data structure to be fill
 * @param : a_eOpType [in]: 	operation type (POLLING,ON_DEMAND_READ,	ON_DEMAND_WRITE)
 * @return: Nothing
 */
void globalConfig::COperationInfo::buildOperationInfo(const YAML::Node& a_baseNode,
		COperationInfo& a_refOpInfo,
		const eOperationType a_eOpType)
{
	/// fill out operationType and default real-time
	a_refOpInfo.m_opType = a_eOpType;
	if (validateParam(a_baseNode, "default_realtime", DT_BOOL) != 0)
	{
		a_refOpInfo.m_defaultIsRT = DEFAULT_REALTIME;
	}
	else
	{
		a_refOpInfo.m_defaultIsRT = a_baseNode["default_realtime"].as<bool>();

	}
	std::string default_RT = "default-" + std::to_string(a_refOpInfo.m_defaultIsRT);
	zmq_handler::set_RT_NRT(default_RT);
	DO_LOG_INFO(" default RT :: " + std::to_string(a_refOpInfo.m_defaultIsRT));

	COperation Obj;
	/// realtime
	Obj.build(a_baseNode, a_refOpInfo.getRTConfig(), true);

	/// non-realtime
	Obj.build(a_baseNode, a_refOpInfo.getNonRTConfig(), false);
}

/** Populate CSparkplugData data structures
 *
 * @param : a_baseNode [in] : YAML node to read from
 * @param : a_refOpInfo [in] : data structure to be fill
 * @return: Nothing
 */
void globalConfig::CSparkplugData::buildSparkPlugInfo(const YAML::Node& a_baseNode,
		CSparkplugData& a_refOpration)
{

	if (validateParam(a_baseNode, "group_id", DT_STRING) != 0)
	{
		a_refOpration.m_sGroupId = DEFAULT_GRP_ID;
	}
	else
	{
		a_refOpration.m_sGroupId = a_baseNode["group_id"].as<std::string>();
	}
	if (validateParam(a_baseNode, "edge_node_id", DT_STRING) != 0)
	{
		a_refOpration.m_stNodeName = DEFAULT_NODE_NAME;
	}
	else
	{
		a_refOpration.m_stNodeName = a_baseNode["edge_node_id"].as<std::string>();
	}

	DO_LOG_INFO("	group_id : " + a_refOpration.getGroupId());
	DO_LOG_INFO("	edge_node_id : ");
	DO_LOG_INFO("		nodeName : " + a_refOpration.m_stNodeName);

}

/** Populate DefaultScale value
 *
 * @param : a_baseNode [in] : YAML node to read from
 * @return: Nothing
 */
void globalConfig::CGlobalConfig::buildDefaultScaleFactor(const YAML::Node& a_baseNode)
{
	if (validateParam(a_baseNode, "default_scale_factor", DT_DOUBLE) != 0)
	{
		globalConfig::CGlobalConfig::getInstance().setDefaultScaleFactor(DEFAULT_SCALE_FACTOR);
		DO_LOG_ERROR("default scale factor parameter is not present, so hardcoded to 1.0");
	}
	else
	{
		double defaultScale = a_baseNode["default_scale_factor"].as<double>();
		if (0 == defaultScale)
		{
			globalConfig::CGlobalConfig::getInstance().setDefaultScaleFactor(DEFAULT_SCALE_FACTOR);
			DO_LOG_ERROR("In globalConfig yml default_scale_factor set to 0, so hardcoded to 1.0");
		}
		else 
		{			
			globalConfig::CGlobalConfig::getInstance().setDefaultScaleFactor(defaultScale);
			DO_LOG_ERROR("In globalConfig yml default_scale_factor is " + std::to_string(defaultScale));
		}
	}

}
/** Read global configurations from YAML file (Global_Config.yml)
 *  global configuration is available in common_config dir from docker volume
 *
 *  if any configuration is missing/invalid then following configuration will be used as a default one,
	#	default_realtime: false
	#	operation_priority: 1
	#	retries: 0
	#	qos: 0
 *
 * @return: true/false - based on success/failure
 */
bool globalConfig::loadGlobalConfigurations()
{
	bool bRetVal = false;
	bool isPollingExist = false;
	bool isOdReadExist = false;
	bool isOdWriteExist = false;
	bool isSparkPlugData = false;


	try
	{
		YAML::Node config = YAML::LoadFile(GLOBAL_CONFIG_FILE_PATH);
                
		for (auto it : config)
		{
			if(it.first.as<std::string>() == "Global")
			{
				YAML::Node ops = it.second;
				globalConfig::CGlobalConfig::getInstance().buildDefaultScaleFactor(ops);
				YAML::Node listOps = ops["Operations"];
				for (auto key : listOps)
				{
					YAML::Node node;
					if(key["Polling"])
					{
						node = key["Polling"];

						DO_LOG_INFO("\nFollowing Global configurations is available inside container");

						DO_LOG_INFO("For Polling >>>");
						COperationInfo::buildOperationInfo(node,
								globalConfig::CGlobalConfig::getInstance().getOpPollingOpConfig(),
								POLLING);
						isPollingExist = true;
						continue;
					}
					else if (key["on-demand-read"])
					{
						node = key["on-demand-read"];
						DO_LOG_INFO("For On-demand read >>>");
						COperationInfo::buildOperationInfo(node,
								globalConfig::CGlobalConfig::getInstance().getOpOnDemandReadConfig(),
								ON_DEMAND_READ);
						isOdReadExist= true;
						continue;
					}
					else if (key["on-demand-write"])
					{
						node = key["on-demand-write"];
						DO_LOG_INFO("For On-demand write >>>");
						COperationInfo::buildOperationInfo(node,
								globalConfig::CGlobalConfig::getInstance().getOpOnDemandWriteConfig(),
								ON_DEMAND_WRITE);
						isOdWriteExist= true;
						continue;
					}
					else if (key["SparkPlug_Operation"])
					{
						node = key["SparkPlug_Operation"];
						DO_LOG_INFO("For SparkPlug_Operation: >>>");
						CSparkplugData::buildSparkPlugInfo(node,
								globalConfig::CGlobalConfig::getInstance().getSparkPlugInfo());
						isSparkPlugData= true;
						continue;
					}
				}
			}
		}
	}
	catch (YAML::Exception& e)
	{
		DO_LOG_ERROR("Error while loading global configurations::" + std::string(e.what()));
		bRetVal = false;
	}

	if(isPollingExist && isOdReadExist && isOdWriteExist)
	{
		// success case
		bRetVal = true;
	}

	// check for errors
	if(!isPollingExist)
	{
		DO_LOG_ERROR("Polling key is missing");
		DO_LOG_INFO("Setting default config for Polling >>>");
		bRetVal = false;
		setDefaultConfig(POLLING);
	}
	if (!isOdReadExist)
	{
		DO_LOG_ERROR("on-demand-read key is missing");
		DO_LOG_INFO("Setting default config for On-Demand-Read >>>");
		bRetVal = false;
		setDefaultConfig(ON_DEMAND_READ);
	}
	if (!isOdWriteExist)
	{
		DO_LOG_ERROR("on-demand-write key is missing");
		DO_LOG_INFO("Setting default config for On-Demand-Write >>>");
		bRetVal = false;
		setDefaultConfig(ON_DEMAND_WRITE);
	}

	if (!isSparkPlugData)
	{
		DO_LOG_ERROR("SparkPlug_Operation key is missing");
		DO_LOG_INFO("Setting default config for SparkPlug_Operation >>>");
		bRetVal = false;
		setDefaultConfig(SPARKPLUG_OPS);
	}
	return bRetVal;
}
