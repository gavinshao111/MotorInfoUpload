/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Constant.h
 * Author: 10256
 *
 * Created on 2017年6月30日, 下午2:47
 */

#ifndef CONSTANT_H
#define CONSTANT_H
#include <string>
class Constant {
public:
    Constant();
    Constant(const Constant& orig);
    virtual ~Constant();
    
    const static std::string cmdPlatformLoginStr;
    const static std::string cmdPlatformLogoutStr;
    const static std::string cmdVehicleLoginStr;
    const static std::string cmdVehicleLogoutStr;
    const static std::string cmdRealtimeUploadStr;
    const static std::string cmdReissueUploadStr;
    const static std::string vinInital;
    const static size_t maxUploadChannelNum;
    const static std::string iniPath;
private:

};

#endif /* CONSTANT_H */

