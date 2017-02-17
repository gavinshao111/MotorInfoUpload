/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DriveBatteryElectricalParameter.h
 * Author: 10256
 *
 * Created on 2017年1月9日, 上午11:13
 */

#ifndef DRIVEBATTERYELECTRICALPARAMETER_H
#define DRIVEBATTERYELECTRICALPARAMETER_H

#include "DataFormat.h";
#include <vector>


/**
 * 动力蓄电池电气参数
 */



class DriveBatteryElectricalParameter {
public:
    DriveBatteryElectricalParameter();
    DriveBatteryElectricalParameter(const DriveBatteryElectricalParameter& orig);
    virtual ~DriveBatteryElectricalParameter();
    
    bool fillDataFromDB();
    bool compress(void);
    
private:
    // 电池总成结构体
    typedef struct TypeBatteryAssembly {
        ubyte code; //电池总成号
        ushort driveBatteryVoltage;
        ushort driveBatteryCurrent;
        ushort cellNum; // 单体蓄电池总数
        ushort currFrameBatteryCode; // 本帧起始电池序号
        ubyte currFrameCellNum; //本帧单体电池总数
        //ushort* cellVoltageArray;
        std::vector<ushort> cellVoltageList;
    }BatteryAssembly;
    
    ubyte driveBatteryNum;
    //BatteryAssembly* batteryAssemblyArray;
    std::vector<BatteryAssembly> batteryAssemblyList;
    
    PtrLen* data;
    
    
};

#endif /* DRIVEBATTERYELECTRICALPARAMETER_H */

