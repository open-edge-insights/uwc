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

#include <iostream>
#include <atomic>
#include <map>
#include <algorithm>
#include <arpa/inet.h>
#include "NetworkInfo.hpp"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/yaml.h"
#include "YamlUtil.hpp"
#include "ConfigManager.hpp"

#include "EnvironmentVarHandler.hpp"

using namespace network_info;
using namespace CommonUtils;

// Unnamed namespace to define globals 
namespace  
{
eNetworkType g_eNetworkType{eNetworkType::eALL};
std::atomic<bool> g_bIsStarted{false};
std::map<std::string, CWellSiteInfo> g_mapYMLWellSite;
std::map<std::string, CRTUNetworkInfo> g_mapRTUNwInfo;
std::map<std::string, CUniqueDataPoint> g_mapUniqueDataPoint;
std::map<std::string, CUniqueDataDevice> g_mapUniqueDataDevice;
std::map<std::string, CDeviceInfo> g_mapDeviceInfo;
std::map<std::string, CDataPointsYML> g_mapDataPointsYML;
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

		auto &oPointList = objWellSiteDev.getDevInfo().getDataPoints();
		for(auto &objPt : oPointList)
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


std::vector<std::string> g_sWellSiteFileList;

/**
 * Get well site list
 * @param a_strSiteListFileName :[in] well site listing file
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

/**
 * Reads datapoints yml file and builds an object of CDataPointsYML if needed
 * @param a_sDataPointsYML:[in] Datapoints YAML file name
 * @return Object of CDataPointsYML
 */
CDataPointsYML& getDataPointsYML(const std::string& a_sDataPointsYML)
{
	try
	{
		// Check if object for this YML is already present
		auto itr = g_mapDataPointsYML.find(a_sDataPointsYML);
		if(itr != g_mapDataPointsYML.end())
		{
			return itr->second;
		}
		// Data Poinst YML object not found. Insert a new one in map.
		std::cout << "YML file: " << a_sDataPointsYML << std::endl;
		YAML::Node node = CommonUtils::loadYamlFile(a_sDataPointsYML);

		DO_LOG_INFO("pointlist found: " + a_sDataPointsYML);
		{
			CDataPointsYML oDataPointsYML{a_sDataPointsYML};
			g_mapDataPointsYML.insert(std::pair <std::string, CDataPointsYML> (a_sDataPointsYML, oDataPointsYML));
		}

		// Get object for processsing
		auto& orPointList = g_mapDataPointsYML.at(a_sDataPointsYML);
		for (auto it : node)
		{
			if(it.first.as<std::string>() == "file" && it.second.IsMap())
			{
				try
				{
					std::string sVersion = it.second["version"].as<std::string>();
					orPointList.setVersion(sVersion);
					std::cout << sVersion << ": data points YML version\n";
					continue;
				}
				catch(std::exception &e)
				{
					std::cout << ": version not found in data points YML \n";
				}
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
						if(0 == orPointList.addDataPoint(objCDataPoint))
						{
							DO_LOG_INFO("Added point with id: " + objCDataPoint.getID());
						}
						else
						{
							DO_LOG_ERROR("Ignoring duplicate point ID from polling :"+ objCDataPoint.getID());
							std::cout << "ERROR: Ignoring duplicate point ID from polling :"<< objCDataPoint.getID() <<std::endl;
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
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << __func__<<" Exception :: " << e.what()<<std::endl;
		throw;
	}
	return g_mapDataPointsYML.at(a_sDataPointsYML);
}

/**
 * Reads base parameters needed to create a DeviceInfo object
 * @param a_oDevInfoYML:[in] YAML data node to read from
 * @param a_sDevName:[out] String to store device name
 * @param a_sPointListYML:[out] String to store pointlist YML name
 */
void getBaseParamsForDeviceInfo(const YAML::Node& a_oDevInfoYML, std::string &a_sDevName, std::string &a_sPointListYML)
{
	bool bIsNameFound = false;
	bool bIsPointsYMLFound = false;
	try
	{
		for (auto test : a_oDevInfoYML)
		{
			if(test.first.as<std::string>() == "device_info")
			{
				try
				{
					a_sDevName = test.second["name"].as<std::string>();
					bIsNameFound = true;
					continue;
				}
				catch(std::exception &e)
				{
					DO_LOG_FATAL(e.what());
					throw YAML::Exception(YAML::Mark::null_mark(), "name key not found in device_info");
				}				
			}
			if(test.first.as<std::string>() == "pointlist")
			{
				a_sPointListYML = test.second.as<std::string>();
				bIsPointsYMLFound = true;
				continue;
			}
		}
		if(false == bIsNameFound)
		{
			DO_LOG_ERROR(" Device without name is found. Ignoring this device.");
			throw YAML::Exception(YAML::Mark::null_mark(), "name or device_info key not found");
		}
		if(false == bIsPointsYMLFound)
		{
			DO_LOG_ERROR(" Device without pointlist is found. Ignoring this device.");
			throw YAML::Exception(YAML::Mark::null_mark(), "pointlist key not found");
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << __func__<<" Exception :: " << e.what()<<std::endl;
		throw;
	}
}

/**
 * Builds an object of CDeviceInfo if needed
 * @param a_oWellSiteDevData:[in] YAML data node to read from
 * @return Object of CDeviceInfo
 */
CDeviceInfo& getDeviceInfo(const YAML::Node& a_oWellSiteDevData)
{
	bool bIsDevRefPresent = false;
	std::string sDevInfoYML{""};
	try
	{
		for (auto it : a_oWellSiteDevData)
		{
			std::cout << "getDeviceInfo: key: " << it.first.as<std::string>() << std::endl;
			if(it.first.as<std::string>() == "deviceinfo")
			{
				sDevInfoYML = it.second.as<std::string>();
				std::cout << "Device Info YML file: " << sDevInfoYML << std::endl;
				// Check if object for this device info YML is already present
				auto itr = g_mapDeviceInfo.find(sDevInfoYML);
				if(itr != g_mapDeviceInfo.end())
				{
					return itr->second;
				}
				// Device info object not found. Insert a new one in map.
				YAML::Node node = CommonUtils::loadYamlFile(sDevInfoYML);
				std::string sDevName{""};
				std::string sDataPointsYML{""};
				getBaseParamsForDeviceInfo(node, sDevName, sDataPointsYML);
				CDataPointsYML &rDataPointsYML = getDataPointsYML(sDataPointsYML);
				CDeviceInfo oDevInfo{sDevInfoYML, sDevName, rDataPointsYML};
				g_mapDeviceInfo.insert(std::pair <std::string, CDeviceInfo> (sDevInfoYML, oDevInfo));
				bIsDevRefPresent = true;
			}
		}
		if(false == bIsDevRefPresent)
		{
			DO_LOG_ERROR(" Device information is not found. Ignoring this well device.");
			std::cout << __func__ << " Device information is not found. Ignoring this well device." << std::endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "deviceinfo not found");
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << __func__<<" Exception :: " << e.what()<<std::endl;
		throw;
	}
	return g_mapDeviceInfo.at(sDevInfoYML);
}

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
						CDeviceInfo& rDevInfo = getDeviceInfo(nodes);
						CWellSiteDevInfo objWellsiteDev{rDevInfo};
						CWellSiteDevInfo::build(nodes, objWellsiteDev);
						int i32RetVal = a_oWellSite.addDevice(objWellsiteDev);
						if(0 == i32RetVal)
						{
							DO_LOG_INFO("Added device with id: " + 
									objWellsiteDev.getID());
						}
						else if(-1 == i32RetVal)
						{
							DO_LOG_ERROR("Ignoring device with id : " +
									objWellsiteDev.getID() +
									", since this point name is already present. Ignore this point.");
							std::cout << "Ignoring device with id : " << objWellsiteDev.getID() << ", since this point name is already present. Ignore this point."<< std::endl;
						}
						else if(-2 == i32RetVal)
						{
							DO_LOG_ERROR("Ignoring device with id : " +
									objWellsiteDev.getID() +
									", since Device type and network type are not matching.");
							std::cout << "Ignoring device with id : " << objWellsiteDev.getID() << ", since Device type and network type are not matching."<< std::endl;
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
			std::cout << "Site without id is found. Ignoring this site."<<std::endl;
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
		DO_LOG_INFO(" baudrate = " + std::to_string(a_oNwInfo.getBaudRate()));
		DO_LOG_INFO(" com_port_name = " + a_oNwInfo.getPortName());
		DO_LOG_INFO(" parity = " + a_oNwInfo.getParity());
		DO_LOG_INFO(" interframe_delay = " + std::to_string(a_oNwInfo.getInterframeDelay()));
		DO_LOG_INFO(" response_timeout = " + std::to_string(a_oNwInfo.getResTimeout()));


		std::cout << "RTU network info parameters..." << std::endl;
		std::cout << " baudrate = " + std::to_string(a_oNwInfo.getBaudRate()) << std::endl;
		std::cout << " com_port_name = " + a_oNwInfo.getPortName() << std::endl;
		std::cout << " parity = " +  a_oNwInfo.getParity() << std::endl;
		std::cout << " interframe_delay = " +  std::to_string(a_oNwInfo.getInterframeDelay()) << std::endl;
		std::cout << " response_timeout = " +  std::to_string(a_oNwInfo.getResTimeout()) << std::endl;

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

				if(tempMap.at("protocol") == "PROTOCOL_RTU")
				{
					try
					{
						a_oWellSiteDevInfo.m_stAddress.m_stRTU.m_uiSlaveId = atoi(tempMap.at("slaveid").c_str());

						a_oWellSiteDevInfo.m_stAddress.m_NwType = network_info::eNetworkType::eRTU;
						bIsProtocolPresent = true;
						DO_LOG_INFO(" : RTU protocol: ");
					}
					catch(std::exception &e)
					{
						DO_LOG_FATAL(e.what());
						std::cout << __func__ << "Required keys not found in PROTOCOL_RTU" << std::endl;
						throw YAML::Exception(YAML::Mark::null_mark(), "Required keys not found in PROTOCOL_RTU");
					}
				}
				else if(tempMap.at("protocol") == "PROTOCOL_TCP")
				{
					try
					{
						if(tempMap.at("ipaddress") == "" || tempMap.at("port") == "" || tempMap.at("unitid") == "")
						{
							std::cout << " ERROR:: ipaddress or port or unitid cannot be empty" <<std::endl;
							std::cout << " Given parameters:: " <<std::endl;
							std::cout << " ipaddress:: " << tempMap.at("ipaddress")<<std::endl;
							std::cout  << " port:: " << tempMap.at("port")<<std::endl;
							std::cout  << " unitid:: " <<tempMap.at("unitid")<<std::endl;
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
							std::cout << "ERROR : IP address is invalid!!" <<tempMap.at("ipaddress") <<std::endl;
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
					catch(std::exception &e)
					{
						DO_LOG_FATAL(e.what());
						std::cout << "Required keys not found in PROTOCOL_TCP"<<std::endl;
						throw YAML::Exception(YAML::Mark::null_mark(), "Required keys not found in PROTOCOL_TCP");
					}
				}
				else
				{
					// error
					DO_LOG_ERROR(" : Unknown protocol: " +
							tempMap.at("protocol"));
					std::cout << __func__<< " : Unknown protocol: " << tempMap.at("protocol") << std::endl;
					throw YAML::Exception(YAML::Mark::null_mark(), "Unknown protocol found");
				}
			}
		}
		if(false == bIsIdPresent)
		{
			DO_LOG_ERROR(" Site device without id is found. Ignoring this well device.");
			std::cout << __func__ << " Site device without id is found. Ignoring this well device."<<std::endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Id key not found");

		}
		if(false == bIsProtocolPresent)
		{
			DO_LOG_ERROR(" Site device without protocol is found. Ignoring it.");
			std::cout << __func__ << " Site device without protocol is found. Ignoring it.."<<std::endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Protocol key not found");
		}
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_ERROR(e.what());
		std::cout << __func__<<" Exception :: " << e.what()<<std::endl;
		throw;
	}

	DO_LOG_DEBUG("End");
}

/**
 * Add data point in m_DataPointList
 * @param a_oDataPoint :[in] data point values to be add
 * if DataPoint id is same in one file then it will be ignored
 * @return 	0 : on success,
 * 			-1 : on error (if point id is duplicate in datapoints file then this point will be ignored)
 */
int network_info::CDataPointsYML::addDataPoint(CDataPoint &a_oDataPoint)
{
	// Search whether given point name is already present
	// If present, ignore the datapoint
	// If not present, add it

	DO_LOG_DEBUG("Start: To add DataPoint - " +	a_oDataPoint.getID());
	for(auto oDataPoint: m_DataPointList)
	{
		if(0 == oDataPoint.getID().compare(a_oDataPoint.getID()))
		{
			DO_LOG_ERROR(a_oDataPoint.getID() + 
					" : Already present DataPoint with id. Ignored new instance.");
			// This point name is already present. Ignore this point
			return -1;
		}
	}
	m_DataPointList.push_back(a_oDataPoint);

	DO_LOG_INFO("Added DataPoint - " + a_oDataPoint.getID() + ". Total points: " + std::to_string(m_DataPointList.size()));
	return 0;
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
 * Get data points YML file listing objects
 * @return map of data points YML file listing objects
 */
const std::map<std::string, CDataPointsYML>& network_info::getDataPointsYMLList()
{
	return g_mapDataPointsYML;
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
		DO_LOG_ERROR(" : Unknown type: " +
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
	
	// Assign default dataPersist value as false in case when dataPersist key is not available in datapoints.yml for the datapoint.
	if (0 != globalConfig::validateParam(a_oData["attributes"], "dataPersist", globalConfig::DT_BOOL))
	{
		a_oCDataPoint.setDataPersist(false);
	}
	// Assign dataPersist value as specified in datapoints.yml for the datapoint.
	else
	{
		bool isPersist = a_oData["attributes"]["dataPersist"].as<bool>();
		a_oCDataPoint.setDataPersist(isPersist);
	}
	
	// Check mandatory parameters
	try
	{
		a_oCDataPoint.m_stAddress.m_bIsByteSwap =  false;
		a_oCDataPoint.m_stAddress.m_bIsWordSwap =  false;
		a_oCDataPoint.m_stAddress.m_dScaleFactor = globalConfig::CGlobalConfig::getInstance().getDefaultScaleFactor();
		a_oCDataPoint.m_sId = a_oData["id"].as<std::string>();
		a_oCDataPoint.m_stAddress.m_eType = getPointType(a_oData["attributes"]["type"].as<std::string>());
		a_oCDataPoint.m_stAddress.m_iAddress = a_oData["attributes"]["addr"].as<std::int32_t>();

		if((a_oCDataPoint.m_stAddress.m_eType == eEndPointType::eCoil) && (a_oData["attributes"]["width"].as<std::int32_t>() != 1))
		{
			DO_LOG_ERROR("Invalid width value is specified in yml file. hence ignoring the point : " + a_oCDataPoint.m_sId);
			std::cout << "ERROR:: Invalid width value is specified in yml file. hence ignoring the point : "<< a_oCDataPoint.m_sId << std::endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Width should be 1 for COIL type");
		}
		else
		{
			a_oCDataPoint.m_stAddress.m_iWidth =  a_oData["attributes"]["width"].as<std::int32_t>();
		}

		// validate width
		if (a_oCDataPoint.m_stAddress.m_iWidth <= 0)
		{
			DO_LOG_ERROR("Invalid width value is specified in yml file. hence ignoring the point : " + a_oCDataPoint.m_sId);
			std::cout << "ERROR:: Invalid width value is specified in yml file. hence ignoring the point : "<< a_oCDataPoint.m_sId << std::endl;
			throw YAML::Exception(YAML::Mark::null_mark(), "Invalid value for width");
		}

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
				std::cout << "ByteSwap value is incorrect. Set to default with exception :: "<< e.what();
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
				std::cout << "WordSwap value is incorrect. Set to default with exception :: "<< e.what();
			}
		}
		try
		{
			if (a_oData["attributes"]["scalefactor"])
			{				
				if(a_oData["attributes"]["scalefactor"].as<double>() == 0)
				{								
					a_oCDataPoint.m_stAddress.m_dScaleFactor = globalConfig::CGlobalConfig::getInstance().getDefaultScaleFactor();
				}
				else
				{
					a_oCDataPoint.m_stAddress.m_dScaleFactor =  a_oData["attributes"]["scalefactor"].as<double>();				
				}
			}
		}
		catch(YAML::Exception &e)
		{
			a_oCDataPoint.m_stAddress.m_dScaleFactor = globalConfig::CGlobalConfig::getInstance().getDefaultScaleFactor();
			DO_LOG_WARN(" Scale factor key is not present. Set to default." + std::string(e.what()));			
		}
		
	}
	catch(YAML::Exception &e)
	{
		DO_LOG_FATAL(e.what());
		throw;
	}
	catch(std::exception &e)
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
	DO_LOG_INFO(": Count start from = " + std::to_string(g_usTotalCnt));
	for(auto &a: g_mapYMLWellSite)
	{
		populateUniquePointData(g_mapYMLWellSite.at(a.first));
	}

	for(auto &a: oWellSiteList)
	{
		std::cout << "\nNew Well Site\n";
		printWellSite(a);
	}

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
									bool CUniqueDataPoint::isIsAwaitResp() const
									{
										return m_bIsAwaitResp.load();
									}

									/**
									 * Set the response status for point
									 * @param isAwaitResp	:[out] true/false based on response received or not
									 */
									void CUniqueDataPoint::setIsAwaitResp(bool isAwaitResp) const
									{
										m_bIsAwaitResp.store(isAwaitResp);
									}
