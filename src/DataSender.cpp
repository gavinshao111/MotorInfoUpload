/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataSender.cpp
 * Author: 10256
 * 
 * Created on 2017年2月16日, 下午3:54
 */

#include "DataSender.h"
#include "DataGenerator.h"
#include "prepared_statement.h"
#include <exception.h>
#include <warning.h>
#include <string.h>

using namespace std;
using namespace sql;

using namespace bytebuf;
using namespace blockqueue;

const static string sqlTemplate = "update car_infokey set infovalue=(?) where infokey='gblastupload'";

DataSender::DataSender(StaticResource* staticResource) : m_staticResource(staticResource) {
    if (m_staticResource->PublicServerUserName.length() != sizeof(m_loginData.username)
            || sizeof(m_loginData.password) != (m_staticResource->PublicServerPassword.length()))
        throw runtime_error("PublicServerUserName or PublicServerPassword Illegal");
    
    m_staticResource->PublicServerUserName.copy((char*)m_loginData.username, sizeof(m_loginData.username));
    m_staticResource->PublicServerPassword.copy((char*)m_loginData.password, sizeof(m_loginData.password));
    m_loginData.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;
    //m_loginData.serialNumber = 1;
    m_lastUploadTime = 0;
}

DataSender::DataSender(const DataSender& orig) {
}

DataSender::~DataSender() {
}

void DataSender::run() {
    if (NULL == m_staticResource->dataQueue) {
        cout << "DataQueue is NULL" << endl;
        return;
    }
    if (NULL == m_staticResource->DBConn
            || m_staticResource->DBConn->isClosed()
            || NULL == m_staticResource->DBState) {
        cout << "DB hasn't setup" << endl;
        return;
    }

    sendDataTask();
}

/* 
 * task5: 发送时先生成登录数据并发送，然后发送车机数据，然后登出，然后更新最后一次完成时间。
 * take data from m_dataQueue 
 * do{
 *     send data
 * } while(read response is not ok);
 * update LastUploadInfo
 * 
 * we assert the network will be recovered soon.
 */
void DataSender::sendDataTask() {
    try {
        DataGenerator::getLastUploadInfo(m_staticResource, m_lastUploadTime);

        time_t now;
        struct tm* nowTM;
        struct tm* lastUploadTimeTM;

        for (;;) {
            DataPtrLen* realtimeData = m_staticResource->dataQueue->take();
            if (NULL == realtimeData->m_ptr || 0 == realtimeData->m_length) {
                delete realtimeData;
                throw runtime_error("sendDataTask(): take an empty realtimeData");
            }

            // 国标：每登入一次，登入流水号自动加1，从1开始循环累加，最大值为65531，循环周期为天。
            now = time(NULL);
            nowTM = localtime(&now);
            lastUploadTimeTM = localtime(&m_lastUploadTime);
            if (m_loginData.serialNumber > 65531
                    || nowTM->tm_mday != lastUploadTimeTM->tm_mday
                    || nowTM->tm_mon != lastUploadTimeTM->tm_mon
                    || nowTM->tm_year != lastUploadTimeTM->tm_year)
                m_loginData.serialNumber = 1;

            updateLoginTime(nowTM);
            login();
            m_loginData.serialNumber++;
            for (uint8_t i = 0; i < 3; i++) {
                sendData(realtimeData->m_ptr, realtimeData->m_length);
                if (0 == readResponse(1))
                    break;
                sleep(m_staticResource->RealtimeDataResendTime);
            }
            m_lastUploadTime = now;
            updateLastUploadTimeIntoDB();
            updateLogoutData();
            logout();
            delete realtimeData;
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }

}

void DataSender::login() {
    for (uint8_t i = 0;; i++) {
        sendData((uint8_t*)&m_loginData, sizeof(m_loginData));
        uint8_t rt = readResponse(m_staticResource->LoginTimeout);
        if (OK == rt)
            break;
        else if (TIMEOUT != rt) {
            cout << "readResponse return " << rt << endl;
            throw runtime_error("readResponse return " + rt);
        }
        if (3 == i) {
            i = 0;
            sleep(m_staticResource->LoginTimeout2);
        }
    }
}

void DataSender::logout() {
    sendData((uint8_t*)&m_logoutData, sizeof(m_logoutData));
}

void DataSender::updateLoginTime(struct tm* nowTM) {
    if (NULL == nowTM)
        throw runtime_error("updateLoginTime(): IllegalArgument");
    m_loginData.year = nowTM->tm_year - 100;
    m_loginData.mon = nowTM->tm_mon;
    m_loginData.mday = nowTM->tm_mday;
    m_loginData.hour = nowTM->tm_hour;
    m_loginData.min = nowTM->tm_min;
    m_loginData.sec = nowTM->tm_sec;
}

void DataSender::updateLogoutData() {
    time_t now;
    struct tm* nowTM;
    now = time(NULL);
    nowTM = localtime(&now);

    m_logoutData.year = nowTM->tm_year - 100;
    m_logoutData.mon = nowTM->tm_mon;
    m_logoutData.mday = nowTM->tm_mday;
    m_logoutData.hour = nowTM->tm_hour;
    m_logoutData.min = nowTM->tm_min;
    m_logoutData.sec = nowTM->tm_sec;
    m_logoutData.serialNumber = m_loginData.serialNumber;
}

/*
 * block to read server response
 * @return
 * 0 if response ok
 * 1 if timeout
 * 2 response not ok
 * -1 read fail
 */
uint8_t DataSender::readResponse(const int& timeout) {
    return OK;
}

void DataSender::updateLastUploadTimeIntoDB(void) {
    PreparedStatement* prepStmt = NULL;
    try {
        char strCurrUploadTime[20];
        struct tm* currUploadTimeTM = localtime(&m_lastUploadTime);
        memset(strCurrUploadTime, 0, sizeof (strCurrUploadTime));
        strftime(strCurrUploadTime, 19, TIMEFORMAT, currUploadTimeTM);

        prepStmt = m_staticResource->DBConn->prepareStatement(sqlTemplate);
        prepStmt->setString(1, strCurrUploadTime);
        // expect only 1 row affected.
        if (1 != prepStmt->executeUpdate())
            throw runtime_error("updateLastUploadTimeIntoDB(): update fail");
        delete prepStmt;
    } catch (SQLException &e) {
        if (NULL != prepStmt) {
            delete prepStmt;
            prepStmt = NULL;
        }
        throw e;
    }
}

void DataSender::sendData(uint8_t* addr, size_t length) {}