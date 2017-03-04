/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSender.h
 * Author: 10256
 *
 * Created on 2017年2月16日, 下午3:54
 */

#ifndef DATASENDER_H
#define DATASENDER_H
#include "DataFormat.h"
#include "DBConnection.h"

#pragma pack(1)
#pragma pack()

class DataSender : public DBConnection {
public:
    DataSender(StaticResource* staticResource);
//    DataSender(const DataSender& orig);
    virtual ~DataSender();
    void run(void);
private:
    LoginData_t m_loginData;
    LogoutData_t m_logoutData;
    DataPacketHeader_t m_headerForLogin;
    DataPacketHeader_t m_headerForLogout;
    DataPacketHeader_t m_headerForCarSig;
    
    DataPtrLen* m_carSigDataPtrLen;
    uint16_t m_serialNumber;
//    sql::Connection* DBConn;
//    sql::Statement* DBState;
    
//    StaticResource* m_staticResource;
//    blockqueue::BlockQueue<bytebuf::ByteBuffer*>* m_dataQueue;
    int m_writeFd;
    int m_RWFd;
    time_t m_lastUploadTime;
    void updateLoginData(struct tm* nowTM);
    void updateLogoutData();
    uint8_t readResponse(const int& timeout);
    void sendDataTask(void);
    void updateLastUploadTimeIntoDB(void);
    void tcpSendData(const enumCmdCode& cmd);
    void sendData();
    void updateHeader();
//    void closeDBConnection();

};

#endif /* DATASENDER_H */

