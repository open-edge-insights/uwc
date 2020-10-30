/*************************************************************************************
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation. Title to the
* Material remains with Intel Corporation.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or delivery of
* the Materials, either expressly, by implication, inducement, estoppel or otherwise.
*************************************************************************************/

#include <iostream>
#include <atomic>
#include <map>
#include <algorithm>
#include <arpa/inet.h>
#include "NetworkInfo.hpp"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/yaml.h"
#include "utils/YamlUtil.hpp"
#include "ConfigManager.hpp"

#include "EnvironmentVarHandler.hpp"

using namespace network_info;
using namespace CommonUtils;

// Set this if configuration YML files are kept in a docker volume
// This is true for SPRINT 1
#define CONFIGFILES_IN_DOCKER_VOLUME

// Unnamed namespace to define globals 
namespace  
{
	eNetworkType g_eNetworkType{eNetworkType::eALL};
	std::atomic<bool> g_bIsStarted{false};
	std::map<std::string, CWellSiteInfo> g_mapYMLWellSite;
	std::map<std::string, CRTUNetworkInfo> g_mapRTUNwInfo;
	std::map<std::string, CUniqueDataPoint> g_mapUniqueDataPoint;
	std::map<std::string, CUniqueDataDevice> g_mapUniqueDataDevice;
	std::vector<std::string> g_sErrorYMLs;
	unsigned short g_usTotalCnt{0};

	/**
	 * Populate unique point data
	 * @param a_oWellSite :[in] well site to populate info of
	 */
	void populateUniquePointData(const CWellSiteInfo &a_oWellSite)
	{
		DO_LOG_DEBUG("Start");
		for(auto &objWellSiteDev : a_oWellSite.getDevices())
		{
			std::string devID(SEPARATOR_CHAR+ objWellSiteDev.getID() + SEPARATOR_CHAR + a_oWellSite.getID());

			// populate device data
			CUniqueDataDevice objDevice{a_oWellSite, objWellSiteDev};
			g_mapUniqueDataDevice.emplace(devID, objDevice);

			auto &refUniqueDev = g_mapUniqueDataDevice.at(devID);

			//unsigned int uiPoint = 0;
			for(auto &objPt : objWellSiteDev.getDevInfo().getDataPoints())
			{
				std::string sUniqueId(SEPARATOR_CHAR + objWellSiteDev.getID()
						+ SEPARATOR_CHAR + a_oWellSite.getID() + SEPARATOR_CHAR +
										objPt.getID());
				
				// Build unique data point
				CUniqueDataPoint oUniquePoint{sUniqueId, a_oWellSite, objWellSiteDev, objPt};
				g_mapUniqueDataPoint.emplace(sUniqueId, oUniquePoint);

				refUniqueDev.addPoint(std::ref(g_mapUniqueDataPoint.at(sUniqueId)));

				DO_LOG_INFO(oUniquePoint.getID() +
							"=" +
							std::to_string(oUniquePoint.getMyRollID()));
			}
		}
		DO_LOG_DEBUG("End");
	}

	
	#ifdef CONFIGFILES_IN_DOCKER_VOLUME
	std::vector<std::string> g_sWellSiteFileList;

	/**
	 * Get well site list
	 * @return 	true : on success,
	 * 			false : on error
	 */
	bool _getWellSiteList(string a_strSiteListFileName)
	{
		DO_LOG_DEBUG(" Start: Reading site_list.yaml");
		try
		{
			YAML::Node Node = CommonUtils::loadYamlFile(a_strSiteListFileName);
			CommonUtils::convertYamlToList(Node, g_sWellSiteFileList);
		}
		catch(YAML::Exception &e)
		{
			DO_LOG_FATAL(e.what());
			return false;
		}
		DO_LOG_DEBUG("End:");
		return true;
	}
	#endif
}

/**
 * Add unique data point reference to unique device
 * @param a_rPoint :[in] data point reference
 */
void network_info::CUniqueDataDevice::addPoint(const CUniqueDataPoint &a_rPoint)
{
	try
	{
		m_rPointList.push_back(a_rPoint);
	}
	catch (std::exception &e)
	{
		DO_LOG_ERROR(e.what());
	}
}

/**
 * Add data point in m_DataPointList
 * @param a_oDataPoint :[in] data point values to be add
 * if DataPoint id is same in one file then it will be ignored
 * @return 	0 : on success,
 * 			-1 : on error (if point id is duplicate in datapoints file then this point will be ignored)
 */
int network_info::CDeviceInfo::addDataPoint(CDataPoint a_oDataPoint)
{
	// Search whether given point name is already present
	// If present, ignore the datapoint
	// If not present, add it

	DO_LOG_DEBUG("Start: To add DataPoint - " +
				a_oDataPoint.getID());
	for(auto oDataPoint: m_DataPointList)
	{
		if(0 == oDataPoint.getID().compare(a_oDataPoint.getID()))
		{
			DO_LOG_ERROR(":Already present DataPoint with id" +
						a_oDataPoint.getID());
			// This point name is already present. Ignore this point
			return -1;
		}
	}
	m_DataPointList.push_back(a_oDataPoint);

	DO_LOG_DEBUG("End: Added DataPoint - " +
				a_oDataPoint.getID());
	return 0;
}

/**
 * Add device
 * @param a_oDevice :[in] device to add
 * @return 	0 : on success,
 * 			-1 : on error - This error will be returned if device ID is common in one site and
 * 			 this device will be ignored
 * 			-2 : if device type does not match with network type.
 * 			E.g. RTU devices will be ignored in TCP container and vice versa
 */
int network_info::CWellSiteInfo::addDevice(CWellSiteDevInfo a_oDevice)
{
	// 1. Check device type TCP or RTU and whether it matches with this network
	//    If not matched, ignore the device
	// 2. Search whether given device name is already present
	//    If present, ignore the device
	//    If not present, add it

	DO_LOG_DEBUG(" Start: To add DataPoint - " +
				a_oDevice.getID());
	// Check network type
	if (g_eNetworkType != a_oDevice.getAddressInfo().m_NwType)
	{
		// Device type and network type are not matching.

		if(g_eNetworkType == eNetworkType::eALL)
		{
			//
		}
		else
		{
			return -2;
		}
	}
	
	// Search whether given device name is already present
	for(auto oDev: m_DevList)
	{
		if(0 == oDev.getID().compare(a_oDevice.getID()))
		{
			// This point name is already present. Ignore this point
			return -1;
		}
	}
	m_DevList.push_back(a_oDevice);
	DO_LOG_DEBUG(" End: Added device - " +
				a_oDevice.getID());
	return 0;
}

/**
 * build well site info to add all devices in data structures from YAML files
 * @param a_oData 		:[in] a_oData : YAML data node to read from
 * @param a_oWellSite	:[in] Data structures to store device information
 */
void network_info::CWellSiteInfo::build(const YAML::Node& a_oData, CWellSiteInfo &a_oWellSite)
{
	//DO_LOG_DEBUG(" Start");
	bool bIsIdPresent = false;
	try
	{
		for (auto test : a_oData)
		{
			if(test.first.as<std::string>() == "id")
			{
				a_oWellSite.m_sId = test.second.as<std::string>();

				DO_LOG_INFO(" : Scanning site: " +
							a_oWellSite.m_sId);
				bIsIdPresent = true;
				continue;
			}
			if(test.second.IsSequence() && test.first.as<std::string>() == "devicelist")
			{
				const YAML::Node& list = test.second;

				for (auto nodes : list)
				{
					try
					{
						CWellSiteDevInfo objWellsiteDev;
						int32_t i32RetVal = 0;
						CWellSiteDevInfo::build(nodes, objWellsiteDev);
						i32RetVal = a_oWellSite.addDevice(objWellsiteDev);
						if(0 == i32RetVal)
						{
							DO_LOG_INFO(" : Added device with id: " +
										objWellsiteDev.getID());
						}
						else if(-1 == i32RetVal)
						{
							DO_LOG_ERROR("Ignoring device with id : " +
							objWellsiteDev.getID() +
							", since this point name is already present. Ignore this point.");
							std::cout << "Ignoring device with id : " << objWellsiteDev.getID() << ", since this point name is already present. Ignore this point."<< endl;
						}
						else if(-2 == i32RetVal)
						{
							DO_LOG_ERROR("Ignoring device with id : " +
							objWellsiteDev.getID() +
							", since Device type and network type are not matching.");
							std::cout << "Ignoring device with id : " << objWellsiteDev.getID() << ", since Device type and network type are not matching."<< endl;
						}
					}
					catch (YAML::Exception& ye)
					{
						DO_LOG_ERROR("Error while parsing device with Exception :: " + std::string(ye.what()));
					}
					catch (std::exception& e)
					{
						DO_LOG_ERROR("Error while parsing device with Exception :: " + std::string(e.what()));
					}
				}
			}
		}
		if(false == bIsIdPresent)
		{
			DO_LOG_FATAL(" Site without id is found. Ignoring this site.");
			throw YAML::Exception(YAML::Mark::null_mark(), "Id key not found");
			cout << "Site without id is found. Ignoring this site."<<endl;
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_FATAL( e.what());
		throw;
	}
	DO_LOG_DEBUG(" End");
}

/**
 * function to validate IP address
 * @param ipAddress :[in] IP to validate
 * @return 	true : on success,
 * 			false : on error
 */
bool network_info::validateIpAddress(const string &ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result;
}

/**
 * build network information for RTU network
 * SAMPLE YML to read is mentioned in device group file
 * E.g baud rate, port, parity
 * @param a_oData			:[in] YAML data node to read from
 * @param a_oNwInfo:[in] Data structure to be updated
 */
void CRTUNetworkInfo::buildRTUNwInfo(CRTUNetworkInfo &a_oNwInfo,
		std::string a_fileName)
{
	try
	{
		YAML::Node node;
		std::map<std::string, CRTUNetworkInfo>::iterator itr =
				g_mapRTUNwInfo.find(a_fileName);

		if(itr != g_mapRTUNwInfo.end())
		{
			// element already exist in map
			a_oNwInfo = g_mapRTUNwInfo.at(a_fileName);
		}
		else
		{
			node = CommonUtils::loadYamlFile(a_fileName);
			a_oNwInfo.m_iBaudRate = atoi(node["baudrate"].as<string>().c_str());
			a_oNwInfo.m_sPortName = node["com_port_name"].as<std::string>();
			a_oNwInfo.m_sParity = node["parity"].as<std::string>();
			a_oNwInfo.m_lInterframeDelay = node["interframe_delay"].as<long>();
			a_oNwInfo.m_lResTimeout = node["response_timeout"].as<long>();
			g_mapRTUNwInfo.emplace(a_fileName, a_oNwInfo);
		}

		DO_LOG_INFO("RTU network info parameters...");
		DO_LOG_INFO(" baudrate = " + to_string(a_oNwInfo.getBaudRate()));
		DO_LOG_INFO(" com_port_name = " + a_oNwInfo.getPortName());
		DO_LOG_INFO(" parity = " + a_oNwInfo.getParity());
		DO_LOG_INFO(" interframe_delay = " + to_string(a_oNwInfo.getInterframeDelay()));
		DO_LOG_INFO(" response_timeout = " + to_string(a_oNwInfo.getResTimeout()));


		cout << "RTU network info parameters..." << endl;
		cout << " baudrate = " + to_string(a_oNwInfo.getBaudRate()) << endl;
		cout << " com_port_name = " + a_oNwInfo.getPortName() << endl;
		cout << " parity = " +  a_oNwInfo.getParity() << endl;
		cout << " interframe_delay = " +  to_string(a_oNwInfo.getInterframeDelay()) << endl;
		cout << " response_timeout = " +  to_string(a_oNwInfo.getResTimeout()) << endl;

	}
	catch (YAML::Exception& e)
	{
		DO_LOG_ERROR("Incorrect configurations is given for RTU network info :: " + std::string(e.what()));
	}
}

/**
 * build well site device info to store device specific parameters mentioned in YAML file
 * SAMPLE YML to read is PL0, PL1,..etc..
 * E.g protocol, ipaddress, port, unitid, slaveid
 * @param a_oData			:[in] YAML data node to read from
 * @param a_oWellSiteDevInfo:[in] Data structure to be updated
 */
void network_info::CWellSiteDevInfo::build(const YAML::Node& a_oData, CWellSiteDevInfo &a_oWellSiteDevInfo)
{
	DO_LOG_DEBUG(" Start");
	bool bIsIdPresent = false;
	bool bIsProtocolPresent = false;
	try
	{
		for (auto it : a_oData)
		{
			// Default value is incorrect context
			a_oWellSiteDevInfo.setCtxInfo(-1);
			if(it.first.as<std::string>() == "id")
			{
				a_oWellSiteDevInfo.m_sId = it.second.as<std::string>();

				DO_LOG_INFO(" : Scanning site device: " +
							a_oWellSiteDevInfo.m_sId);
				bIsIdPresent = true;
				continue;
			}

			if(it.first.as<std::string>() == "rtu_master_network_info")
			{
				CRTUNetworkInfo::buildRTUNwInfo(a_oWellSiteDevInfo.m_rtuNwInfo,
						it.second.as<std::string>());
			}

			// read tcp master info 
			if(it.first.as<std::string>() == "tcp_master_info")
			{
				YAML::Node node = CommonUtils::loadYamlFile(it.second.as<std::string>());
				a_oWellSiteDevInfo.m_stTCPMasterInfo.m_lInterframeDelay =
						node["interframe_delay"].as<long>();
				a_oWellSiteDevInfo.m_stTCPMasterInfo.m_lResTimeout =
						node["response_timeout"].as<long>();
			}

			if(it.first.as<std::string>() == "protocol" && it.second.IsMap())
			{
				std::map<std::string, std::string > tempMap;
				tempMap = it.second.as<std::map<string, string>>();

				//a_oWellSiteDevInfo.m_stAddress.a_NwType = tempMap.at("protocol");
				if(tempMap.at("protocol") == "PROTOCOL_RTU")
				{
					try
					{
						a_oWellSiteDevInfo.m_stAddress.m_stRTU.m_uiSlaveId = atoi(tempMap.at("slaveid").c_str());

						a_oWellSiteDevInfo.m_stAddress.m_NwType = network_info::eNetworkType::eRTU;
						bIsProtocolPresent = true;
						DO_LOG_INFO(" : RTU protocol: ");
					}
					catch(exception &e)
					{
						DO_LOG_FATAL(e.what());
						std::cout << __func__ << "Required keys not found in PROTOCOL_RTU" << endl;
						throw YAML::Exception(YAML::Mark::null_mark(), "Required keys not found in PROTOCOL_RTU");
					}
				}
				else if(tempMap.at("protocol") == "PROTOCOL_TCP")
				{
					try
					{
						if(tempMap.at("ipaddress") == "" || tempMap.at("port") == "" || tempMap.at("unitid") == "")
						{
							std::cout << " ERROR:: ipaddress or port or unitid cannot be empty" <<endl;
							std::cout << " Given parameters:: " <<endl;
							std::cout << " ipaddress:: " << tempMap.at("ipaddress")<<endl;
							std::cout  << " port:: " << tempMap.at("port")<<endl;
							std::cout  << " unitid:: " <<tempMap.at("unitid")<<endl;
							DO_LOG_ERROR("ipaddress or port or unitid cannot be empty");
							DO_LOG_ERROR("Given parameters are following ::");
							DO_LOG_ERROR("ipaddress:: " + tempMap.at("ipaddress"));
							DO_LOG_ERROR("unitid:: " + tempMap.at("unitid"));
							DO_LOG_ERROR("port:: " + tempMap.at("port"));
							return;
						}

						if(validateIpAddress(tempMap.at("ipaddress")))
						{
							a_oWellSiteDevInfo.m_stAddress.m_stTCP.m_sIPAddress = tempMap.at("ipaddress");
						}
						else
						{
							std::cout << "ERROR : IP address is invalid!!" <<tempMap.at("ipaddress") <<endl;
							DO_LOG_ERROR("IP address is invalid " + tempMap.at("ipaddress"));
						}

						a_oWellSiteDevInfo.m_stAddress.m_stTCP.m_ui16PortNumber = atoi(tempMap.at("port").c_str());
						a_oWellSiteDevInfo.m_stAddress.m_NwType = network_info::eNetworkType::eTCP;
						a_oWellSiteDevInfo.m_stAddress.m_stTCP.m_uiUnitID = atoi(tempMap.at("unitid").c_str());
						bIsProtocolPresent = true;

						DO_LOG_INFO(" : TCP protocol: " +
									a_oWellSiteDevInfo.m_stAddress.m_stTCP.m_sIPAddress +
									":" +std::to_string(a_oWellSiteDevInfo.m_stAddress.m_stTCP.m_ui16PortNumber));
					}
					catch(exception &e)
					{
						DO_LOG_FATAL(e.what());
						std::cout << "Required keys not found in PROTOCOL_TCP"<<endl;
						throw YAML::Exception(YAML::Mark::null_mark(), "Required keys not found in PROTOCOL_TCP");
					}
				}
				else
				{
					// error
					DO_LOG_ERROR(" : Unknown protocol: " +
								tempMap.at("protocol"));
					std::cout << __func__<< " : Unknown protocol: " << tempMap.at("protocol") << endl;
					throw YAML::Exception(YAML::Mark::null_mark(), "Unknown protocol found");
				}
			}

			if(it.first.as<std::string>() == "deviceinfo")
			{
				YAML::Node node = CommonUtils::loadYamlFile(it.second.as<std::string>());
				CDeviceInfo::build(node, a_oWellSiteDevInfo.getDevInfo1());
			}
		}
		if(false == bIsIdPresent)
		{
			DO_LOG_ERROR(" Site device without id is found. Ignoring this well device.");
			std::cout << __func__ << " Site device without id is found. Ignoring this well device."<<endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Id key not found");

		}
		if(false == bIsProtocolPresent)
		{
			DO_LOG_ERROR(" Site device without protocol is found. Ignoring it.");
			std::cout << __func__ << " Site device without protocol is found. Ignoring it.."<<endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Protocol key not found");
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << __func__<<" Exception :: " << e.what()<<endl;
		throw;
	}
	
	DO_LOG_DEBUG("End");
}

/**
 * This function is used to read data points YML files and store it in CDeviceInfo data struct
 * All the datapoints.yml file data will be stored in data structures
 * @param a_oData			:[in] YAML data node to read from
 * @param a_oCDeviceInfo:[in] Data structure to be updated
 */
void network_info::CDeviceInfo::build(const YAML::Node& a_oData, CDeviceInfo &a_oCDeviceInfo )
{
	DO_LOG_DEBUG("Start");
	bool bIsNameFound = false;
	try
	{
		for (auto test : a_oData)
		{
			if(test.first.as<std::string>() == "device_info")
			{
				try
				{
					a_oCDeviceInfo.m_sName = test.second["name"].as<std::string>();
					bIsNameFound = true;
					continue;
				}
				catch(exception &e)
				{
					DO_LOG_FATAL(e.what());
					throw YAML::Exception(YAML::Mark::null_mark(), "name key not found in device_info");
				}				
			}
			//a_oWellSite.m_sId = test["id"].as<std::string>();
			if(test.first.as<std::string>() == "pointlist")
			{
				YAML::Node node = CommonUtils::loadYamlFile(test.second.as<std::string>());

				DO_LOG_INFO(" : pointlist found: " +
							test.second.as<std::string>());

				for (auto it : node)
				{
					if(it.first.as<std::string>() == "file" && it.second.IsMap())
					{
						// store if required
					}
					if(it.second.IsSequence() && it.first.as<std::string>() == "datapoints")
					{
						const YAML::Node& points =  it.second;
						for (auto it1 : points)
						{
							try
							{
								CDataPoint objCDataPoint;
								CDataPoint::build(it1, objCDataPoint, globalConfig::CGlobalConfig::getInstance().getOpPollingOpConfig().getDefaultRTConfig());
								if(0 == a_oCDeviceInfo.addDataPoint(objCDataPoint))
								{
									DO_LOG_INFO("Added point with id: " +
											objCDataPoint.getID());
								}
								else
								{
									DO_LOG_ERROR("Ignoring duplicate point ID from polling :"+ objCDataPoint.getID());
									std::cout << "ERROR: Ignoring duplicate point ID from polling :"<< objCDataPoint.getID() <<endl;
								}
							}
							catch (YAML::Exception& ye)
							{
								DO_LOG_ERROR("Error while parsing datapoint with Exception :: " + std::string(ye.what()));
							}
							catch (std::exception& e)
							{
								DO_LOG_ERROR("Error while parsing datapoint with Exception :: " + std::string(e.what()));
							}
						}
					}
				}
			}
		}
		if(false == bIsNameFound)
		{
			DO_LOG_ERROR(" Device without name is found. Ignoring this device.");
			throw YAML::Exception(YAML::Mark::null_mark(), "name key not found");
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_FATAL(e.what());
		throw;
	}
	DO_LOG_DEBUG("End");
}

/**
 * Get well site list
 * @return global well site map (e.g. ("PL0", CWellSiteInfo))
 */
const std::map<std::string, CWellSiteInfo>& network_info::getWellSiteList()
{
	DO_LOG_DEBUG("");
	return g_mapYMLWellSite;	
}

/**
 * Get unique point list
 * @return map of unique points
 */
const std::map<std::string, CUniqueDataPoint>& network_info::getUniquePointList()
{
	DO_LOG_DEBUG("");
	return g_mapUniqueDataPoint;
}

/**
 * Get unique device list
 * @return map of unique device
 */
const std::map<std::string, CUniqueDataDevice>& network_info::getUniqueDeviceList()
{
	return g_mapUniqueDataDevice;
}

/**
 * Get point type
 * @param a_type	:[in] point type as string from YAML file
 * @return point point type enum based on string
 */
eEndPointType network_info::CDataPoint::getPointType(const std::string& a_type)
{
	DO_LOG_DEBUG(" Start: Received type: " +
				a_type);
	eEndPointType type;
	if(a_type == "INPUT_REGISTER")
	{
		type = eEndPointType::eInput_Register;
	}
	else if(a_type == "HOLDING_REGISTER")
	{
		type = eEndPointType::eHolding_Register;
	}
	else if(a_type == "COIL")
	{
		type = eEndPointType::eCoil;
	}
	else if(a_type == "DISCRETE_INPUT")
	{
		type = eEndPointType::eDiscrete_Input;
	}
	else
	{
		DO_LOG_INFO(" : Unknown type: " +
					a_type);
		throw YAML::Exception(YAML::Mark::null_mark(), "Unknown Modbus point type");
	}

	DO_LOG_DEBUG("End");
	return type;
}

/**
 * check if input is number
 * @param s	:[in] string might be containing number
 * @return 	true : on success,
 * 			false : on error
 */
bool network_info:: isNumber(string s)
{
    for (uint32_t i32Count = 0; i32Count < s.length(); i32Count++)
        if (isdigit(s[i32Count]) == false)
            return false;

    return true;
}

/**
 * This function is used to store each point information in CDataPoint class
 * Point information will be read from datapoints.yml file
 * @param a_oData		:[in] YAML node to read from
 * @param a_oCDataPoint	:[in] data structure to store read values
 */
void network_info::CDataPoint::build(const YAML::Node& a_oData, CDataPoint &a_oCDataPoint, bool a_bDefaultRealTime)
{
	DO_LOG_DEBUG(" Start");
	// First check optional parameters

	a_oCDataPoint.m_stPollingConfig.m_uiPollFreq = 0;
	a_oCDataPoint.m_stPollingConfig.m_bIsRealTime = false;

	if(0 != globalConfig::validateParam(a_oData["polling"], "realtime", globalConfig::DT_BOOL))
	{
		/* if realtime flag is missing in data points yml file then default value will be used
		 from global configuration */
		a_oCDataPoint.m_stPollingConfig.m_bIsRealTime = a_bDefaultRealTime;
	}
	else
	{
		a_oCDataPoint.m_stPollingConfig.m_bIsRealTime =  a_oData["polling"]["realtime"].as<bool>();
	}

	if(0 != globalConfig::validateParam(a_oData["polling"], "pollinterval", globalConfig::DT_UNSIGNED_INT))
	{
		a_oCDataPoint.m_stPollingConfig.m_uiPollFreq = 0;
	}
	else
	{
		a_oCDataPoint.m_stPollingConfig.m_uiPollFreq =
				a_oData["polling"]["pollinterval"].as<std::uint32_t>();
	}

	if(0 != globalConfig::validateParam(a_oData["attributes"], "datatype", globalConfig::DT_STRING))
	{
		a_oCDataPoint.m_stAddress.m_sDataType = "";
	}
	else
	{
		a_oCDataPoint.m_stAddress.m_sDataType =  a_oData["attributes"]["datatype"].as<std::string>();
	}

	// Check mandatory parameters
	try
	{
		a_oCDataPoint.m_stAddress.m_bIsByteSwap =  false;
		a_oCDataPoint.m_stAddress.m_bIsWordSwap =  false;

		a_oCDataPoint.m_sId = a_oData["id"].as<std::string>();
		a_oCDataPoint.m_stAddress.m_eType = getPointType(a_oData["attributes"]["type"].as<std::string>());
		a_oCDataPoint.m_stAddress.m_iAddress = a_oData["attributes"]["addr"].as<std::int32_t>();
		a_oCDataPoint.m_stAddress.m_iWidth =  a_oData["attributes"]["width"].as<std::int32_t>();
		if (a_oData["attributes"]["byteswap"])
		{
			try
			{
				a_oCDataPoint.m_stAddress.m_bIsByteSwap =  a_oData["attributes"]["byteswap"].as<bool>();
			}
			catch(YAML::Exception &e)
			{
				a_oCDataPoint.m_stAddress.m_bIsByteSwap = false;
				DO_LOG_WARN("ByteSwap value is incorrect. Set to default with exception ::" + std::string(e.what()));
				cout << "ByteSwap value is incorrect. Set to default with exception :: "<< e.what();
			}
		}
		if (a_oData["attributes"]["wordswap"])
		{
			try
			{
				a_oCDataPoint.m_stAddress.m_bIsWordSwap =  a_oData["attributes"]["wordswap"].as<bool>();
			}
			catch(YAML::Exception &e)
			{
				a_oCDataPoint.m_stAddress.m_bIsWordSwap = false;
				DO_LOG_WARN("WordSwap value is incorrect. Set to default." + std::string(e.what()));
				cout << "WordSwap value is incorrect. Set to default with exception :: "<< e.what();
			}
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_FATAL(e.what());
		throw;
	}
	catch(exception &e)
	{
		DO_LOG_FATAL(e.what());
		throw YAML::Exception(YAML::Mark::null_mark(), "key not found");
	}

	DO_LOG_DEBUG("End");
}

/**
 * Print well site info
 * @param a_oWellSite	:[in] well site
 */
void printWellSite(CWellSiteInfo a_oWellSite)
{
	DO_LOG_DEBUG(" Start: wellsite: " +
				a_oWellSite.getID());

	for(auto &objWellSiteDev : a_oWellSite.getDevices())
	{
		DO_LOG_DEBUG(a_oWellSite.getID() +
					"\\" +
					objWellSiteDev.getID());

		for(auto &objPt : objWellSiteDev.getDevInfo().getDataPoints())
		{
			DO_LOG_DEBUG(a_oWellSite.getID() +
						"\\" +
						objWellSiteDev.getID() +
						"\\" +
						objPt.getID());
		}
	}
	DO_LOG_DEBUG("End");
}

/**
 * Build network info based on network type
 * if network type is TCP then this function will read all TCP devices and store it
 * in associated data structures and vice versa for RTU
 */
void network_info::buildNetworkInfo(string a_strNetworkType, string a_strSiteListFileName, string a_strAppId)
{
	DO_LOG_DEBUG(" Start");

	// Check if this function is already called once. If yes, then exit
	if(true == g_bIsStarted)
	{
		std::cout << "already started... return with no action \n";
		DO_LOG_INFO(" This function is already called once. Ignoring this call");
		return;
	}
	// Set the flag to avoid all future calls to this function. 
	// This is done to keep data structures in tact once network is built
	g_bIsStarted = true;
	
	// Set the network type TCP or RTU
	transform(a_strNetworkType.begin(), a_strNetworkType.end(), a_strNetworkType.begin(), ::toupper);

	if(a_strNetworkType == "TCP")
	{
		g_eNetworkType = eNetworkType::eTCP;
	}
	else if(a_strNetworkType == "RTU")
	{
		g_eNetworkType = eNetworkType::eRTU;
	}
	else if(a_strNetworkType == "ALL")
	{
		g_eNetworkType = eNetworkType::eALL;
	}
	else
	{
		DO_LOG_ERROR("Invalid parameter set for Network Type");
		return;
	}

	std::cout << "Network set as: " << (int)g_eNetworkType << std::endl;
	DO_LOG_INFO(" Network set as: " +
				std::to_string((int)g_eNetworkType));
	
	// Following stage is needed only when configuration files are placed in a docker volume
	#ifdef CONFIGFILES_IN_DOCKER_VOLUME
	//std::cout << "Config files are kept in a docker volume\n";
	//DO_LOG_INFO(" Config files are kept in a docker volume");
	
	// get list of well sites
	if(false == _getWellSiteList(a_strSiteListFileName))
	{
		DO_LOG_ERROR(" Site-list could not be obtained");
		return;
	}
	std::vector<CWellSiteInfo> oWellSiteList;
	for(auto &sWellSiteFile: g_sWellSiteFileList)
	{
		if(true == sWellSiteFile.empty())
		{
			std::cout << __func__ <<" : Encountered empty file name. Ignoring";
			DO_LOG_INFO(" : Encountered empty file name. Ignoring");
			continue;
		}
		// Check if the file is already scanned
		std::map<std::string, CWellSiteInfo>::iterator it = g_mapYMLWellSite.find(sWellSiteFile);

		if(g_mapYMLWellSite.end() != it)
		{
			// It means record exists
			std::cout << __func__ << " " << sWellSiteFile <<" : is already scanned. Ignoring";
			DO_LOG_INFO(sWellSiteFile +
						"Already scanned YML file: Ignoring it.");
			continue;
		}

		DO_LOG_INFO(" New YML file: " +
					sWellSiteFile);

		try
		{
			YAML::Node baseNode = CommonUtils::loadYamlFile(sWellSiteFile);

			CWellSiteInfo objWellSite;
			CWellSiteInfo::build(baseNode, objWellSite);
			oWellSiteList.push_back(objWellSite);
			g_mapYMLWellSite.emplace(sWellSiteFile, objWellSite);

			DO_LOG_INFO(" Successfully scanned: " +
						sWellSiteFile +
						": Id = " +
						objWellSite.getID());
		}
		catch(YAML::Exception &e)
		{
			DO_LOG_FATAL(" Ignoring YML:" +
						sWellSiteFile +
						"Error: " +
						e.what());
			// Add this file to error YML files
			g_sErrorYMLs.push_back(sWellSiteFile);
		}
	}

	// Once network information is read, prepare a list of unique points
	// Set variables for unique point listing
	//if(const char* env_p = std::getenv("MY_APP_ID"))
	if(!a_strAppId.empty())
	{
		DO_LOG_INFO(": MY_APP_ID value = " + a_strAppId);
		auto a = (unsigned short) atoi(a_strAppId.c_str());
		a = a & 0x000F;
		g_usTotalCnt = (a << 12);
	}
	else
	{
		DO_LOG_INFO("MY_APP_ID value is not set. Expected values 0 to 16");
		DO_LOG_INFO("Assuming value as 0");
		g_usTotalCnt = 0;
	}
	DO_LOG_INFO(": Count start from = " +g_usTotalCnt);
	for(auto &a: g_mapYMLWellSite)
	{
		populateUniquePointData(g_mapYMLWellSite.at(a.first));
	}

	for(auto &a: oWellSiteList)
	{
		std::cout << "\nNew Well Site\n";
		printWellSite(a);
	}

	#endif
	DO_LOG_DEBUG("End");
}

/**
 * Constructor
 * @param a_sId 		:[in] site id
 * @param a_rWellSite	:[in] well site
 * @param a_rWellSiteDev:[in] well site device
 * @param a_rPoint		:[in] data points
 */
CUniqueDataPoint::CUniqueDataPoint(std::string a_sId, const CWellSiteInfo &a_rWellSite,
				const CWellSiteDevInfo &a_rWellSiteDev, const CDataPoint &a_rPoint) :
				m_uiMyRollID{((unsigned int)g_usTotalCnt)+1}, m_sId{a_sId},
				m_rWellSite{a_rWellSite}, m_rWellSiteDev{a_rWellSiteDev}, m_rPoint{a_rPoint}, m_bIsAwaitResp{false}, m_bIsRT{a_rPoint.getPollingConfig().m_bIsRealTime}
{
	++g_usTotalCnt;
}

/**
 * Constructor
 * @param a_objPt 		:[in] reference CUniqueDataPoint object for copy constructor
 */
CUniqueDataPoint::CUniqueDataPoint(const CUniqueDataPoint &a_objPt) :
				m_uiMyRollID{a_objPt.m_uiMyRollID}, m_sId{a_objPt.m_sId},
				m_rWellSite{a_objPt.m_rWellSite}, m_rWellSiteDev{a_objPt.m_rWellSiteDev}, m_rPoint{a_objPt.m_rPoint}, m_bIsAwaitResp{false}
{
		m_bIsRT.store(a_objPt.m_bIsRT);
}

/**
 * Check if response is received or not for specific point
 * @return 	true : on success,
 * 			false : on error
 */
bool CUniqueDataPoint::isIsAwaitResp() const {
	return m_bIsAwaitResp.load();
}

/**
 * Set the response status for point
 * @param isAwaitResp	:[out] true/false based on response received or not
 */
void CUniqueDataPoint::setIsAwaitResp(bool isAwaitResp) const {
	m_bIsAwaitResp.store(isAwaitResp);
}