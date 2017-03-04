/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataFormat.h
 * Author: 10256
 *
 * Created on 2017年1月9日, 上午11:22
 */

#ifndef DATAFORMAT_H
#define DATAFORMAT_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "ByteBuffer.h"
#include <time.h>
#include "driver.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "statement.h"
#include "BlockQueue.h"
#include <assert.h>
#include <mutex>
#include <condition_variable>
#include <map>
#include "DataPtrLen.h"

#define BUF_SIZE 1024
#define INFO_SIZE 1024*5
#define VINLEN 17
#define TIMEFORMAT "%Y-%m-%d %H:%M:%S"
#define TIMEFORMAT_ "%d-%02d-%02d %02d:%02d:%02d"
#define TIMEOUT 1
#define OK 0

#define TEST true
#define DEBUG true

class CarData;
typedef std::map<std::string, CarData*> CarDataMap;

typedef enum {
    null = 1,
    rsa = 2,
    aes128 = 3,
} enumEncryptionAlgorithm;

typedef enum {
    carSignalDataUpload = 2,
    reissueUpload = 3,
    platformLogin = 5,
    platformLogout = 6,
} enumCmdCode;


#define ALARMDATALEN 6
#define MAXDATAUNITLEN ALARMDATALEN+66

#define PARKANDCHARGE 1

// a complete data packet is DataPacketHeader_t + ******Data_t + uint8_t checkCode

#pragma pack(1)

/*
 * DataPacketHeader和checkCode在sender中生成，CarSignalData在Generator生成，并存在内存中不断被更新，需要上传时，生成copy，可能需要将DM数据去除
 */

typedef struct DataPacketHeader {
    uint8_t startCode[2];
    uint8_t CmdId;
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

typedef struct LoginData {
    // 登入登出流水号 may be we should store it in DB.
    
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t serialNumber;
    uint8_t username[12];
    uint8_t password[20];
    uint8_t encryptionAlgorithm;    // header 也有这个字段，重复

} LoginData_t;

typedef struct LogoutData {
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t serialNumber;

} LogoutData_t;

typedef struct CarSignalData {
    //uint8_t dataUnit[72];
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;

    // CompletelyBuildedVehicle
    uint8_t CBV_typeCode;
    uint8_t CBV_vehicleStatus; // 1
    uint8_t CBV_chargeStatus; // 1
    uint8_t CBV_runningMode; // 1
    uint16_t CBV_speed; // 2
    uint32_t CBV_mileages; // 4
    uint16_t CBV_totalVoltage; // 2
    uint16_t CBV_totalCurrent; // 2
    uint8_t CBV_soc; // 1
    uint8_t CBV_dcdcStatus; // 1
    uint8_t CBV_gear; // 1
    uint16_t CBV_insulationResistance; // 2
    uint16_t CBV_reverse; // 2    

    // DriveMotor
    uint8_t DM_typeCode;
    uint8_t DM_Num; // 1;  // our car only have one
    uint8_t DM_sequenceCode;
    uint8_t DM_status; // 1
    uint8_t DM_controllerTemperature; // 1
    uint16_t DM_rotationlSpeed; // 2
    uint16_t DM_torque; // 2
    uint8_t DM_temperature; // 1
    uint16_t DM_controllerInputVoltage; // 2
    uint16_t DM_controllerDCBusCurrent; // 2

    // Location
    uint8_t L_typeCode;
    uint8_t L_status;
    uint32_t L_longitudeAbs;
    uint32_t L_latitudeAbs;

    // Extreme Value
    uint8_t EV_typeCode;
    uint8_t EV_maxVoltageSysCode;
    uint8_t EV_maxVoltageMonomerCode;
    uint16_t EV_maxVoltage;
    uint8_t EV_minVoltageSysCode;
    uint8_t EV_minVoltageMonomerCode;
    uint16_t EV_minVoltage;
    uint8_t EV_maxTemperatureSysCode;
    uint8_t EV_maxTemperatureProbeCode;
    uint8_t EV_maxTemperature;
    uint8_t EV_minTemperatureSysCode;
    uint8_t EV_minTemperatureProbeCode;
    uint8_t EV_minTemperature;

    // Alarm
    uint8_t A_typeCode;
    uint8_t A_highestAlarmLevel;
    uint32_t A_generalAlarmSigns;

    // checkCode 应放在复制时计算
    // uint8_t checkCode;

    CarSignalData() {
        CBV_chargeStatus = 0xfe; // 后面需要根据充电状态做处理，所以默认值设为无效。
        // To do 所有字段默认值设为无效 0xfe

        // 以下是根据实际情况的固定值    
        CBV_typeCode = 1;
        CBV_runningMode = 1;
        CBV_reverse = 0;
        DM_typeCode = 2;
        DM_Num = 1;
        DM_sequenceCode = 1;
        L_typeCode = 5;
        L_status = 0; // 0__0 0000 第0位（0有效，1无效）、第1位北南纬、第2位东西经、第3~7位（保留）设为0 
        EV_typeCode = 6;
        EV_maxVoltageSysCode = 1;
        EV_minVoltageSysCode = 1;
        EV_maxTemperatureSysCode = 1;
        EV_minTemperatureSysCode = 1;
        A_typeCode = 7;
    }
} CarSigData_t;

#pragma pack()

typedef struct {
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;

    std::string DBHostName;
    std::string DBUserName;
    std::string DBPassword;

    std::string MQServerUrl;
    std::string MQTopic;
    std::string MQClientID;
    std::string MQServerUserName;
    std::string MQServerPassword;
    // 登入时若等待回应超时（loginTimeout），重新发送登入数据。连续三次没有回应，等待loginTimeout2时间，继续重复登入流程。
    int LoginTimeout;
    int LoginTimeout2;
    // 实时数据校验失败重发每个1min
    int RealtimeDataResendTime;
    time_t Period;

    enumEncryptionAlgorithm EncryptionAlgorithm;

    sql::Driver* DBDriver;

    CarDataMap carDataMap;
    blockqueue::BlockQueue<DataPtrLen*>* dataQueue;

} StaticResource;

typedef struct TypeCarBaseInfo {
    uint64_t carId;
    std::string vin;
} CarBaseInfo;

#endif /* DATAFORMAT_H */
