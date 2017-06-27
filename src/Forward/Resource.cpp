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

Resource* Resource::s_resource = NULL;

Resource::Resource() :
PublicServerIp("10.34.16.94"),
PublicServerPort(1234),
PublicServerUserName("LEAP"),
PublicServerPassword("LEAP"),
EncryptionAlgorithm(enumEncryptionAlgorithm::null),
ThePlatformTcpServicePort(18885),
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
}

Resource* Resource::GetResource() {
    if (s_resource == NULL)
        s_resource = new Resource();
    return s_resource;
}

Resource::~Resource() {
}
