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

#include <cstdlib>

Resource* Resource::s_resource = NULL;

Resource::Resource() :
PublicServerIp("10.34.16.181"),
PublicServerPort(1234),
PublicServerUserName("LEAP"),
PublicServerPassword("LEAP"),
EncryptionAlgorithm(enumEncryptionAlgorithm::null),
ThePlatformTcpServicePort(8885),
MQServerUrl("ssl://120.26.86.124:8883"),
// topic format: /carid/lpcloud/candata/gbt32960/response
MQTopicForResponse("/lpcloud/candata/gbt32960/response"),
MQClientID("VehicleInfoForwardPlatform"),
MQServerUserName("easydarwin"),
MQServerPassword("123456"),
ReadResponseTimeOut(10),
LoginTimes(3),
LoginIntervals(60),
LoginIntervals2(1800),
//城市邮政编码(310000) + VIN前三位（123?）+ 两位自定义数据(00?) + "000000"
PaltformId("31000012300000000"),
ReSetupPeroid(1),
MaxSerialNumber(65531),
TcpConnWithPublicPlatform(PublicServerIp, PublicServerPort),
VehicleDataQueue(1024),
HeartBeatCycle(10) {
    std::string pathOfED = getenv("ED");
    pathOfServerPublicKey += pathOfED;
    pathOfServerPublicKey.append("/emqtt.pem");
    pathOfPrivateKey += pathOfED;
    pathOfPrivateKey.append("/emqtt.key");
}

Resource* Resource::GetResource() {
    if (s_resource == NULL)
        s_resource = new Resource();
    return s_resource;
}

Resource::~Resource() {
}
