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
//#include "DataPacketForward.h"
#include "../DataFormat.h"
#include "DataFormatForward.h"
#include "ByteBuffer.h"

namespace senderstatus {

    typedef enum {
        ok = 0,
        timeout = 1,
        notOk = 2,
        error = 3,
        connectionClosed = 4,
    } EnumSenderStatus;
}

//class PacketForwardInfo {
//public:
//
//    PacketForwardInfo();
//    PacketForwardInfo(const PacketForwardInfo& orig);
//    virtual ~PacketForwardInfo();
//
//    bool isEmpty();
//
//    void putData(DataPacketForward* carData);
//    DataPacketForward* getData() const;
//    void clear();
//    std::string& vin();
//    std::string BaseInfoToString();
//    void output(std::ostream& out);
//    void sendTime(const time_t& time);
//private:
//    DataPacketForward* m_carData;
//    time_t m_sendTime;
//    std::stringstream m_stringstream;
//};

class Sender {
public:
    Sender(StaticResourceForward& staticResource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection);
    Sender(const Sender& orig);
    virtual ~Sender();

    void run();
    void runForCarCompliance();
private:
    size_t m_reissueNum;
//    PacketForwardInfo m_packetForwardInfo;
    StaticResourceForward& s_staticResource;
    DataQueue_t& s_carDataQueue;
    DataQueue_t& s_responseDataQueue;
    TcpConn_t& s_tcpConn;
    LoginDataForward_t m_loginData;
    LogoutDataForward_t m_logoutData;
//    DataPacketForward* m_carData;
    boost::shared_ptr<bytebuf::ByteBuffer> m_carData;

    uint16_t m_serialNumber;
    bool m_setupTcpInitial;
    time_t m_lastloginTime;
    time_t m_lastSendTime;
    bytebuf::ByteBuffer* m_readBuf;
    DataPacketHeader_t* m_responseHdr;

    senderstatus::EnumSenderStatus m_senderStatus;
    std::stringstream m_stream;

    void tcpSend();
    void setupConnection();
    void updateLoginData();
    void updateLogoutData();
    void updateHeader();
    void readResponse(const int& timeout);
    void tcpSendData(const enumCmdCode& cmd);

    void sendData();
    void login(const bool& needResponse = true);
    void logout();
    void forwardCarData(const bool& needResponse = true);
    void static outputMsg(std::ostream& out, const std::string& vin, const time_t& collectTime, const time_t& sendTime, const bytebuf::ByteBuffer* data = NULL);
};

#endif /* SENDER_H */

