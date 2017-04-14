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
//#include "driver.h"
//#include "mysql_connection.h"
//#include "mysql_driver.h"
//#include "statement.h"
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
#define NOTOK 2

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
    vehicleLogin = 1,
    vehicleSignalDataUpload = 2,
    reissueUpload = 3,
    vehicleLogout = 4,
    platformLogin = 5,
    platformLogout = 6,
} enumCmdCode;


#define ALARMDATALEN 6
#define MAXDATAUNITLEN ALARMDATALEN+66

#define PARKANDCHARGE 1

// a complete data packet is DataPacketHeader_t + ******Data_t + uint8_t checkCode

#pragma pack(1)

/*
0 "##" | 2 cmdUnit | 4 vin | 21 加密方式 | 22 DataUnitLength=72 | 24 DataUnit | 96 校验码 | 97
        
DataUnit: 6+1+20+1+13+1+9+1+14+1+5=72
    | 24 时间 | 30 整车数据 | 51 驱动数据 | 65 定位数据 | 75 极值数据 | 91 报警数据 | 97 

整车：
30 1 | 31 车辆状态 | 32 充电状态 | 33 运行模式=1 | 34 车速 | 36 累计里程 | 40 总电压 | 42 总电流 | 44 soc | 45 dcdc | 46 档位 | 47 绝缘电阻 | 49 预留=0 | 51
驱动电机：
51 2 | 52 个数=1 | 53 序号=1 | 54 电机状态 | 55 控制器温度 | 56 转速 | 58 转矩 | 60 温度 | 61 电压 | 63 电流 | 65
车辆位置：
65 5 | 66 定位数据 | 75
极值数据：
75 6 | 76 最高电压系统号 | 77 最高电压单体号 | 78 最高电压值 | 80 最低电压系统号 | 81 最低电压单体号 | 82 最低电压值 | 84 最高温系统号 | 
85 最高温探针号 | 86 最高温度值 | 87 最低温系统号 | 88 最低温探针号 | 89 最低温度值 | 90
报警数据：
90 7 | 91 最高报警等级 | 92 通用报警标志 | 96
 */

/*
 * DataPacketHeader和checkCode在sender中生成，CarSignalData在Generator生成，并存在内存中不断被更新，需要上传时，生成copy，可能需要将DM数据去除
 */

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
    uint8_t encryptionAlgorithm; // header 也有这个字段，重复

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

    //    sql::Driver* DBDriver;

    CarDataMap carDataMap;
    blockqueue::BlockQueue<DataPtrLen*>* dataQueue;

} StaticResource;

typedef struct TypeCarBaseInfo {
    uint64_t carId;
    std::string vin;
} CarBaseInfo;

#endif /* DATAFORMAT_H */
