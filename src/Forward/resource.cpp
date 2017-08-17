/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   resource.cpp
 * Author: 10256
 * 
 * Created on 2017年5月16日, 下午3:37
 */

#include "resource.h"
#include "Constant.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/make_shared.hpp>
#include <string>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include "utility.h"
#include "logger.h"

using namespace std;

resource* resource::s_resource = NULL;

resource::resource() {
    fstream iniFile;
    iniFile.open(Constant::iniPath, ios::in);
    if (!iniFile)
        throw runtime_error("couldn't open " + Constant::iniPath);
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(Constant::iniPath, pt);

    string section = "AccessToPublicServer";
    publicServerIp = pt.get<string>(section + ".Address", "");
    publicServerPort = pt.get<int>(section + ".TcpPort", 1);
    publicServerUserName = pt.get<string>(section + ".UserName", "");
    publicServerPassword = pt.get<string>(section + ".Password", "");

    section.assign("EnterpriseServiceForVehicle");
    enterprisePlatformTcpServicePort = pt.get<int>(section + ".TcpPort", 1);
    heartBeatCycle = pt.get<int>(section + ".HeartBeatCycle", 1);
    int MaxVehicleDataQueueSize = pt.get<int>(section + ".MaxVehicleDataQueueSize", 1);
    vehicleDataQueueSPtr = boost::make_shared<blockqueue::BlockQueue < BytebufSPtr_t >> (MaxVehicleDataQueueSize);
    string vinAllowed = pt.get<string>(section + ".VinAllowed", "");
    vinAllowedArray = gutility::str_split(vinAllowed, ',');
    section.assign("DataPacket");
    encryptionAlgorithm = (enumEncryptionAlgorithm) pt.get<int>(section + ".EncryptionAlgorithm", 1);
    paltformId = pt.get<string>(section + ".PaltformId", "");
    maxSerialNumber = pt.get<int>(section + ".MaxSerialNumber", 1);

    section.assign("ConnectToPublicServer");
    readResponseTimeOut = pt.get<int>(section + ".ReadResponseTimeOut", 1);
    loginTimes = pt.get<int>(section + ".LoginTimes", 1);
    loginIntervals = pt.get<int>(section + ".LoginIntervals", 1);
    loginIntervals2 = pt.get<int>(section + ".LoginIntervals2", 1);
    reSetupPeroid = pt.get<int>(section + ".ReSetupPeroid", 1);

    section.assign("Runing");
    uploadChannelNumber = pt.get<int>(section + ".UploadChannelNumber", 1);
    mode = pt.get<int>(section + ".Mode", 1);
    system = (enumSystem) pt.get<int>(section + ".System", 1);
    
    string systemStr;
    switch (system) {
        case enumSystem::bin:
            systemStr = "binary";
            break;
        case enumSystem::oct:
            systemStr = "octal";
            break;
        case enumSystem::dec:
            systemStr = "decimal";
            break;
        case enumSystem::hex:
            systemStr = "hexadecimal";
            break;
        default:
            throw runtime_error("unrecognized system enumeration: " + to_string((int)system));
    }
    // 输出到日志 vin - collect time - send time - data(decimal)   
    GREPORT << setiosflags(ios::left)
            << setw(21) << setfill(' ') << "vin"
            << setw(23) << setfill(' ') << "collect_time"
            << setw(23) << setfill(' ') << "send_time"
            << systemStr << " data\n";
}

resource* resource::getResource() {
    if (s_resource == NULL)
        s_resource = new resource();
    return s_resource;
}

resource::~resource() {
}
