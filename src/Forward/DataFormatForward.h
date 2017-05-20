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
#include "ByteBuffer.h"

#define VINLEN 17
#define MAXPACK_LENGTH 512

namespace responseflag {

    typedef enum {
        success = 1,
        error = 2,
        vinrepeat = 3,
    } enumResponseFlag;
}

typedef enum {
    null = 1,
    rsa = 2,
    aes128 = 3,
} enumEncryptionAlgorithm;

typedef enum {
    init = 0,
    vehicleLogin = 1,
    vehicleSignalDataUpload = 2,
    reissueUpload = 3,
    vehicleLogout = 4,
    platformLogin = 5,
    platformLogout = 6,
    heartBeat = 7,        
} enumCmdCode;

typedef struct {
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;

    int DataReceivePort;
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

} StaticResourceForward1;

#pragma pack(1)

typedef struct TimeForward {
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} TimeForward_t;

typedef struct DataPacketHeader {
    uint8_t startCode[2];
    uint8_t cmdId;
    uint8_t responseFlag;
    uint8_t vin[VINLEN];
    uint8_t encryptionAlgorithm;
    uint16_t dataUnitLength;

    DataPacketHeader() {
        startCode[0] = '#';
        startCode[1] = '#';
        responseFlag = 0xfe;
    }

} DataPacketHeader_t;

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

struct VehicleDataStructInfo {
    const static uint8_t CBV_typeCode = 1;
    const static size_t CBV_size = 20;
    const static uint8_t DM_typeCode = 2;
    const static size_t DM_size = 13;
    const static uint8_t L_typeCode = 5;
    const static size_t L_size = 9;
    const static uint8_t EV_typeCode = 6;
    const static size_t EV_size = 14;
    const static uint8_t A_typeCode = 7;
    const static uint8_t RESV_typeCode = 8;
    const static uint8_t REST_typeCode = 9;
};

typedef boost::shared_ptr<bytebuf::ByteBuffer> BytebufSPtr_t;

#endif /* DATAFORMATFORWARD_H */

