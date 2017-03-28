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
#include "Util.h"
#include "DataPtrLen.h"
#include <exception.h>
#include <warning.h>
#include <string.h>
#include <thread>


using namespace std;
using namespace sql;

using namespace bytebuf;
using namespace blockqueue;

const static string sqlTemplate = "update car_infokey set infovalue=(?) where infokey='gblastupload'";

DataSender::DataSender(StaticResource* staticResource) :
DBConnection(staticResource),
m_lastUploadTime(0),
m_carSigDataPtrLen(NULL),
m_RWFd(-1),
m_serialNumber(1) {
    if (m_staticResource->PublicServerUserName.length() != sizeof (m_loginData.username)
            || sizeof (m_loginData.password) != (m_staticResource->PublicServerPassword.length()))
        throw runtime_error("DataSender(): PublicServerUserName or PublicServerPassword Illegal");

    m_staticResource->PublicServerUserName.copy((char*) m_loginData.username, sizeof (m_loginData.username));
    m_staticResource->PublicServerPassword.copy((char*) m_loginData.password, sizeof (m_loginData.password));
    m_loginData.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;

    m_headerForLogin.CmdId = 5;
    m_headerForLogout.CmdId = 6;

    m_headerForCarSig.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;
    m_headerForLogin.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;
    m_headerForLogout.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;

    m_headerForLogin.dataUnitLength = sizeof (LoginData_t);
    m_headerForLogout.dataUnitLength = sizeof (LogoutData_t);
    Util::BigLittleEndianTransfer(&m_headerForLogin.dataUnitLength, 2);
    Util::BigLittleEndianTransfer(&m_headerForLogout.dataUnitLength, 2);
}

//DataSender::DataSender(const DataSender& orig) {
//}

DataSender::~DataSender() {
}

void DataSender::run() {
    if (NULL == m_staticResource->dataQueue) {
        cout << "DataQueue is NULL" << endl;
        return;
    }

    if (NULL == m_DBConn
            || m_DBConn->isClosed()
            || NULL == m_DBState) {
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
        getLastUploadInfoFromDB(m_lastUploadTime);
        closeDBConnection();

        for (;;) {
            m_carSigDataPtrLen = m_staticResource->dataQueue->take();
            if (0 >= m_carSigDataPtrLen->m_dataBuf->remaining()) {
                delete m_carSigDataPtrLen;
                throw runtime_error("sendDataTask(): take an empty realtimeData");
            }

            cout << "\n\nSender get realtimeData from " << m_carSigDataPtrLen->m_vin << endl;
            sendData();
            m_lastUploadTime = m_carSigDataPtrLen->getCollectTime();
            setupDBConnection();
            updateLastUploadTimeIntoDB();
            closeDBConnection();
            delete m_carSigDataPtrLen;
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }

}

void DataSender::updateLoginData() {
    time_t now;
    struct tm* nowTM;
    now = time(NULL);
    nowTM = localtime(&now);

    m_loginData.year = nowTM->tm_year - 100;
    m_loginData.mon = nowTM->tm_mon + 1;
    m_loginData.mday = nowTM->tm_mday;
    m_loginData.hour = nowTM->tm_hour;
    m_loginData.min = nowTM->tm_min;
    m_loginData.sec = nowTM->tm_sec;
    m_loginData.serialNumber = m_serialNumber;
    Util::BigLittleEndianTransfer(&m_loginData.serialNumber, 2);
}

void DataSender::updateLogoutData() {
    time_t now;
    struct tm* nowTM;
    now = time(NULL);
    nowTM = localtime(&now);

    m_logoutData.year = nowTM->tm_year - 100;
    m_logoutData.mon = nowTM->tm_mon + 1;
    m_logoutData.mday = nowTM->tm_mday;
    m_logoutData.hour = nowTM->tm_hour;
    m_logoutData.min = nowTM->tm_min;
    m_logoutData.sec = nowTM->tm_sec;
    m_logoutData.serialNumber = m_loginData.serialNumber;
}

/*
 * block to read server response ... not done
 * @return
 * 0 if response ok
 * 1 if timeout
 * 2 response not ok
 * -1 read fail
 */
uint8_t DataSender::readResponse(const int& timeout) {
    if (0 > m_RWFd)
        throw runtime_error("DataSender::readResponse(): TCP server hasn't setup");
    uint8_t readBuf[BUF_SIZE] = {0};
    size_t n = read(m_RWFd, readBuf, BUF_SIZE);

    if (readBuf[0] == 'o' && readBuf[1] == 'k') {
        cout << "Sender: Public Server Response ok" << endl;
        return OK;
    }
    if (n > 0)
        cout << "read " << n << " bytes data: " << (char*) readBuf << endl;
    else
        cout << "read return " << n << endl;
    // 应答标志定义： 1 成功，2错误，3 vin重复，0xfe 命令 not done
        
    return -1;
}

void DataSender::updateLastUploadTimeIntoDB(void) {
    PreparedStatement* prepStmt = NULL;
    try {
        char strCurrUploadTime[22] = {0};
        struct tm* currUploadTimeTM = localtime(&m_lastUploadTime);
        strftime(strCurrUploadTime, sizeof (strCurrUploadTime) - 1, TIMEFORMAT, currUploadTimeTM);

        prepStmt = m_DBConn->prepareStatement(sqlTemplate);
        prepStmt->setString(1, strCurrUploadTime);
        // expect only 1 row affected.
        assert(1 == prepStmt->executeUpdate());
#if DEBUG
        cout << "update DB last upload time to " << strCurrUploadTime << "\n\n" << endl;
#endif        
        delete prepStmt;
    } catch (SQLException &e) {
        if (NULL != prepStmt) {
            delete prepStmt;
            prepStmt = NULL;
        }
        cout << "updateLastUploadTimeIntoDB error" << endl;
        throw;
    }
}

void DataSender::updateHeader() {
    m_headerForCarSig.CmdId = m_carSigDataPtrLen->m_isReissue ? enumCmdCode::reissueUpload : enumCmdCode::carSignalDataUpload;
    m_carSigDataPtrLen->m_vin.copy((char*) m_headerForCarSig.vin, VINLEN);
    m_carSigDataPtrLen->m_vin.copy((char*) m_headerForLogin.vin, VINLEN);
    m_carSigDataPtrLen->m_vin.copy((char*) m_headerForLogout.vin, VINLEN);
    m_headerForCarSig.dataUnitLength = m_carSigDataPtrLen->m_dataBuf->capacity();
    Util::BigLittleEndianTransfer(&m_headerForCarSig.dataUnitLength, 2);
}

void DataSender::sendData() {
    time_t now;
    struct tm* timeTM;
    updateHeader();
    
    // 国标：每登入一次，登入流水号自动加1，从1开始循环累加，最大值为65531，循环周期为天。
    now = time(NULL);
    
//    cout << "[DEBUG1] now = " << Util::timeToStr(now) << endl;
    
    timeTM = localtime(&now);
    int mday = timeTM->tm_mday;
    int mon = timeTM->tm_mon;
    int year = timeTM->tm_year;
    localtime(&m_lastUploadTime);
    
    if (m_serialNumber > 65531
            || mday != timeTM->tm_mday
            || mon != timeTM->tm_mon
            || year != timeTM->tm_year)
        m_serialNumber = 1;

    // login
    updateLoginData();

    m_RWFd = Util::setupConnectionToTCPServer(m_staticResource->PublicServerIp, m_staticResource->PublicServerPort);

    try {
        for (uint8_t i = 0;; i++) {
            tcpSendData(enumCmdCode::platformLogin);
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
        m_serialNumber++;
#if DEBUG
        cout << "login done" << endl;
#endif

        // send car signal data
        for (uint8_t i = 0; i < 3; i++) {
            tcpSendData(enumCmdCode::carSignalDataUpload);
            if (0 == readResponse(1))
                break;
            sleep(m_staticResource->RealtimeDataResendTime);
        }
#if DEBUG
        cout << "send car signal data done" << endl;
#endif            

        // logout
        updateLogoutData();
        tcpSendData(enumCmdCode::platformLogout);
#if DEBUG
        cout << "logout done\n\n" << endl;
#endif            
        close(m_RWFd);
    } catch (exception& e) {
        close(m_RWFd);
        throw;
    }
}

void DataSender::tcpSendData(const enumCmdCode& cmd) {
#if !TCPNOTDONE
    if (0 > m_RWFd)
        throw runtime_error("DataSender::tcpSendData(): TCP server hasn't setup");
#endif
    void* headerPtr;
    void* dataUnitPtr;
    size_t headerSize, dataUnitSize;
    switch (cmd) {
        case enumCmdCode::platformLogin:
            headerPtr = &m_headerForLogin;
            dataUnitPtr = &m_loginData;
            headerSize = sizeof (m_headerForLogin);
            dataUnitSize = sizeof (m_loginData);
            break;
        case enumCmdCode::platformLogout:
            headerPtr = &m_headerForLogout;
            dataUnitPtr = &m_logoutData;
            headerSize = sizeof (m_headerForLogout);
            dataUnitSize = sizeof (m_logoutData);
            break;
        case enumCmdCode::carSignalDataUpload:
            headerPtr = &m_headerForCarSig;
            dataUnitPtr = m_carSigDataPtrLen->m_dataBuf->array();
            headerSize = sizeof (m_headerForCarSig);
            dataUnitSize = m_carSigDataPtrLen->m_dataBuf->capacity();
            break;
        default:
            char errMsg[256] = "Illegal cmd: ";
            snprintf(errMsg + strlen(errMsg), 1, "%d", cmd);
            throw runtime_error(errMsg);
    }

    uint8_t checkCode = Util::generateBlockCheckCharacter((uint8_t*)headerPtr + 2, headerSize - 2);
    checkCode = Util::generateBlockCheckCharacter(checkCode, dataUnitPtr, dataUnitSize);
#if DEBUG
//    cout << "check code = " << (short) checkCode << endl;
#endif

    cout << "Sender: " << headerSize + dataUnitSize + 1 << " bytes to be sent." << endl;
    Util::sendByTcp(m_RWFd, headerPtr, headerSize);
    Util::sendByTcp(m_RWFd, dataUnitPtr, dataUnitSize);
    Util::sendByTcp(m_RWFd, &checkCode, 1);
    //                m_RWFd = -1;
    //            cout << " fail" << endl;

}