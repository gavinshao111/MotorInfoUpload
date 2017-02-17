/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CarData.h
 * Author: 10256
 *
 * Created on 2017年2月10日, 上午10:41
 */

#ifndef CARDATA_H
#define CARDATA_H

#include "DataFormat.h"
//#include "prepared_statement.h"

#define ALARMDATALEN 6
#define MAXDATAUNITLEN ALARMDATALEN+66

#define PARKANDCHARGE 1

#pragma pack(1)

typedef struct CarSignalData {
    uint8_t startCode[2];
    uint8_t CmdId;
    uint8_t responseFlag;
    uint8_t vin[VINLEN];
    uint8_t encryptionAlgorithm;
    uint16_t dataUnitLength;

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
    uint8_t L_data[9];

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


} CarSigData_t;

#pragma pack()




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

class DataGenerator;

class CarData {
public:
    
    CarData(const CarBaseInfo& carInfo, DataGenerator* motorInfoUpload);
    CarData(const std::string& vin, DataGenerator* motorInfoUpload);

    CarData(const CarData& orig);

    virtual ~CarData();
    void createByDataFromDB(const bool& isReissue, const time_t& collectTime);
    DataPtrLen* createDataCopy();
    // data format expected like | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | ... |
    void updateStructByMQ(uint8_t* ptr, size_t length);
    
//    static void initForClass(sql::Statement* state, sql::PreparedStatement* prep_stmt, enumEncryptionAlgorithm encryptionAlgorithm);
    time_t getCollectTime();
    std::string getVin();
private:
    std::string m_vin;
    time_t m_collectTime;
    uint64_t m_carId;   // maybe 0
    //CarBaseInfo m_carInfo;
    // bytebuf::ByteBuffer* m_data;
    CarSigData_t m_data;

//    static sql::Statement* m_state;
//    static sql::PreparedStatement* m_prep_stmt;
//    static enumEncryptionAlgorithm m_encryptionAlgorithm;
    
    
    class DataGenerator* m_dataGenerator;

//    // 以信号码为键，装各个信号在数据单元中从整车数据起始的偏移量，得到偏移量，信号值就可以直接填入数据单元中
//    static SignalMap m_signalMap;
//    static SignalMap::iterator m_signalIterator;

    void genRealtimeUploadData();
//    void genCompletelyBuildedVehicleData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    void genDriveMotorData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    //void genEngineData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    void genLocationData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    void genExtremeValueData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    void geAlarmData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
//    void compressSignalDataWithLength(bytebuf::ByteBuffer* DataOut, const uint64_t& carId, const SignalInfo signalInfoArray[], const int& offset, const int& size);
    void getSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[]);
    void getBigSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[]);
    void getLocationFromDBAndUpdateStruct();
    void initFixedData(void);
    void updateBySigTypeCode(const uint32_t& signalTypeCode, uint8_t* sigValAddr);
    void updateCollectTime(const time_t& collectTime);

};


#endif /* CARDATA_H */

