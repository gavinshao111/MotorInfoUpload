/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Sender.h
 * Author: 10256
 *
 * Created on 2017年3月8日, 下午7:43
 */

#ifndef SENDER_H
#define SENDER_H

#include <sstream>
#include <fstream>
#include "DataFormatForward.h"
#include "../BlockQueue.h"
#include "GSocket.h"
#include "ByteBuffer.h"

namespace senderstatus {

    typedef enum {
        init = 0,
        responseOk = 1,
        timeout = 2,
        responseNotOk = 3,
        connectionClosed = 4,
        responseFormatErr = 5,
        carLoginOk = 6,
    } EnumSenderStatus;
}

class Sender {
public:
    Sender();
    virtual ~Sender();
    void run();
    void runForCarCompliance();

private:
    Sender(const Sender& orig);
    void tcpSend();
    void setupConnection();
    void updateLoginData();
    void updateLogoutData();
    void updateHeader();
    void readResponse(const int& timeout);
    void tcpSendData(const enumCmdCode& cmd);
    void forwardCarData();
    void setupConnAndLogin(const bool& needResponse = true);
    void logout();
    void static outputMsg(std::ostream& out, const std::string& vin, const time_t& collectTime, const time_t& sendTime, const bytebuf::ByteBuffer* data = NULL);

    blockqueue::BlockQueue<BytebufSPtr_t>&  s_carDataQueue;
    gsocket::GSocket& s_tcpConn;
    LoginDataForward_t m_loginData;
    LogoutDataForward_t m_logoutData;
    boost::shared_ptr<bytebuf::ByteBuffer> m_carData;
    uint16_t m_serialNumber;
    time_t m_lastloginTime;
    time_t m_lastSendTime;
    boost::shared_ptr<bytebuf::ByteBuffer> m_responseBuf;
    senderstatus::EnumSenderStatus m_senderStatus;
    std::stringstream m_stream;
};

#endif /* SENDER_H */

