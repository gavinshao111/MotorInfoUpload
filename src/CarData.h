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
    DataPtrLen* createDataCopy(const bool& isReissue = false);
    // data format expected like | lenghtOfSignalValue(1) | signalTypeCode(4) | signalValue(lenghtOfSignalValue) | ... |
    void updateStructByMQ(int8_t* ptr, size_t length);
    const time_t& getCollectTime() const;
    const std::string& getVin() const;
    const bool& noneGetData() const {return m_noneGetData;}
    const bool& allGetData() const {return m_allGetData;}
    void updateCollectTime(const time_t& collectTime);
private:
    std::string m_vin;
    time_t m_collectTime;
    CarSigData_t m_carSigData;

    bool m_noneGetData; // 初始值 true，如果任何一次查询中得到数据，则置为 false
    bool m_allGetData; // 初始值 true，如果任何一次查询中没得到数据，则置为 false

    class DataGenerator* m_dataGenerator;
    void getSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[], const size_t& size);
    //void getBigSigFromDBAndUpdateStruct(const uint32_t signalCodeArray[]);
    void getLocationFromDBAndUpdateStruct(const uint32_t signalCodeArray[]);
//    void initFixedData(void);
    void updateBySigTypeCode(const uint32_t& signalTypeCode, int8_t* sigValAddr);
    void updateBySigTypeCode(const uint32_t& signalTypeCode, const std::string& sigVal);
};


#endif /* CARDATA_H */

