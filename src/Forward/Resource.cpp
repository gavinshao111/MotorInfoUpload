/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Resource.cpp
 * Author: 10256
 * 
 * Created on 2017年5月16日, 下午3:37
 */

#include "Resource.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/make_shared.hpp>
#include <string>
#include <stdexcept>
#include <fstream>

using namespace std;
const static string iniPath = "configurtion.ini";
const static string SecAccessToPublicServer = "AccessToPublicServer";
const static string SecEnterpriseServiceForVehicle = "EnterpriseServiceForVehicle";
const static string SecDataPacket = "DataPacket";
const static string SecConnectToPublicServer = "ConnectToPublicServer";

Resource* Resource::s_resource = NULL;

Resource::Resource()
//:
//PublicServerIp("127.0.0.1"),
//PublicServerPort(1234),
//PublicServerUserName("LEAP"),
//PublicServerPassword("LEAP"),
//EncryptionAlgorithm(enumEncryptionAlgorithm::null),
//EnterprisePlatformTcpServicePort(18885),
//ReadResponseTimeOut(10),
//LoginTimes(3),
//LoginIntervals(60),
//LoginIntervals2(1800),
////城市邮政编码(310000) + VIN前三位（123?）+ 两位自定义数据(00?) + "000000"
//PaltformId("31000012300000000"),
//ReSetupPeroid(1),
//MaxSerialNumber(65531),
//TcpConnWithPublicPlatform(PublicServerIp, PublicServerPort),
//VehicleDataQueue(1024),
//HeartBeatCycle(10) 
{
    fstream iniFile;
    iniFile.open(iniPath, ios::in);
    if (!iniFile)
        throw runtime_error("couldn't open " + iniPath);
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(iniPath, pt);
    
    string section = "AccessToPublicServer";
    PublicServerIp = pt.get<string>(section + ".Address", "");
    PublicServerPort = pt.get<int>(section + ".TcpPort", 1);
    PublicServerUserName = pt.get<string>(section + ".UserName", "");
    PublicServerPassword = pt.get<string>(section + ".Password", "");
    
    section.assign("EnterpriseServiceForVehicle");
    EnterprisePlatformTcpServicePort = pt.get<int>(section + ".TcpPort", 1);
    HeartBeatCycle = pt.get<int>(section + ".HeartBeatCycle", 1);
    int MaxVehicleDataQueueSize = pt.get<int>(section + ".MaxVehicleDataQueueSize", 1);
    
    section.assign("DataPacket");
    EncryptionAlgorithm = (enumEncryptionAlgorithm)pt.get<int>(section + ".EncryptionAlgorithm", 1);
    PaltformId = pt.get<string>(section + ".PaltformId", "");
    MaxSerialNumber = pt.get<int>(section + ".MaxSerialNumber", 1);
    
    section.assign("ConnectToPublicServer");
    ReadResponseTimeOut = pt.get<int>(section + ".ReadResponseTimeOut", 1);
    LoginTimes = pt.get<int>(section + ".LoginTimes", 1);
    LoginIntervals = pt.get<int>(section + ".LoginIntervals", 1);
    LoginIntervals2 = pt.get<int>(section + ".LoginIntervals2", 1);
    ReSetupPeroid = pt.get<int>(section + ".ReSetupPeroid", 1);
    UploadChannelNumber = pt.get<int>(section + ".UploadChannelNumber", 1);
    Mode = pt.get<int>(section + ".Mode", 1);
    VehicleDataQueueSPtr = boost::make_shared<blockqueue::BlockQueue<BytebufSPtr_t>>(MaxVehicleDataQueueSize);
}

Resource* Resource::GetResource() {
    if (s_resource == NULL)
        s_resource = new Resource();
    return s_resource;
}

Resource::~Resource() {
}
