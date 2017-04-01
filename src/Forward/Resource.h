/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Resource.h
 * Author: 10256
 *
 * Created on 2017年3月29日, 下午7:55
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <bits/shared_ptr.h>
#include "DataFormatForward.h"
#include "DataPacketForward.h"
#include "GSocketClient.h"
#include "../BlockQueue.h"

typedef struct {
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;
    std::string MQServerUrl;
    std::string MQTopic;
    std::string MQClientID;
    std::string MQServerUserName;
    std::string MQServerPassword;

    /*
     * 等待公共平台回应超时（ReadResponseTimeOut），
     */
    size_t ReadResponseTimeOut;
    /* 
     * 登录没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 3（LoginTimes）次登入没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    size_t LoginIntervals;
    size_t LoginIntervals2;
    size_t LoginTimes;
    /*
     * 校验失败：
     * 国标：重发本条实时信息（应该是调整后重发）。每隔1min（CarDataResendIntervals）重发，最多发送3(MaxSendCarDataTimes)次
     * 国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
     * 由于无法做到实时调整，只能忽略错误的那条实时信息，继续发下一条。
     * 未响应：
     * 国标2：如客户端平台未收到应答，平台重新登入
     */
    size_t CarDataResendIntervals;
    size_t MaxSendCarDataTimes;

    enumEncryptionAlgorithm EncryptionAlgorithm;
    std::shared_ptr<blockqueue::BlockQueue<DataPacketForward*>> dataQueue;
//    blockqueue::BlockQueue<DataPacketForward*>* dataQueue;

    /* 平台登入登出的唯一识别号，
     * 城市邮政编码 + VIN前三位（地方平台使用GOV）+ 两位自定义数据 + "000000"
     */
    std::string PaltformId;
    size_t ReSetupPeroid; // tcp 中断后每隔几秒重连
    std::shared_ptr<gavinsocket::GSocketClient> tcpConnection;
//    gavinsocket::GSocketClient* tcpConnection;
    size_t MaxSerialNumber;

} StaticResourceForward;

#endif /* RESOURCE_H */

