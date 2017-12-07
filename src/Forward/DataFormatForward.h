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
    realtimeUpload = 2,
    reissueUpload = 3,
    vehicleLogout = 4,
    platformLogin = 5,
    platformLogout = 6,
    heartBeat = 7,
} enumCmdCode;

typedef enum {
    hex = 0,
    dec = 1,
    oct = 2,
    bin = 3,
} enumSystem;

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

    DataPacketHeader();
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
    
    LoginDataForward();
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

typedef boost::shared_ptr<bytebuf::ByteBuffer> boost_bytebuf_sptr;

#endif /* DATAFORMATFORWARD_H */

