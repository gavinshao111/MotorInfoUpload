/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Resource.h
 * Author: 10256
 *
 * Created on 2017年5月16日, 下午3:37
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>
#include <map>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "../BlockQueue.h"
//#include "GSocket.h"
#include "DataFormatForward.h"
#include "TcpSession.h"
#include "Logger.h"
#include "PublicServer.h"

class Resource {
public:
    
    static Resource* GetResource();
    virtual ~Resource();

    blockqueue::BlockQueue<BytebufSPtr_t>& GetVehicleDataQueue() {
        return *VehicleDataQueueSPtr;
    }

    const size_t& GetMaxSerialNumber() const {
        return MaxSerialNumber;
    }

    const size_t& GetReSetupPeroid() const {
        return ReSetupPeroid;
    }

    const std::string& GetPaltformId() const {
        return PaltformId;
    }

    const enumEncryptionAlgorithm& GetEncryptionAlgorithm() const {
        return EncryptionAlgorithm;
    }

    const size_t& GetLoginTimes() const {
        return LoginTimes;
    }

    const size_t& GetLoginIntervals2() const {
        return LoginIntervals2;
    }

    const size_t& GetLoginIntervals() const {
        return LoginIntervals;
    }

    const size_t& GetReadResponseTimeOut() const {
        return ReadResponseTimeOut;
    }

    const int& GetEnterprisePlatformTcpServicePort() const {
        return EnterprisePlatformTcpServicePort;
    }

    const std::string& GetPublicServerPassword() const {
        return PublicServerPassword;
    }

    const std::string& GetPublicServerUserName() const {
        return PublicServerUserName;
    }

    const int& GetPublicServerPort() const {
        return PublicServerPort;
    }

    const std::string& GetPublicServerIp() const {
        return PublicServerIp;
    }
    
    typedef std::map<std::string, SessionRef_t> ConnTable_t;
    
    ConnTable_t& GetVechicleConnTable() {
        return VechicleConnTable;
    }

    std::mutex& GetTableMutex() {
        return MtxForTable;
    }

    const size_t& GetHeartBeatCycle() const {
        return HeartBeatCycle;
    }

    boost::asio::io_service& GetIoService() {
        return IoService;
    }

    Logger& GetLogger() {
        return logger;
    }

    const size_t& GetUploadChannelNumber() const {
        return UploadChannelNumber;
    }

    const uint8_t& GetMode() const {
        return Mode;
    }

private:
    Resource();
    
    static Resource* s_resource;
    
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;

    int EnterprisePlatformTcpServicePort;

    /*
     * 等待公共平台回应超时（ReadResponseTimeOut），
     */
    size_t ReadResponseTimeOut;
    /* 
     * 登入没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 3（LoginTimes）次登入没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    size_t LoginIntervals;
    size_t LoginIntervals2;
    size_t LoginTimes;
    /*
     * 校验失败：
     *  国标：重发本条实时信息（应该是调整后重发）。每隔1min（CarDataResendIntervals）重发，最多发送3(MaxSendCarDataTimes)次
     *  国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
     *  由于平台无法做到实时调整，只能转发错误回应给车机，继续发下一条。所以不再重发本条实时信息，等车机调整后直接转发。
     * 未响应：
     *  国标2：如客户端平台未收到应答，平台重新登入
     */
    //    size_t CarDataResendIntervals;
    //    size_t MaxSendCarDataTimes;

    enumEncryptionAlgorithm EncryptionAlgorithm;

    //    blockqueue::BlockQueue<DataPacketForward*>* dataQueue;

    /* 平台登入登出的唯一识别号，
     * 城市邮政编码 + VIN前三位（地方平台使用GOV）+ 两位自定义数据 + "000000"
     */
    std::string PaltformId;
    size_t ReSetupPeroid; // tcp 中断后每隔几秒重连

    //    gavinsocket::GSocketClient* tcpConnection;
    size_t MaxSerialNumber;    
    size_t HeartBeatCycle;  // 车机与我平台的默认心跳周期
    size_t UploadChannelNumber;
    
    uint8_t Mode;

    boost::shared_ptr<blockqueue::BlockQueue<BytebufSPtr_t>> VehicleDataQueueSPtr;
//    boost::shared_ptr<gsocket::GSocket> TcpConnWithPublicPlatformSPtr;
    
    ConnTable_t VechicleConnTable;
    std::mutex MtxForTable;
    
    boost::asio::io_service IoService;
    Logger logger;
};

#endif /* RESOURCE_H */

