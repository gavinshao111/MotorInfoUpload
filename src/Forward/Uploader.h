/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Uploader.h
 * Author: 10256
 *
 * Created on 2017年7月1日, 下午4:48
 */

#ifndef UPLOADER_H
#define UPLOADER_H

#include "DataFormatForward.h"
#include "../BlockQueue.h"
#include "PublicServer.h"
#include "ByteBuffer.h"
#include "ResponseReader.h"
#include "thread.h"

namespace uploaderstatus {

    typedef enum {
        init = 0,
        connectionClosed = 4,
    } EnumUploaderStatus;
}

typedef enum {
    release = 0,
    vehicleCompliance = 1,
    platformCompliance = 2,
} EnumRunMode;

class resource;

class Uploader : public gavin::thread {
public:
    Uploader(const size_t& no);
    virtual ~Uploader();

    friend class resource;
private:
    Uploader(const Uploader& orig);
    void tcpSend();
    void setupConnection();
    void updateLoginData();
    void updateLogoutData();
    void updateHeader();
    void tcpSendData(const uint8_t& cmd);
    void forwardCarData();
    void setupConnAndLogin();
    void logout(bool needResponse = true);
    void static outputMsg(const enumSystem& system, const std::string& vin,
            const time_t& collectTime, const time_t& sendTime, bytebuf::ByteBuffer* data = NULL);
    void run() override;

    resource* r_resource;
    blockqueue::BlockQueue<boost_bytebuf_sptr>& r_reissue_queue;
    blockqueue::BlockQueue<boost_bytebuf_sptr>& r_realtime_queue;
    //    gsocket::GSocket& s_tcpConn;
    PublicServer m_publicServer;
    LoginDataForward_t m_loginData;
    LogoutDataForward_t m_logoutData;
    boost_bytebuf_sptr m_carData;
    static uint16_t m_serialNumber;
#if UseBoostMutex
    static boost::mutex m_serial_number_mtx;
#else
    static std::mutex m_serial_number_mtx;
#endif
    static time_t m_uploader_last_login_time;
    time_t m_lastSendTime;
    uploaderstatus::EnumUploaderStatus m_uploaderStatus;

    std::string m_vin;
    DataPacketHeader_t* m_packetHdr;
    ResponseReader m_responseReader;
    EnumRunMode m_mode;
    std::string m_id;
};

#endif /* UPLOADER_H */
