/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   resource.h
 * Author: 10256
 *
 * Created on 2017年5月16日, 下午3:37
 */

#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>
#include <map>
#include <boost/thread/mutex.hpp>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "../BlockQueue.h"
//#include "GSocket.h"
#include "DataFormatForward.h"
#include "TcpSession.h"
#include "PublicServer.h"

class resource {
public:
    
    static resource* getResource();
    virtual ~resource();

    blockqueue::BlockQueue<boost_bytebuf_sptr>& get_realtime_queue() {
        return *realtime_vehicle_data_queue_sptr;
    }

    blockqueue::BlockQueue<boost_bytebuf_sptr>& get_reissue_queue() {
        return *reissue_vehicle_data_queue_sptr;
    }

    const size_t& getMaxSerialNumber() const {
        return maxSerialNumber;
    }

    const size_t& getReSetupPeroid() const {
        return reSetupPeroid;
    }

    const std::string& getPaltformId() const {
        return paltformId;
    }

    const enumEncryptionAlgorithm& getEncryptionAlgorithm() const {
        return encryptionAlgorithm;
    }

    const size_t& getLoginTimes() const {
        return loginTimes;
    }

    const size_t& getLoginIntervals2() const {
        return loginIntervals2;
    }

    const size_t& getLoginIntervals() const {
        return loginIntervals;
    }

    const int& getEnterprisePlatformTcpServicePort() const {
        return enterprisePlatformTcpServicePort;
    }

    const std::string& getPublicServerPassword() const {
        return publicServerPassword;
    }

    const std::string& getPublicServerUserName() const {
        return publicServerUserName;
    }

    const int& getPublicServerPort() const {
        return publicServerPort;
    }

    const std::string& getPublicServerIp() const {
        return publicServerIp;
    }
    
    typedef std::map<std::string, SessionRef_t> SessionTable_t;
    
    SessionTable_t& getVechicleSessionTable() {
        return vechicleSessionTable;
    }

    boost::mutex& getTableMutex() {
        return mtxForTable;
    }

    const size_t& getHeartBeatCycle() const {
        return heartBeatCycle;
    }

    boost::asio::io_service& getIoService() {
        return ioService;
    }

    const size_t& getUploadChannelNumber() const {
        return uploadChannelNumber;
    }

    const uint8_t& getMode() const {
        return mode;
    }

    const enumSystem& getSystem() const {
        return system;
    }

    const std::vector<std::string>& getVinAllowedArray() const {
        return vinAllowedArray;
    }

private:
    resource();
    
    static resource* s_resource;
    
    std::string publicServerIp;
    int publicServerPort;
    std::string publicServerUserName;
    std::string publicServerPassword;

    int enterprisePlatformTcpServicePort;

    /* 
     * 登入没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 3（LoginTimes）次登入没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    size_t loginIntervals;
    size_t loginIntervals2;
    size_t loginTimes;
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

    enumEncryptionAlgorithm encryptionAlgorithm;

    /* 平台登入登出的唯一识别号，
     * 城市邮政编码 + VIN前三位（地方平台使用GOV）+ 两位自定义数据 + "000000"
     */
    std::string paltformId;
    size_t reSetupPeroid; // tcp 中断后每隔几秒重连

    //    gavinsocket::GSocketClient* tcpConnection;
    size_t maxSerialNumber;    
    size_t heartBeatCycle;  // 车机与我平台的默认心跳周期
    size_t uploadChannelNumber;
    
    uint8_t mode;

    boost::shared_ptr<blockqueue::BlockQueue<boost_bytebuf_sptr>> realtime_vehicle_data_queue_sptr;
    boost::shared_ptr<blockqueue::BlockQueue<boost_bytebuf_sptr>> reissue_vehicle_data_queue_sptr;
//    boost::shared_ptr<gsocket::GSocket> TcpConnWithPublicPlatformSPtr;
    
    SessionTable_t vechicleSessionTable;
    boost::mutex mtxForTable;
    
    boost::asio::io_service ioService;
    enumSystem system;
    // 符合性检测下允许上传车机vin列表，以逗号隔开    
    // 构造函数中为vinAllowedArray重新赋值（移动）前会先析构其默认构造的资源，由于其默认构造资源开销小，所以可以这样写。
    std::vector<std::string> vinAllowedArray;
};

#endif /* RESOURCE_H */

