/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CompleteVehicle.h
 * Author: 10256
 *
 * Created on 2017年1月16日, 下午6:41
 */

#ifndef COMPLETELYBUILDEDVEHICLE_H
#define COMPLETELYBUILDEDVEHICLE_H

#include "Info.h"


class CompletelyBuildedVehicle : public Info {
public:
    CompletelyBuildedVehicle();
    CompletelyBuildedVehicle(const CompletelyBuildedVehicle& orig);
    virtual ~CompletelyBuildedVehicle();
    bool fillDataFromDB(void);
private:
    byte   vehicleStatus;    // 1
    byte   chargeStatus;     // 1
    byte   runningMode;      // 1
    ushort  speed;            // 2
    uint    mileages;         // 4
    ushort  totalVoltage;     // 2
    ushort  totalCurrent;     // 2
    byte   soc;              // 1
    byte   dcdcStatus;       // 1
    byte   gear;             // 1
    ushort  insulationResistance; // 2
    ushort  reverse;          // 2
    
};

#endif /* COMPLETELYBUILDEDVEHICLE_H */

