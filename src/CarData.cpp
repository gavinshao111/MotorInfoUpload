/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CarData.cpp
 * Author: 10256
 * 
 * Created on 2017年2月10日, 上午10:41
 */

#include "CarData.h"
#include "SignalTypeCode.h"
#include "prepared_statement.h"
#include "DataGenerator.h"
#include <exception.h>
#include <warning.h>
#include <string.h>

using namespace std;
using namespace bytebuf;
using namespace sql;
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

// 协议规定的数据包大小为固定值 97
//const static uint8_t sizeOfData = 97;

// complete
const static uint32_t sigArray_CBV[] = {
    OPCODE_VCU_POWERON2REQ,
    OPCODE_BMS_BATT_CHARGE_STS,
    OPCODE_ESC_VEHICLE_SPEED,
    //OPCODE_ICU_TOTAL_METER, // 累计里程4字节，需从其他表获取
    OPCODE_BMS_BATT_TOTAL_VOLT,
    OPCODE_BMS_BATT_TOTAL_CURR,
    OPCODE_BCM_LBMS_SOC,
    OPCODE_DCDC_WORKING_STS,
    OPCODE_VCU_GEAR_LEVEL_POS_STS,
    OPCODE_BMS_ISO_RESSISTANCE_LEVEL
};
const static uint32_t sigArray_DM[] = {
    // 缺 驱动电机状态
    OPCODE_MCU_TEMP,
    OPCODE_MCU_MOTOR_RPM,
    OPCODE_MCU_ACTUALTORQUEFB,
    OPCODE_MCU_MOTOR_TEMP,
    OPCODE_MCU_MAINWIREVOLT,
    OPCODE_MCU_MAINWIRECURR,
};
// 未定
const static uint32_t sigArray_Location[] = {};
// 缺太多
const static uint32_t sigArray_EV[] = {};
// 缺太多
const static uint32_t sigArray_Alarm[] = {};
// 长度超过2字节的信号单独处理
const static uint32_t sigArray_Big[] = {
    OPCODE_ICU_TOTAL_METER //累计里程4字节
};

// 程序运行过程中新车机发送完整数据时调用此构造函数
CarData::CarData(const string& vin, DataGenerator* motorInfoUpload) : m_vin(vin), m_dataGenerator(motorInfoUpload) {
    if (VINLEN != vin.length())
        throw runtime_error("CarData(): illegal vin");
//    m_noneGetData = true;
//    m_allGetData = true;

    initFixedData();
}

// 程序初始化从数据库读取数据时调用此构造函数
CarData::CarData(const CarBaseInfo& carInfo, DataGenerator* motorInfoUpload) : m_vin(carInfo.vin), m_dataGenerator(motorInfoUpload) {
    if (VINLEN != carInfo.vin.length())
        throw runtime_error("CarData(): illegal vin");
    if (carInfo.carId < 1)
        throw runtime_error("CarData(): Illegal carInfo.carId");
    m_noneGetData = true;
    m_allGetData = true;

    m_dataGenerator->m_prepStmtForGps->setUInt64(1, carInfo.carId);
    m_dataGenerator->m_prepStmtForSig->setUInt64(1, carInfo.carId);
    m_dataGenerator->m_prepStmtForBigSig->setUInt64(1, carInfo.carId);
    
    initFixedData();
}

CarData::CarData(const CarData& orig) {
}

CarData::~CarData() {
}

time_t CarData::getCollectTime() {
    return m_collectTime;
}

string CarData::getVin() {
    return m_vin;
}

void CarData::initFixedData() {
    m_data.startCode[0] = '#';
    m_data.startCode[1] = '#';
    m_data.responseFlag = 0xfe;
    m_vin.copy((char*) m_data.vin, sizeof (m_data.vin));
    m_data.encryptionAlgorithm = m_dataGenerator->m_staticResource->EncryptionAlgorithm;
    m_data.dataUnitLength = sizeof (m_data) - offsetof(CarSignalData, year);

    m_data.CBV_typeCode = 1;
    m_data.CBV_runningMode = 1;
    m_data.CBV_reverse = 0;
    m_data.DM_typeCode = 2;
    m_data.DM_Num = 1;
    m_data.DM_sequenceCode = 1;
    m_data.L_typeCode = 5;
    m_data.EV_typeCode = 6;
    m_data.EV_maxVoltageSysCode = 1;
    m_data.EV_minVoltageSysCode = 1;
    m_data.EV_maxTemperatureSysCode = 1;
    m_data.EV_minTemperatureSysCode = 1;
    m_data.A_typeCode = 7;
}

/*
 * not done
 * signal value may be nagetive
 */
void CarData::updateBySigTypeCode(const uint32_t& signalTypeCode, uint8_t* sigValAddr) {

    switch (signalTypeCode) {
        case 106:
            m_data.CBV_vehicleStatus = *sigValAddr;
            break;
            /*
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 409:
                        m_data.CBV_speed = *(uint16_t*)sigValAddr;
                        break;
                    case 108:
                        m_data.CBV_mileages = *(uint32_t*)sigValAddr;
                        break;
                    case 201:
                        m_data.CBV_totalVoltage = *(uint16_t*)sigValAddr;
                        break;
                    case 202:
                        m_data.CBV_totalCurrent = *(uint16_t*)sigValAddr;
                        break;
                    case 113:
                        m_data.CBV_soc = *sigValAddr;
                        break;
                    case 101:
                        m_data.CBV_dcdcStatus = *sigValAddr;
                        break;
                    case 105:
                        m_data.CBV_gear = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_insulationResistance = *(uint16_t*)sigValAddr;
                        break;
                    case 0:
                        m_data.DM_status = *sigValAddr;
                        break;
                    case 202:
                        m_data.DM_controllerTemperature = *sigValAddr;
                        break;
                    case 101:
                        m_data.DM_rotationlSpeed = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
                    case 106:
                        m_data.CBV_vehicleStatus = *sigValAddr;
                        break;
                    case 102:
                        m_data.CBV_chargeStatus = *sigValAddr;
                        break;
             */
        default:
            throw runtime_error("CarData::updateBySigTypeCode(): Illegal signalTypeCode");
    }
}

void CarData::createByDataFromDB(const bool& isReissue, const time_t& collectTime) {
    // 2字节以内数据从 car_signal 表取，2字节以上数据从其他表取

    getSigFromDBAndUpdateStruct(sigArray_CBV);
    // 国标：停车充电过程中无需传输驱动电机数据
    if (PARKANDCHARGE != m_data.CBV_chargeStatus)
        getSigFromDBAndUpdateStruct(sigArray_DM);
    getLocationFromDBAndUpdateStruct();
    getSigFromDBAndUpdateStruct(sigArray_EV);
    getSigFromDBAndUpdateStruct(sigArray_Alarm);

    getBigSigFromDBAndUpdateStruct(sigArray_Big);

    updateCollectTime(collectTime);

    m_data.CmdId = isReissue ? enumCmdCode::reissueUpload : enumCmdCode::realtimeUpload;
}

DataPtrLen* CarData::createDataCopy() {

    DataPtrLen* dataCpy = new DataPtrLen();

    // 如果充电状态为停车充电，则只复制除驱动电机外的数据
    if (PARKANDCHARGE == m_data.CBV_chargeStatus) {
        //需除去驱动电机数据
        size_t DMLength = offsetof(CarSignalData, L_typeCode) - offsetof(CarSignalData, DM_typeCode);
        dataCpy->m_length = sizeof (m_data) - DMLength + 1;
        dataCpy->m_ptr = new uint8_t[dataCpy->m_length];
        memset(dataCpy->m_ptr, 0, dataCpy->m_length);

        m_data.dataUnitLength -= DMLength;

        size_t ofset_DM = offsetof(CarSignalData, DM_typeCode);
        size_t ofset_AfterDM = offsetof(CarSignalData, L_typeCode);
        memcpy(dataCpy->m_ptr, (uint8_t*) & m_data, ofset_DM);
        memcpy(dataCpy->m_ptr + ofset_DM, (uint8_t*) (&m_data + ofset_AfterDM), sizeof (m_data) - ofset_AfterDM);
    } else {
        dataCpy->m_length = sizeof (m_data) + 1;
        dataCpy->m_ptr = new uint8_t[dataCpy->m_length];
        memset(dataCpy->m_ptr, 0, dataCpy->m_length);
        memcpy(dataCpy->m_ptr, (uint8_t*) & m_data, sizeof (m_data));
    }

    // put check code. 国标：采用BCC（异或校验）法，校验范围从命令单元的第一个字节开始，同后一字节异或，直到校验码前一字节为止，校验码占用一个字节
    uint8_t checkCode = *(dataCpy->m_ptr);
    size_t i = 1;
    for (; i < dataCpy->m_length - 1; i++)
        checkCode ^= *(dataCpy->m_ptr + i);

    *(dataCpy->m_ptr + i) = checkCode;
    return dataCpy;
}

void CarData::updateCollectTime(const time_t& collectTime) {
    if (0 == collectTime)
        throw runtime_error("updateCollectTime(): IllegalArgument");
    m_collectTime = collectTime;
    struct tm* currUploadTimeTM = localtime(&m_collectTime);
    m_data.year = currUploadTimeTM->tm_year - 100;
    m_data.mon = currUploadTimeTM->tm_mon;
    m_data.mday = currUploadTimeTM->tm_mday;
    m_data.hour = currUploadTimeTM->tm_hour;
    m_data.min = currUploadTimeTM->tm_min;
    m_data.sec = currUploadTimeTM->tm_sec;
}

/*
 * 从 car_signal 表读取信号值，信号值长度为2字节
 * 若没有数据返回false
 */
void CarData::getSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[]) {
    if (0 == sizeof (signalCodeArray))
        return;

    ResultSet* result = NULL;

    try {
        uint16_t sigVal;
        
        for (int i = 0; i < sizeof (signalCodeArray); i++) {
            m_dataGenerator->m_prepStmtForSig->setUInt(2, signalCodeArray[i]);
            result = m_dataGenerator->m_prepStmtForSig->executeQuery();
            if (result->next()) {
                sigVal = result->getInt("signal_value");
                updateBySigTypeCode(signalCodeArray[i], (uint8_t*) & sigVal);
                m_noneGetData = false;
            } else
                m_allGetData = false;

            if (NULL != result) {
                delete result;
                result = NULL;
            }
        }

    } catch (SQLException &e) {
        if (NULL != result) {
            delete result;
            result = NULL;
        }
        throw e;
    }
}

/*
 * 从 car_signal1 表读取信号值，信号值长度为4字节
 */
void CarData::getBigSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[]) {
    if (0 == sizeof (signalCodeArray))
        return;

    ResultSet* result = NULL;

    try {
        uint32_t sigVal;
        
        for (int i = 0; i < sizeof (signalCodeArray); i++) {
            m_dataGenerator->m_prepStmtForBigSig->setUInt(2, signalCodeArray[i]);
            result = m_dataGenerator->m_prepStmtForBigSig->executeQuery();
            if (result->next()) {
                sigVal = result->getUInt("signal_value");
                updateBySigTypeCode(signalCodeArray[i], (uint8_t*) & sigVal);
                m_noneGetData = false;
            }
            else
                m_allGetData = false;

            if (NULL != result) {
                delete result;
                result = NULL;
            }
        }
    } catch (SQLException &e) {
        if (NULL != result) {
            delete result;
            result = NULL;
        }
        throw e;
    }
}

/*
 * not done
 * 车机MQ最多30s上传一次
 * 从 car_trace_gps_info 表获取定位数据
 */
void CarData::getLocationFromDBAndUpdateStruct() {
    ResultSet* result = NULL;

    // 等其他信号码确定好，再确定这两个
    uint32_t latitudeSigTypeCode = 12123;
    uint32_t longitudeSigTypeCode = 12124;

    try {
        int latitude, longitude;
        
        result = m_dataGenerator->m_prepStmtForGps->executeQuery();
        if (result->next()) {
            latitude = (int) result->getDouble("latitude") * 1000000;
            longitude = (int) result->getDouble("longitude") * 1000000;
            updateBySigTypeCode(latitudeSigTypeCode, (uint8_t*) & latitude);
            updateBySigTypeCode(longitudeSigTypeCode, (uint8_t*) & longitude);
            m_noneGetData = false;
        } else
            m_allGetData = false;

        if (NULL != result) {
            delete result;
            result = NULL;
        }
    } catch (SQLException &e) {
        if (NULL != result) {
            delete result;
            result = NULL;
        }
        throw e;
    }
}

// data format expected like | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | time(8) | ... |

void CarData::updateStructByMQ(uint8_t* ptr, size_t length) {
    if (NULL == ptr || 1 > length)
        throw runtime_error("updateStructByMQ(): IllegalArgument");

    uint8_t sigValLen;
    uint32_t sigTypeCode;
    time_t collectTime;
    for (size_t i = 0; i < length;) {
        if (i + 1 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        sigValLen = *(ptr + i);
        i + 1;

        if (i + 4 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        sigTypeCode = *(uint32_t*) (ptr + i);
        i += 4;

        if (i + sigValLen > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        updateBySigTypeCode(sigTypeCode, ptr + i);
        i += sigValLen;

        if (i + 8 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        collectTime = *(time_t*) (ptr + i);
        updateCollectTime(collectTime);
        i += 8;
    }
}
