/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MotorInfoUpload.h
 * Author: 10256
 *
 * Created on 2017年1月13日, 下午2:36
 */

#ifndef MOTORINFOUPLOAD_H
#define MOTORINFOUPLOAD_H
#include <string>
#include <vector>
#include "DataFormat.h"
#include "BlockQueue.h"

#include <time.h>
#include "driver.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "statement.h"
#include "prepared_statement.h"



class MotorInfoUpload {
public:
    MotorInfoUpload();    
    virtual ~MotorInfoUpload();
    bool initialize(void);
    /*
     * start 2 threads, one generate data and put to queue, the other take data from queue then send to server.
     */
    void uploadData(void);
    void generateDataTask(void);
    void sendDataTask(void);
    
private:
    std::string CServerIp;
    int CServerPort;
    
    std::string CDBHostName;
    std::string CDBUserName;
    std::string CDBPassword;
    
    // unit is second
    time_t m_period;
    int m_writeFd;
    int m_readFd;
    blockqueue::BlockQueue* m_dataQueue;
    std::vector<CarBaseInfo*> m_allCarArray;    //所有车机list
    sql::Driver* m_driver;
    sql::Connection* m_conn;
    sql::Statement* m_state;
    sql::PreparedStatement* m_prep_stmt;
    
    enumEncryptionAlgorithm m_encryptionAlgorithm;

// 登入时若等待回应超时（loginTimeout），重新发送登入数据。连续三次没有回应，等待loginTimeout2时间，继续重复登入流程。
    int LoginTimeout;
    int LoginTimeout2;    
    
    
    // get connection with Server, setup read & write fd.
    bool setupConnection(void);
    void closeConnection(void);
    
    bool login(void);
    void logout(void);
    
    void genLoginData(bytebuf::ByteBuffer* loginData);
    uint8_t readResponse(const int& timeout);
    bool updateCarList(void);
    bool genCarData(const int& carIndex, const bool isReissue, struct tm* currTime);
    bool getLastUploadInfo(int& carIndex, time_t& uploadTime);
    bool setupDBConnection(void);
    void closeDBConnection(void);
    bool genCompletelyBuildedVehicleData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    bool genDriveMotorData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    bool genEngineData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    bool genLocationData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    bool genExtremeValueData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    bool genAlarmData(const uint64_t& carId, bytebuf::ByteBuffer* DataOut);
    
    
    
    bool compressSignalDataWithLength(bytebuf::ByteBuffer* DataOut, const uint64_t& carId, const SignalInfo signalInfoArray[], const int& offset, const int& size);
    bool compressSignalData(uint32_t& DataOut, const uint64_t& carId, const byte signalCodeArray[], const int& offset, const int& size);
    void clearAndFreeAllCarArray(void);
};

#endif /* MOTORINFOUPLOAD_H */

