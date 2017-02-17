/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataFormat.h
 * Author: 10256
 *
 * Created on 2017年1月9日, 上午11:22
 */

#ifndef DATAFORMAT_H
#define DATAFORMAT_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "ByteBuffer.h"
#include <time.h>
#include "driver.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "statement.h"
#include "BlockQueue.h"

#include <mutex>
#include <condition_variable>
#include <map>

#define BUF_SIZE 1024
#define INFO_SIZE 1024*5
#define VINLEN 17
#define TIMEFORMAT "%Y-%m-%d %H:%M:%S"
#define TIMEOUT 1
#define OK 0

class CarData;
typedef std::map<std::string, CarData*> CarDataMap;

typedef enum {
    null = 1,
    rsa = 2,
    aes128 = 3,
} enumEncryptionAlgorithm;

typedef enum {
    realtimeUpload = 2,
    reissueUpload = 3,
    platformLogin = 5,
    platformLogout = 6,
} enumCmdCode;

class DataPtrLen {
public:
    uint8_t* m_ptr;
    size_t m_length;

    DataPtrLen() : m_ptr(0), m_length(0) {
    }

    DataPtrLen(uint8_t* ptr, const size_t& length) : m_ptr(ptr), m_length(length) {
    }

    virtual ~DataPtrLen() {
    }

    void freeMemery(void) {
        if (NULL != m_ptr && m_length > 0)
            delete m_ptr;
    }
};

typedef struct {
    std::string PublicServerIp;
    int PublicServerPort;
    std::string PublicServerUserName;
    std::string PublicServerPassword;

    std::string DBHostName;
    std::string DBUserName;
    std::string DBPassword;

    std::string MQServerUrl;
    std::string MQTopic;
    std::string MQClientID;
    std::string MQServerUserName;
    std::string MQServerPassword;
    // 登入时若等待回应超时（loginTimeout），重新发送登入数据。连续三次没有回应，等待loginTimeout2时间，继续重复登入流程。
    int LoginTimeout;
    int LoginTimeout2;
    // 实时数据校验失败重发每个1min
    int RealtimeDataResendTime;
    time_t Period;

    enumEncryptionAlgorithm EncryptionAlgorithm;

    sql::Driver* DBDriver;
    sql::Connection* DBConn;
    sql::Statement* DBState;

    CarDataMap carDataMap;
    blockqueue::BlockQueue<DataPtrLen*>* dataQueue;

} StaticResource;

typedef struct TypeCarBaseInfo {
    uint64_t carId;
    std::string vin;
} CarBaseInfo;

#endif /* DATAFORMAT_H */
