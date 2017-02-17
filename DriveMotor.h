/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DriveMotor.h
 * Author: 10256
 *
 * Created on 2017年1月16日, 下午7:27
 */

#ifndef DRIVEMOTOR_H
#define DRIVEMOTOR_H

#include "Info.h"


class DriveMotor : Info{
public:
    DriveMotor();
    DriveMotor(const DriveMotor& orig);
    virtual ~DriveMotor();
    bool fillDataFromDB(void);
private:
//    const static ubyte   CdriveMotorNum = 1;  // our car only have one
//    // type code
//    const static ubyte   code           = 71;
//    const static ubyte   status         = 72;         // 1
//    ubyte   controllerTemperature; // 1
//    ushort  rotationlSpeed; // 2
//    ushort  torque;         // 2
//    ubyte   temperature;    // 1
//    ushort  controllerInputVoltage; // 2
//    ushort  controllerDCBusCurrent; // 2
};

#endif /* DRIVEMOTOR_H */

