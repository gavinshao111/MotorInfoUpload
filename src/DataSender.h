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
typedef struct {
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    // 登入登出流水号 may be we should store it in DB.
    uint16_t serialNumber;
    uint8_t username[12];
    uint8_t password[20];
    uint8_t encryptionAlgorithm;
} LoginData_t;
typedef struct {
    uint8_t year;
    uint8_t mon;
    uint8_t mday;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t serialNumber;
} LogoutData_t;
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
//    sql::Connection* DBConn;
//    sql::Statement* DBState;
    
//    StaticResource* m_staticResource;
//    blockqueue::BlockQueue<bytebuf::ByteBuffer*>* m_dataQueue;
    int m_writeFd;
    int m_readFd;
    time_t m_lastUploadTime;
    void updateLoginTime(struct tm* nowTM);
    void updateLogoutData();
    uint8_t readResponse(const int& timeout);
    void login(void);
    void logout(void);
    // get connection with Server, setup read & write fd.
    void setupConnection(void);
    void closeConnection(void);
    void sendDataTask(void);
    void updateLastUploadTimeIntoDB(void);
    void sendData(uint8_t* addr, size_t length);
//    void closeDBConnection();

};

#endif /* DATASENDER_H */

