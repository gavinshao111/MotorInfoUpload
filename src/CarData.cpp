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
#include <stdlib.h>

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
    OPCODE_VCU_POWERON2REQ, // 1010211
    OPCODE_BMS_BATT_CHARGE_STS, // 1030102
    OPCODE_ESC_VEHICLE_SPEED, // 1100409
    OPCODE_ICU_TOTAL_METER,       // 1070108 累计里程4字节
    OPCODE_BMS_BATT_TOTAL_VOLT, // 1030201 电压单位V，小数
    OPCODE_BMS_BATT_TOTAL_CURR, // 1030202 电流单位A，小数
    OPCODE_BCM_LBMS_SOC, // 1060115
    OPCODE_DCDC_WORKING_STS, // 1040101
    OPCODE_VCU_GEAR_LEVEL_POS_STS, // 1010105
    OPCODE_BMS_ISO_RESSISTANCE_LEVEL// 1030116
};
const static uint32_t sigArray_DM[] = {
    DM_Status, // 1 缺 驱动电机状态 test
    OPCODE_MCU_TEMP, // 1020202 may negative,
    OPCODE_MCU_MOTOR_RPM, // 1020101 may negative,
    OPCODE_MCU_ACTUALTORQUEFB, // 1020108 扭矩单位N*m,小数 may negative,
    OPCODE_MCU_MOTOR_TEMP, // 1020201 may negative,
    OPCODE_MCU_MAINWIREVOLT, // 1020203 电机控制器输入电压单位mV may negative,
    OPCODE_MCU_MAINWIRECURR // 1020204 电机控制器直流母线电流单位mA may negative,
};
// 未定
const static uint32_t sigArray_Location[] = {
    L_Longitude, // 经度
    L_Latitude // 纬度 顺序固定
};
// 缺太多
const static uint32_t sigArray_EV[] = {
    EV_MaxVoltageMonomerCode, // 4
    OPCODE_BMS_CELLMAXVOLT, // 1030601
    EV_MinVoltageMonomerCode, // 5
    OPCODE_BMS_CELLMINVOLT, // 1030603
    EV_MaxTemperatureProbeCode, // 6
    OPCODE_BMS_BATT_MAX_TEMP, // 1030401 may negative,
    EV_MinTemperatureProbeCode, // 7
    OPCODE_BMS_BATT_MIN_TEMP // 1030402 may negative,
};
// 缺太多
const static uint32_t sigArray_Alarm[] = {
    OPCODE_VCU_VEHICLE_WARNING, // 1010104
    A_GeneralAlarmSigns // 8 通用报警标志 4字节
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
//    m_dataGenerator->m_prepStmtForBigSig->setUInt64(1, carInfo.carId);

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
    m_data.CBV_chargeStatus = 0xfe; // 后面需要根据充电状态做处理，所以默认值设为无效。
    // To do 所有字段默认值设为无效 0xfe
    
// 以下是根据实际情况的固定值    
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
    m_data.L_status &= 0x60; // 0110 0000 第0位（0有效，1无效）、第3~7位（保留）设为0 
    m_data.EV_typeCode = 6;
    m_data.EV_maxVoltageSysCode = 1;
    m_data.EV_minVoltageSysCode = 1;
    m_data.EV_maxTemperatureSysCode = 1;
    m_data.EV_minTemperatureSysCode = 1;
    m_data.A_typeCode = 7;
}

/*
 * @sigVal
 * is from car_signal.signal_value(varchar 50), maybe float, nagetive, uint8_t, uint16_t, uint32_t, 
 * 测试通过
 */
void CarData::updateBySigTypeCode(const uint32_t& signalTypeCode, const string& sigVal) {
    switch (signalTypeCode) {
        case OPCODE_MCU_ACTUALTORQUEFB: // 1020108 扭矩单位N*m,小数
        case OPCODE_BMS_BATT_TOTAL_VOLT: // 1020203 总电压单位V，小数
        case OPCODE_BMS_BATT_TOTAL_CURR: // 1020204 总电流单位A，小数
            float sigValFlt;
            sigValFlt = atof(sigVal.c_str());
            updateBySigTypeCode(signalTypeCode, (int8_t*) & sigValFlt);
            break;
        default:
            int64_t sigValLong = 0; // 用long型是因为通用报警标志可能为 0xffffffff,int型装不下
            int32_t sigValInt = 0;
            // sigVal maybe -123, 100, ，0xffff, 2 bytes or 1 byte
            sigValLong = atol(sigVal.c_str());
            if (sigValLong < 0) {
                sigValInt = (int32_t)sigValLong;
                updateBySigTypeCode(signalTypeCode, (int8_t*) & sigValInt);
            }
            else
                updateBySigTypeCode(signalTypeCode, (int8_t*) & sigValLong);
            break;
    }
}

/*
 * not done
 * 缺的信号先自定义，到时候只要改缺的信号就行，数据转换已完成
 * 字符数字测试通过
 * 数据转换规则参考 Docs/零云平台-整车数据信号整理.xlsx
 * signal value may be nagetive or deciaml
 */
void CarData::updateBySigTypeCode(const uint32_t& signalTypeCode, int8_t* sigValAddr) {
    switch (signalTypeCode) {
        case OPCODE_VCU_POWERON2REQ:
            m_data.CBV_vehicleStatus = *sigValAddr;
            break;
        case OPCODE_BMS_BATT_CHARGE_STS:
            m_data.CBV_chargeStatus = *sigValAddr;
            break;
        case OPCODE_ESC_VEHICLE_SPEED:
            m_data.CBV_speed = *(uint16_t*) sigValAddr * 10;
            break;
        case OPCODE_ICU_TOTAL_METER:
            m_data.CBV_mileages = *(uint32_t*) sigValAddr * 10;
            break;
        case OPCODE_BMS_BATT_TOTAL_VOLT:
            m_data.CBV_totalVoltage = *(float*) sigValAddr * 10;
            break;
        case OPCODE_BMS_BATT_TOTAL_CURR:
            m_data.CBV_totalCurrent = *(float*) sigValAddr * 10;
            break;
        case OPCODE_BCM_LBMS_SOC:
            m_data.CBV_soc = *sigValAddr;
            break;
        case OPCODE_DCDC_WORKING_STS:
            m_data.CBV_dcdcStatus = (0 == *sigValAddr) ? 2 : 1;
            break;
        case OPCODE_VCU_GEAR_LEVEL_POS_STS:
        {
            uint8_t first4bit = 3;
            uint8_t last4bit;
            switch (*sigValAddr) {
                case 1:last4bit = 0xf;
                    break;
                case 2:last4bit = 0xd;
                    break;
                case 3:last4bit = 0;
                    break;
                case 4:last4bit = 0xe;
                    break;
                default:
                    throw runtime_error("CarData::updateBySigTypeCode(): Illegal gear: " + *sigValAddr);
            }
            m_data.CBV_gear = (first4bit << 4) + last4bit;
            break;
        }
        case OPCODE_BMS_ISO_RESSISTANCE_LEVEL:
            m_data.CBV_insulationResistance = *(uint16_t*) sigValAddr;
            break;
        case DM_Status:
            m_data.DM_status = *sigValAddr;
            break;
        case OPCODE_MCU_TEMP:   // 温度值 取值范围 -40~210 可能2字节，可能1字节，按2字节读取，最终转换结果为1字节
            m_data.DM_controllerTemperature = *(int16_t*)sigValAddr + 40;
            break;
        case OPCODE_MCU_MOTOR_RPM:
            m_data.DM_rotationlSpeed = *(int16_t*) sigValAddr + 20000;
            break;
        case OPCODE_MCU_ACTUALTORQUEFB:
            m_data.DM_torque = *(float*) sigValAddr * 10 + 20000;
            break;
        case OPCODE_MCU_MOTOR_TEMP:
            m_data.DM_temperature = *(int16_t*) sigValAddr + 40;
            break;
        case OPCODE_MCU_MAINWIREVOLT:
            m_data.DM_controllerInputVoltage = *(uint16_t*) sigValAddr * 10;
            break;
        case OPCODE_MCU_MAINWIRECURR:
            m_data.DM_controllerDCBusCurrent = *(int16_t*) sigValAddr * 10 + 10000;
            break;
        case L_Longitude:
        { //经度 假设车机经纬度数据是一定一起发送的。
            int32_t longitude = *(int32_t*) sigValAddr;
            if (longitude < 0) {
                m_data.L_status |= 0x20; // 第2位（东西经）设为1，西经
                m_data.L_longitudeAbs -= longitude;
            } else {
                m_data.L_status &= 0xdf; // 第2位（东西经）设为0，东经
                m_data.L_longitudeAbs = longitude;
            }
            break;
        }
        case L_Latitude:
        {
            int32_t latitude = *(int32_t*) sigValAddr;
            if (latitude < 0) {
                m_data.L_status |= 0x40; // 0100 0000 第1位（北南纬）设为1，南纬
                m_data.L_latitudeAbs -= latitude;
            } else {
                m_data.L_status &= 0xbf; // 1011 1111 第1位（北南纬）设为0，北纬
                m_data.L_latitudeAbs = latitude;
            }
            break;
        }
        case EV_MaxVoltageMonomerCode:
            m_data.EV_maxVoltageMonomerCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_CELLMAXVOLT:
            m_data.EV_maxVoltage = *(uint16_t*) sigValAddr;
            break;
        case EV_MinVoltageMonomerCode:
            m_data.EV_minVoltageMonomerCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_CELLMINVOLT:
            m_data.EV_minVoltage = *(uint16_t*) sigValAddr;
            break;
        case EV_MaxTemperatureProbeCode:
            m_data.EV_maxTemperatureProbeCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_BATT_MAX_TEMP:
            m_data.EV_maxTemperature = *(int16_t*)sigValAddr + 40;
            break;
        case EV_MinTemperatureProbeCode:
            m_data.EV_minTemperatureProbeCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_BATT_MIN_TEMP:
            m_data.EV_minTemperature = *(int16_t*)sigValAddr + 40;
            break;
        case OPCODE_VCU_VEHICLE_WARNING:
            switch (*sigValAddr) {
                case 0: m_data.A_highestAlarmLevel = 0;
                    break;
                case 1: m_data.A_highestAlarmLevel = 3;
                    break;
                case 2: m_data.A_highestAlarmLevel = 2;
                    break;
                case 3: m_data.A_highestAlarmLevel = 1;
                    break;
                default: throw runtime_error("CarData::updateBySigTypeCode(): Illegal generalAlarmSigns: " + *sigValAddr);
            }
        case A_GeneralAlarmSigns:
            m_data.A_generalAlarmSigns = *(int32_t*) sigValAddr;
            break;
        default:
            throw runtime_error("CarData::updateBySigTypeCode(): Illegal signalTypeCode：" + signalTypeCode);
    }
}

void CarData::createByDataFromDB(const bool& isReissue, const time_t& collectTime) {
    // 2字节以内数据从 car_signal 表取，2字节以上数据从其他表取

    getSigFromDBAndUpdateStruct(sigArray_CBV, sizeof(sigArray_CBV)/sizeof(uint32_t));
    // 国标：停车充电过程中无需传输驱动电机数据
    // 若不是补发数据，则是创建基础数据，需要获取DM数据
    if (2 == m_data.CBV_chargeStatus
            || 3 == m_data.CBV_chargeStatus
            || 4 == m_data.CBV_chargeStatus
            || !isReissue) {
        getSigFromDBAndUpdateStruct(sigArray_DM, sizeof(sigArray_DM)/sizeof(uint32_t));
    }
    getLocationFromDBAndUpdateStruct(sigArray_Location);
    getSigFromDBAndUpdateStruct(sigArray_EV, sizeof(sigArray_EV)/sizeof(uint32_t));
    getSigFromDBAndUpdateStruct(sigArray_Alarm, sizeof(sigArray_Alarm)/sizeof(uint32_t));
    //getBigSigFromDBAndUpdateStruct(sigArray_Big);

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
    m_data.mon = currUploadTimeTM->tm_mon + 1;  // [0-11]
    m_data.mday = currUploadTimeTM->tm_mday;
    m_data.hour = currUploadTimeTM->tm_hour;
    m_data.min = currUploadTimeTM->tm_min;
    m_data.sec = currUploadTimeTM->tm_sec;
}

/*
 * 从 car_signal 表读取信号值，信号值长度为2字节
 * 若没有数据返回false
 */
void CarData::getSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[], const size_t& size) {
    if (0 == sizeof (signalCodeArray))
        return;

    ResultSet* result = NULL;

    try {
        string sigVal;

        for (int i = 0; i < size; i++) {
            m_dataGenerator->m_prepStmtForSig->setUInt(2, signalCodeArray[i]);
            // 挂了
            result = m_dataGenerator->m_prepStmtForSig->executeQuery();
            if (result->next()) {
                //sigVal = result->getInt("signal_value");
                sigVal = result->getString("signal_value");
                updateBySigTypeCode(signalCodeArray[i], sigVal);
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
        throw;
    }
}

/*
 * abandon
 * 从 car_signal1 表读取信号值，信号值长度为4字节

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
                updateBySigTypeCode(signalCodeArray[i], (int8_t*) & sigVal);
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
        throw;
    }
} */

/*
 * not done
 * 车机MQ最多30s上传一次
 * 从 car_trace_gps_info 表获取定位数据
 */
void CarData::getLocationFromDBAndUpdateStruct(const uint32_t signalCodeArray[]) {
    ResultSet* result = NULL;

    try {
        int32_t longitude, latitude;
        result = m_dataGenerator->m_prepStmtForGps->executeQuery();
        if (result->next()) {
            longitude = (int32_t) (result->getDouble("longitude") * 1000000);
            latitude = (int32_t) (result->getDouble("latitude") * 1000000);
            updateBySigTypeCode(signalCodeArray[0], (int8_t*) & longitude);
            updateBySigTypeCode(signalCodeArray[1], (int8_t*) & latitude);
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
        throw;
    }
}

// data format expected like | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | time(8) | ... |

void CarData::updateStructByMQ(int8_t* ptr, size_t length) {
    if (NULL == ptr || 1 > length)
        throw runtime_error("updateStructByMQ(): IllegalArgument");

    int8_t sigValLen = 0;
    uint32_t sigTypeCode = 0;
    time_t collectTime = 0;
    uint32_t signalVal = 0; // 接受数据最大为4字节，不足的0填充
    for (size_t i = 0; i < length;) {
        if (i + 1 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        sigValLen = *(ptr + i);
        if (sigValLen > 4 || sigValLen <= 0)
            throw runtime_error("updateStructByMQ(): Illegal sigValLen: " + sigValLen);
        i + 1;

        if (i + 4 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        sigTypeCode += *(ptr + i++) << 24;
        sigTypeCode += *(ptr + i++) << 16;
        sigTypeCode += *(ptr + i++) << 8;
        sigTypeCode += *(ptr + i++);
//        sigTypeCode = *(uint32_t*) (ptr + i);
//        i += 4;
        if (i + sigValLen > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        
        for (int j = 0; j < sigValLen; j++, i++) {
            signalVal += *(ptr + i) << 8*(sigValLen - j - 1);
        }
        updateBySigTypeCode(sigTypeCode, (int8_t*)&signalVal);
//        i += sigValLen;

        if (i + 8 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        collectTime = *(time_t*) (ptr + i);
        updateCollectTime(collectTime);
        i += 8;
    }
}

bool CarData::isNotParkCharge() {
    return (2 == m_data.CBV_chargeStatus || 3 == m_data.CBV_chargeStatus || 4 == m_data.CBV_chargeStatus);
}