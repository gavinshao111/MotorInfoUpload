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
    
    const static std::string CmdPlatformLoginStr;
    const static std::string CmdPlatformLogoutStr;
    const static std::string CmdVehicleLoginStr;
    const static std::string CmdVehicleLogoutStr;
    const static std::string CmdRealtimeUploadStr;
    const static std::string CmdReissueUploadStr;
    const static std::string VinInital;
    const static size_t MaxUploadChannelNum;
private:

};

#endif /* CONSTANT_H */

