/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constant.cpp
 * Author: 10256
 * 
 * Created on 2017年6月30日, 下午2:47
 */

#include "Constant.h"
using namespace std;

const string Constant::cmdPlatformLoginStr("platform login");
const string Constant::cmdPlatformLogoutStr("platform logout");
const string Constant::cmdVehicleLoginStr("vehicle login");
const string Constant::cmdVehicleLogoutStr("vehicle logout");
const string Constant::cmdRealtimeUploadStr("realtime upload");
const string Constant::cmdReissueUploadStr("reissue upload");
const string Constant::vinInital("Vin not set");
const size_t Constant::maxUploadChannelNum(100);
const string Constant::iniPath("configurtion.ini");
const string Constant::defaultTimeFormat("%Y-%m-%d %H:%M:%S");  // "%d-%02d-%02d %02d:%02d:%02d"
const string Constant::timeFormatForFile("%Y%m%d_%H%M%S");
