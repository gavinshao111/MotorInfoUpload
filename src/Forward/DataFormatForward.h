/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataFormatForward.h
 * Author: 10256
 *
 * Created on 2017年3月8日, 下午7:53
 */

#ifndef DATAFORMATFORWARD_H
#define DATAFORMATFORWARD_H

#include <string>
#include <boost/shared_ptr.hpp>
#include "DataPacketForward.h"
#include "GSocketClient.h"
#include "../BlockQueue.h"
namespace responseflag {

    typedef enum {
        success = 1,
        error = 2,
        vinrepeat = 3,
    } enumResponseFlag;
}

typedef struct {
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;
    std::string MQServerUrl;
    std::string MQTopicForUpload;
    std::string MQTopicForResponse;
    
    std::string MQClientID;
    std::string MQServerUserName;
    std::string MQServerPassword;
    std::string pathOfPrivateKey;
    std::string pathOfServerPublicKey;

    /*
     * 等待公共平台回应超时（ReadResponseTimeOut），
     */
    size_t ReadResponseTimeOut;
    /* 
     * 登入没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 3（LoginTimes）次登入没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    size_t LoginIntervals;
    size_t LoginIntervals2;
    size_t LoginTimes;
    /*
     * 校验失败：
     *  国标：重发本条实时信息（应该是调整后重发）。每隔1min（CarDataResendIntervals）重发，最多发送3(MaxSendCarDataTimes)次
     *  国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
     *  由于平台无法做到实时调整，只能转发错误回应给车机，继续发下一条。所以不再重发本条实时信息，等车机调整后直接转发。
     * 未响应：
     *  国标2：如客户端平台未收到应答，平台重新登入
     */
//    size_t CarDataResendIntervals;
//    size_t MaxSendCarDataTimes;

    enumEncryptionAlgorithm EncryptionAlgorithm;
    
//    blockqueue::BlockQueue<DataPacketForward*>* dataQueue;

    /* 平台登入登出的唯一识别号，
     * 城市邮政编码 + VIN前三位（地方平台使用GOV）+ 两位自定义数据 + "000000"
     */
    std::string PaltformId;
    size_t ReSetupPeroid; // tcp 中断后每隔几秒重连
    
//    gavinsocket::GSocketClient* tcpConnection;
    size_t MaxSerialNumber;

} StaticResourceForward;

#pragma pack(1)

typedef struct TimeForward {
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} TimeForward_t;

typedef struct LoginDataForward {
    // 登入登出流水号 may be we should store it in DB.
    DataPacketHeader_t header;
    TimeForward_t time;
    uint16_t serialNumber;
    uint8_t username[12];
    uint8_t password[20];
    uint8_t encryptionAlgorithm; // header 也有这个字段，重复
    uint8_t checkCode;
} LoginDataForward_t;

typedef struct LogoutDataForward {
    DataPacketHeader_t header;
    TimeForward_t time;
    uint16_t serialNumber;
    uint8_t checkCode;
} LogoutDataForward_t;
#pragma pack()

typedef boost::shared_ptr<bytebuf::ByteBuffer> BytebufSPtr_t;
typedef blockqueue::BlockQueue<BytebufSPtr_t> DataQueue_t;
typedef gavinsocket::GSocketClient TcpConn_t;

#endif /* DATAFORMATFORWARD_H */

