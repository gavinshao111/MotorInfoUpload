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
#include "Util.h"
#include "DataPtrLen.h"
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
    OPCODE_ICU_TOTAL_METER, // 1070108 累计里程4字节
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

    //    initFixedData();
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

    //    initFixedData();
}

CarData::CarData(const CarData& orig) {

}

CarData::~CarData() {
}

const time_t& CarData::getCollectTime() const {
    return m_collectTime;
}

const string& CarData::getVin() const {
    return m_vin;
}

//void CarData::initFixedData() {
//
////    m_vin.copy((char*) vin, sizeof (vin));
//    
//}

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
        case OPCODE_MCU_MAINWIREVOLT: //电机控制器输入电压
        case OPCODE_MCU_MAINWIRECURR: //电机控制器直流母线电流
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
                sigValInt = (int32_t) sigValLong;
                updateBySigTypeCode(signalTypeCode, (int8_t*) & sigValInt);
            } else
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
 * 注意：
 * 1. 温度值 取值范围 -40~210 可能2字节，可能1字节，按2字节读取，最终转换结果为1字节
 *    车机端：所有温度信号值如果为负，需要传2字节
 */
void CarData::updateBySigTypeCode(const uint32_t& signalTypeCode, int8_t* sigValAddr) {
    void* ptr = NULL;
    size_t size = 0;
    int16_t t;
    switch (signalTypeCode) {
        case OPCODE_VCU_POWERON2REQ:
            m_carSigData.CBV_vehicleStatus = *sigValAddr;
            break;
        case OPCODE_BMS_BATT_CHARGE_STS:
            m_carSigData.CBV_chargeStatus = *sigValAddr;
            break;
        case OPCODE_ESC_VEHICLE_SPEED:
            m_carSigData.CBV_speed = *(uint16_t*) sigValAddr * 10;
            ptr = &m_carSigData.CBV_speed;
            size = 2;
            break;
        case OPCODE_ICU_TOTAL_METER:
            m_carSigData.CBV_mileages = *(uint32_t*) sigValAddr * 10;
            ptr = &m_carSigData.CBV_mileages;
            size = 4;
            break;
        case OPCODE_BMS_BATT_TOTAL_VOLT:
            m_carSigData.CBV_totalVoltage = *(float*) sigValAddr * 10;
            ptr = &m_carSigData.CBV_totalVoltage;
            size = 2;
            break;
        case OPCODE_BMS_BATT_TOTAL_CURR:
            m_carSigData.CBV_totalCurrent = *(float*) sigValAddr * 10 + 10000;
            ptr = &m_carSigData.CBV_totalCurrent;
            size = 2;
            break;
        case OPCODE_BCM_LBMS_SOC:
            m_carSigData.CBV_soc = *sigValAddr;
            break;
        case OPCODE_DCDC_WORKING_STS:
            m_carSigData.CBV_dcdcStatus = (0 == *sigValAddr) ? 2 : 1;
            break;
        case OPCODE_VCU_GEAR_LEVEL_POS_STS:
        {
            switch (*sigValAddr) { //总是有驱动力和制动力，国标前四位固定0011
                case 1:
                    m_carSigData.CBV_gear = 0x3f;
                    break;
                case 2:
                    m_carSigData.CBV_gear = 0x3d;
                    break;
                case 3:
                    m_carSigData.CBV_gear = 0x30;
                    break;
                case 4:
                    m_carSigData.CBV_gear = 0x3e;
                    break;
                default:
                    char errMsg[256] = "CarData::updateBySigTypeCode(): Illegal gear: ";
                    snprintf(errMsg + strlen(errMsg), 1, "%d", *sigValAddr);
                    throw runtime_error(errMsg);
            }
            break;
        }
        case OPCODE_BMS_ISO_RESSISTANCE_LEVEL:
            m_carSigData.CBV_insulationResistance = *(uint16_t*) sigValAddr;
            ptr = &m_carSigData.CBV_insulationResistance;
            size = 2;
            break;
        case DM_Status:
            m_carSigData.DM_status = *sigValAddr;
            break;
        case OPCODE_MCU_TEMP:
            t = *(int16_t*) sigValAddr + 40;
            if (t > 255)
                cout << "[WARN] read DM_controllerTemperature = " << t << " which data size only 1 byte." << endl;
            m_carSigData.DM_controllerTemperature = (uint8_t) t;
            break;
        case OPCODE_MCU_MOTOR_RPM:
            m_carSigData.DM_rotationlSpeed = *(int16_t*) sigValAddr + 20000;
            ptr = &m_carSigData.DM_rotationlSpeed;
            size = 2;
            break;
        case OPCODE_MCU_ACTUALTORQUEFB:
            m_carSigData.DM_torque = *(float*) sigValAddr * 10 + 20000;
            ptr = &m_carSigData.DM_torque;
            size = 2;
            break;
        case OPCODE_MCU_MOTOR_TEMP:
            t = *(int16_t*) sigValAddr + 40;
            if (t > 255)
                cout << "[WARN] read DM_temperature = " << t << " which data size only 1 byte." << endl;
            m_carSigData.DM_temperature = *(int16_t*) sigValAddr + 40;
            break;
        case OPCODE_MCU_MAINWIREVOLT:
            m_carSigData.DM_controllerInputVoltage = *(float*) sigValAddr * 10;
            ptr = &m_carSigData.DM_controllerInputVoltage;
            size = 2;
            break;
        case OPCODE_MCU_MAINWIRECURR:
            m_carSigData.DM_controllerDCBusCurrent = *(float*) sigValAddr * 10 + 10000;
            ptr = &m_carSigData.DM_controllerDCBusCurrent;
            size = 2;
            break;
        case L_Longitude:
        { //经度 假设车机经纬度数据是一定一起发送的。
            int32_t longitude = *(int32_t*) sigValAddr;
            if (longitude < 0) {
                m_carSigData.L_status |= 0x20; // 第2位（东西经）设为1，西经
                m_carSigData.L_longitudeAbs = -longitude;
            } else {
                m_carSigData.L_status &= 0xdf; // 第2位（东西经）设为0，东经
                m_carSigData.L_longitudeAbs = longitude;
            }
            ptr = &m_carSigData.L_longitudeAbs;
            size = 4;
            break;
        }
        case L_Latitude:
        {
            int32_t latitude = *(int32_t*) sigValAddr;
            if (latitude < 0) {
                m_carSigData.L_status |= 0x40; // 0100 0000 第1位（北南纬）设为1，南纬
                m_carSigData.L_latitudeAbs = -latitude;
            } else {
                m_carSigData.L_status &= 0xbf; // 1011 1111 第1位（北南纬）设为0，北纬
                m_carSigData.L_latitudeAbs = latitude;
            }
            ptr = &m_carSigData.L_latitudeAbs;
            size = 4;
            break;
        }
        case EV_MaxVoltageMonomerCode:
            m_carSigData.EV_maxVoltageMonomerCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_CELLMAXVOLT:
            m_carSigData.EV_maxVoltage = *(uint16_t*) sigValAddr;
            ptr = &m_carSigData.EV_maxVoltage;
            size = 2;
            break;
        case EV_MinVoltageMonomerCode:
            m_carSigData.EV_minVoltageMonomerCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_CELLMINVOLT:
            m_carSigData.EV_minVoltage = *(uint16_t*) sigValAddr;
            ptr = &m_carSigData.EV_minVoltage;
            size = 2;
            break;
        case EV_MaxTemperatureProbeCode:
            m_carSigData.EV_maxTemperatureProbeCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_BATT_MAX_TEMP:
            m_carSigData.EV_maxTemperature = *(int16_t*) sigValAddr + 40;
            break;
        case EV_MinTemperatureProbeCode:
            m_carSigData.EV_minTemperatureProbeCode = *sigValAddr + 1;
            break;
        case OPCODE_BMS_BATT_MIN_TEMP:
            t = *(int16_t*) sigValAddr + 40;
            if (t > 255)
                cout << "[WARN] read EV_minTemperature = " << t << " which data size only 1 byte." << endl;
            m_carSigData.EV_minTemperature = (uint8_t) t;
            break;
        case OPCODE_VCU_VEHICLE_WARNING:
            switch (*sigValAddr) {
                case 0: m_carSigData.A_highestAlarmLevel = 0;
                    break;
                case 1: m_carSigData.A_highestAlarmLevel = 3;
                    break;
                case 2: m_carSigData.A_highestAlarmLevel = 2;
                    break;
                case 3: m_carSigData.A_highestAlarmLevel = 1;
                    break;
                default:
                    char errMsg[256] = "CarData::updateBySigTypeCode(): Illegal generalAlarmSigns: ";
                    snprintf(errMsg + strlen(errMsg), 1, "%d", *sigValAddr);
                    throw runtime_error(errMsg);
            }
        case A_GeneralAlarmSigns:
            m_carSigData.A_generalAlarmSigns = *(int32_t*) sigValAddr;
            ptr = &m_carSigData.A_generalAlarmSigns;
            size = 4;
            break;
        default:
            char errMsg[256] = "CarData::updateBySigTypeCode(): Illegal signalTypeCode: ";
            snprintf(errMsg + strlen(errMsg), 4, "%d", signalTypeCode);
            throw runtime_error(errMsg);
    }
    if (NULL != ptr && size > 1)
        Util::BigLittleEndianTransfer(ptr, size);
}

void CarData::createByDataFromDB(const bool& isReissue, const time_t& collectTime) {
    // 2字节以内数据从 car_signal 表取，2字节以上数据从其他表取

    getSigFromDBAndUpdateStruct(sigArray_CBV, sizeof (sigArray_CBV) / sizeof (uint32_t));
    // 国标：停车充电过程中无需传输驱动电机数据
    // 若不是补发数据，则是创建基础数据，需要获取DM数据
    if (2 == m_carSigData.CBV_chargeStatus
            || 3 == m_carSigData.CBV_chargeStatus
            || 4 == m_carSigData.CBV_chargeStatus
            || !isReissue) {
        getSigFromDBAndUpdateStruct(sigArray_DM, sizeof (sigArray_DM) / sizeof (uint32_t));
    }
    getLocationFromDBAndUpdateStruct(sigArray_Location);
    getSigFromDBAndUpdateStruct(sigArray_EV, sizeof (sigArray_EV) / sizeof (uint32_t));
    getSigFromDBAndUpdateStruct(sigArray_Alarm, sizeof (sigArray_Alarm) / sizeof (uint32_t));
    //getBigSigFromDBAndUpdateStruct(sigArray_Big);

    updateCollectTime(collectTime);

}

DataPtrLen* CarData::createDataCopy(const bool& isReissue/* = false*/) {

    DataPtrLen* dataCpy;

    // 如果充电状态为停车充电，则只复制除驱动电机外的数据
    if (PARKANDCHARGE == m_carSigData.CBV_chargeStatus) {
        //需除去驱动电机数据
        size_t ofset_DM = offsetof(CarSignalData, DM_typeCode);
        size_t ofset_AfterDM = offsetof(CarSignalData, L_typeCode);
        size_t DMLength = ofset_AfterDM - ofset_DM;

        dataCpy = new DataPtrLen(m_vin, sizeof (m_carSigData) - DMLength, isReissue);
        dataCpy->m_dataBuf->put((uint8_t*) & m_carSigData, 0, ofset_DM);
        dataCpy->m_dataBuf->put((uint8_t*) & m_carSigData, ofset_AfterDM, sizeof (m_carSigData) - ofset_AfterDM);
    } else {
        dataCpy = new DataPtrLen(m_vin, sizeof (m_carSigData), isReissue);
        dataCpy->m_dataBuf->put((uint8_t*) & m_carSigData, 0, sizeof (m_carSigData));
    }

    //    dataCpy->genCheckCode();
    dataCpy->m_dataBuf->flip();
    return dataCpy;
}

/*
 * 通过MQ更新，采集时间就是车机发上来的采集时间；通过不发数据，采集时间就是那次的周期点时间。
 */
void CarData::updateCollectTime(const time_t& collectTime) {
    if (0 == collectTime)
        throw runtime_error("updateCollectTime(): IllegalArgument");
    m_collectTime = collectTime;
    struct tm* currUploadTimeTM = localtime(&m_collectTime);
    m_carSigData.year = currUploadTimeTM->tm_year - 100;
    m_carSigData.mon = currUploadTimeTM->tm_mon + 1; // [0-11]
    m_carSigData.mday = currUploadTimeTM->tm_mday;
    m_carSigData.hour = currUploadTimeTM->tm_hour;
    m_carSigData.min = currUploadTimeTM->tm_min;
    m_carSigData.sec = currUploadTimeTM->tm_sec;
    //#if DEBUG
    //    cout << "CarData: collectTime is updated to " << Util::timeToStr(m_collectTime) << endl;
    //#endif
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

/* 
 * data format expected like 
 * | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | time(8) |
 * | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | time(8) |
 * | ...
 */

void CarData::updateStructByMQ(int8_t* ptr, size_t length) {
    if (NULL == ptr || 1 > length)
        throw runtime_error("updateStructByMQ(): IllegalArgument");

    int8_t sigValLen;
    uint32_t sigTypeCode;
    time_t collectTime;
    uint8_t signalVal[4]; // 接受数据最大为4字节，不足的0填充
    for (size_t i = 0; i < length;) {
        sigValLen = 0;
        sigTypeCode = 0;
        collectTime = 0;
        memset(signalVal, 0, sizeof (signalVal));

        if (i + 1 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        sigValLen = *(ptr + i);

        if (0 == sigValLen)
            break;
        else if (sigValLen > 4 || sigValLen < 0) {
            char errMsg[256] = "CarData::updateStructByMQ(): Illegal sigValLen: ";
            snprintf(errMsg + strlen(errMsg), 4, "%d", sigValLen);
            throw runtime_error(errMsg);
        }
        i++;

        if (i + 4 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");

        sigTypeCode = *(uint32_t*) (ptr + i);
        i += 4;
        Util::BigLittleEndianTransfer((uint8_t*) & sigTypeCode, 4);

        if (i + sigValLen > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");

        //        Util::printBinary(*(uint8_t*) (ptr + i));
        memcpy(signalVal, ptr + i, sigValLen);
        Util::BigLittleEndianTransfer(signalVal, sigValLen);
        i += sigValLen;

        updateBySigTypeCode(sigTypeCode, (int8_t*) signalVal);

        if (i + 8 > length)
            throw runtime_error("updateStructByMQ(): MQ payload too small");
        collectTime = *(time_t*) (ptr + i);
        Util::BigLittleEndianTransfer((uint8_t*) & collectTime, 8);
        updateCollectTime(collectTime);
        i += 8;
    }
}

