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

/*** ConfigManager.hpp is used to handle configuration management using ConfigManager eii library*/

#ifndef UWC_CONFIGMANAGER_HPP_
#define UWC_CONFIGMANAGER_HPP_

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
// EII configmgr
#include "eii/config_manager/config_mgr.hpp"
#include <yaml-cpp/yaml.h>
#include "CommonDataShare.hpp"
#include "ZmqHandler.hpp"
#define DIR_PATH "/config"
#define GLOBAL_CONFIG_FILE_PATH "/opt/intel/eii/uwc_data/common_config/Global_Config.yml"
#define handle_error_en(en, msg) do { errno = en; perror(msg); } while (0)

using namespace eii::config_manager;

#ifndef SCADA_RTU

/** CfgManager class to manage the configuration*/
class CfgManager {
public:

	/** Returns the single instance of this class
	 *
	 * @param  : nothing
	 * @return : object of this class
	 */
	static CfgManager& Instance();


	/** Returns if EII configmgr is created or not
	 *
	 * @param : Nothing
	 * @return: true/false based on status
	 */
	bool IsClientCreated();

	/** Returns EII configMgr reference from EII CfgMgr library
	 *
	 * @param : Nothing
	 * @return: EII Configuration object reference
	 */ // getConfigClient()
	ConfigMgr* getEiiCfgMgr()  {
		return m_eii_cfg;
	}

	// EII ConfigMgr reference
	ConfigMgr* m_eii_cfg = NULL;

private:

	// // EII ConfigMgr reference
	// ConfigMgr* m_eii_cfg = NULL;

	/// True for success and false for failure for eii cfgmgr creation
	bool isClientCreated;

	/** Constructor*/
	CfgManager();

    /** copy constructor is private*/
	CfgManager(CfgManager const&);

    /** assignment operator is private */
	CfgManager& operator=(CfgManager const&);
};
#endif

/** globalConfig namespace consists of default values to be used for setting global configuration for all operations*/
namespace globalConfig
{

/** default values to be used for setting global configuration for all operations */
#define DEFAULT_OPERATION_PRIORITY 1
#define DEFAULT_REALTIME false
#define DEFAULT_RETRIES 0
#define DEFAULT_QOS 0
#define DEFAULT_GRP_ID "UWC Nodes"
#define DEFAULT_NODE_NAME "SCADA RTU"
const double DEFAULT_SCALE_FACTOR = 1.0;
/**
 * Enum of operation types and hierarchy
 */
enum eOperationType
{
	POLLING,       //!< POLLING
	ON_DEMAND_READ,//!< ON_DEMAND_READ
	ON_DEMAND_WRITE,//!< ON_DEMAND_WRITE
	SPARKPLUG_OPS,	// Sparkplug operation
	UNKNOWN_OPERATION
};

/**
 * Enum of thread policy types and hierarchy
 */
typedef enum eThreadScheduler
{
	OTHER, //!< OTHER
	FIFO,  //!< FIFO
	RR,    //!< RR
	UNKNOWN//!< UNKNOWN
}threadScheduler;

/** struct to hold thread info*/
struct stThreadInfo
{
	int priority; /** priority number*/
	eThreadScheduler sched;
};

/**
 * Enum of data types
 */
enum eDataType
{
	DT_BOOL,        //!< DT_BOOL
	DT_INTEGER,     //!< DT_INTEGER
	DT_UNSIGNED_INT,//!< DT_UNSIGNED_INT
	DT_STRING,      //!< DT_STRING
	DT_MAP,         //!< DT_MAP
	DT_DOUBLE       //!< DT_DOUBLE
};

/**
 * Class holds information about sparkplug operation
 */
class CSparkplugData
{
public:

	// default constructor
	CSparkplugData()
{
		m_sGroupId = "";
		m_stNodeName = "";
}

	static void buildSparkPlugInfo(const YAML::Node& a_baseNode,
			CSparkplugData& a_refOpration);

	/** Returns Group Id
	 *
	 * @param : Nothing
	 * @return: string containing group id
	 */
	const std::string& getGroupId() const
	{
		return m_sGroupId;
	}

	/** Returns Node name
	 *
	 * @param : Nothing
	 * @return: string containing node name
	 */
	const std::string& getNodeName() const
	{
		return m_stNodeName;
	}

private:
	std::string m_sGroupId;/** Group ID number*/
	std::string m_stNodeName; /** Node name*/
};

/**
 * Class holds information about individual operation
 */
class COperation
{
	bool m_isRT; /**RT or non-RT(true or false)*/
	int m_operationPriority; /** Operation priority number*/
	int m_qos; /** qos number*/
	int m_retries; /** number of reties*/

public:

	/** default constructor to initialize default values */
	COperation();

	/** Populate COperation data structures
	 *
	 * @param : a_baseNode [in] : YAML node to read from
	 * @param : a_refOpration [in] : data structure to be fill
	 * @param : m_isRT [in] : RT/Noon-RT
	 * @return: Nothing
	 */
	static void build(const YAML::Node& a_baseNode,
			COperation& a_refOpration,
			const bool a_isRT);

	/**
	 * Check if operation is real-time
	 * @return true if operation is real-time
	 * 			false if operation is not real-time
	 */
	bool isRT() const
	{
		return m_isRT;
	}

	/**
	 * Get operation thread priority
	 * @return operation thread priority in int
	 */
	int getOperationPriority() const
	{
		return m_operationPriority;
	}

	/**
	 * Get QOS for operation
	 * @return QOS for operation in int
	 */
	int getQos() const
	{
		return m_qos;
	}

	/**
	 * Get operation retry count
	 * @return operation retry count in int
	 */
	int getRetries() const
	{
		return m_retries;
	}

};

/**
 * Class holds information about operation per operation type
 */
class COperationInfo
{
	eOperationType m_opType; /** operation type*/
	bool m_defaultIsRT; /** default is RT or non-rt(true or false)*/

	COperation m_RTConfig;
	COperation m_NonRTConfig;

public:

	/** default constructor */
	COperationInfo();

	/** Populate COperation data structures
	 *
	 * @param : a_baseNode [in] : YAML node to read from
	 * @param : a_refOpInfo [in] : data structure to be fill
	 * @param : a_eOpType [in]: 	operation type (POLLING,ON_DEMAND_READ,	ON_DEMAND_WRITE)
	 * @return: Nothing
	 */
	static void buildOperationInfo(const YAML::Node& a_baseNode,
			COperationInfo& a_refOpInfo,
			const eOperationType a_eOpType);

	/**
	 * Get default RT configuration for this operation
	 * @return true if default RT
	 * 			false if not
	 */
	bool getDefaultRTConfig()
	{
		return m_defaultIsRT;
	}

	/**
	 * Get non-RT configuration of this operation
	 * @return reference of non-RT operation configuration
	 */
	COperation& getNonRTConfig()
	{
		return m_NonRTConfig;
	}

	/**
	 * Get RT configuration of this operation
	 * @return reference of RT operation configuration
	 */
	COperation& getRTConfig()
	{
		return m_RTConfig;
	}

};

/**
 * Class holds global configuration for all operations
 */
class CGlobalConfig
{
	COperationInfo m_OpPollingConfig;
	COperationInfo m_OpOnDemandReadConfig;
	COperationInfo m_OpOnDemandWriteConfig;
	CSparkplugData m_SparkPlugInfo;
	double m_dDefaultScale;

	// Private constructor so that no objects can be created.
	CGlobalConfig()
	{
		m_dDefaultScale = DEFAULT_SCALE_FACTOR;
	}
	CGlobalConfig(const CGlobalConfig & obj)=delete;
	CGlobalConfig& operator=(CGlobalConfig const&)=delete;

public:
	/**
	 * Get this instance of class
	 * @return reference of this class instance
	 */
	static CGlobalConfig& getInstance();

	/**
	 * Get configuration of polling operation
	 * @return reference to instance of polling operation class
	 */
	COperationInfo& getOpPollingOpConfig()
	{
		return m_OpPollingConfig;
	}

	/**
	 * Get configuration of on-demand read operation
	 * @return reference to instance of on-demand read operation class
	 */
	COperationInfo& getOpOnDemandReadConfig()
	{
		return m_OpOnDemandReadConfig;
	}

	/**
	 * Get configuration of on-demand write operation
	 * @return reference to instance of on-demand write operation class
	 */
	COperationInfo& getOpOnDemandWriteConfig()
	{
		return m_OpOnDemandWriteConfig;
	} 

	/**
	 * Get configuration of SparkPlug operation
	 * @return reference to instance of SparkPlug operation class
	 */
	CSparkplugData& getSparkPlugInfo()
	{
		return m_SparkPlugInfo;
	}

	/**
	 * Return configuration of DefaultScale
	 * @return DefaultScale from Global Config file
	 */
	double getDefaultScaleFactor()
	{
		return m_dDefaultScale;
	}

	/**
	 *  Set configuration of DefaultScale
	 *  @param defaultScale Set DefaultScale from Global Config file
	 */
	void setDefaultScaleFactor(double defaultScale)
	{
	  m_dDefaultScale = defaultScale;
	}

	/** Populate DefaultScale value
	 *
	 * @param : a_baseNode [in] : YAML node to read from
	 * @return: Nothing
	 */
	void buildDefaultScaleFactor(const YAML::Node& a_baseNode);
};

/**
 * Class holds information about thread priority for each operation
 */
class CPriorityMgr
{
	/** map holds operational priority wise information about thread scheduler and thread priority*/
	std::map<int, std::pair<int, threadScheduler>> m_opPriorityThreadInfo;

	/** Private constructor so that no objects can be created. */
	CPriorityMgr();
	CPriorityMgr(const CPriorityMgr & obj)=delete;
	CPriorityMgr& operator=(CPriorityMgr const&)=delete;

public:
	/**
	 * Get only this instance of class
	 * @return this instance of class
	 */
	static CPriorityMgr& getInstance()
	{
		static CPriorityMgr _self;
			return _self;
	}

	/**
	 * Get thread priority depending on operational priority
	 * @param opPriority :[in] operational priority
	 * @return thread priority in int
	 */
	int getThreadPriority(int opPriority)
	{
		if(m_opPriorityThreadInfo.find(opPriority) != m_opPriorityThreadInfo.end())
		{
			return m_opPriorityThreadInfo.at(opPriority).first;
		}
		else
		{
			return -1;
		}
	}

	/**
	 * Get thread scheduling policy depending on operational-priority
	 * @param opPriority : [in]operational priority
	 * @return thread scheduling policy
	 */
	globalConfig::eThreadScheduler getThreadScheduler(int opPriority)
	{
		if(m_opPriorityThreadInfo.find(opPriority) != m_opPriorityThreadInfo.end())
		{
			return m_opPriorityThreadInfo.at(opPriority).second;
		}
		else
		{
			return UNKNOWN;
		}
	}
};

/** Read global configurations from YAML file and store it in data structures
 *
 * @return: true/false - based on success/failure
 */
bool loadGlobalConfigurations();

/** Function to validate key in YAML file
 *
 * @param : baseNode [in] : YAML node to read from
 * @param : a_sKey [in] : key to validate
 * @param : a_eDataType [in] : data type
 * @return: [int] : 0- success & -1 - if key is empty or absent & -2 if incorrect data type
 */
int validateParam(const YAML::Node& a_BaseNode,
		const std::string& a_sKey,
		const enum eDataType a_eDT);

/** Function to set default value
 ** @param : a_eOpType [in]: 	operation type (POLLING,ON_DEMAND_READ,	ON_DEMAND_WRITE)
 * @return: Nothing
 */
void setDefaultConfig(const enum eOperationType a_eOpType);

/** Function to set thread parameters
 ** @param : a_OpsInfo [in]: Operation class reference,
 ** @param : a_iPriority [in]: optional parameter for priority other than defined operation priority
 ** @param : a_eSched [in]: optional parameter for scheduler other than defined operation priority
 ** param : a_bIsOperation [in]: optional value. (if this flag is set then only a_iPriority & a_eSched will be used)
 * @return: Nothing
 */
void set_thread_sched_param(const COperation a_OpsInfo,
		const int a_iPriority = 0,
		const eThreadScheduler a_eSched = OTHER,
		const bool a_bIsOperation = false);

/** functions added for displaying thread attr */
void display_thread_sched_attr(const std::string a_sMsg);
void display_sched_attr(int policy, struct sched_param& param);

}  /** namespace globalConfig */

#endif //UWC_CONFIGMANAGER_HPP_
