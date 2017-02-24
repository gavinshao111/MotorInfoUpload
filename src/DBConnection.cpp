/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DBConnection.cpp
 * Author: 10256
 * 
 * Created on 2017年2月20日, 下午5:29
 */

#include "DBConnection.h"
#include <string>
#include <exception.h>

using namespace std;
using namespace sql;

#if TEST
extern string vinForTest;
extern string StrlastUploadTimeForTest;
#endif

DBConnection::DBConnection(StaticResource* staticResource) : m_staticResource(staticResource) {setupDBConnection();}

DBConnection::DBConnection(const DBConnection& orig) {
}

DBConnection::~DBConnection() {
    closeDBConnection();
}

void DBConnection::setupDBConnection() {
    m_DBConn = m_staticResource->DBDriver->connect(m_staticResource->DBHostName, m_staticResource->DBUserName, m_staticResource->DBPassword);
    //conn->setAutoCommit(false);
    m_DBState = m_DBConn->createStatement();
    m_DBState->execute("use lpcarnet");

//    cout << "setupDBConnection: " << m_staticResource.DBConn->isClosed() << endl;
}

void DBConnection::closeDBConnection() {
    if (NULL != m_DBState) {
        m_DBState->close();
        delete m_DBState;
    }
    if (NULL != m_DBConn && !m_DBConn->isClosed()) {
        m_DBConn->close();
        delete m_DBConn;
    }
}

void DBConnection::getLastUploadInfo(time_t& uploadTime) {
    ResultSet *result = NULL;
    try {
        result = m_DBState->executeQuery("select infovalue from car_infokey where infokey = 'gblastupload'");
        if (result->next()) {
            //carIndex = result->getInt("carIndex");
            string strtime = result->getString("infovalue");
#if TEST
            assert(0 == StrlastUploadTimeForTest.compare(strtime));
#endif

            struct tm tmTemp;
            strptime(strtime.data(), TIMEFORMAT, &tmTemp);
            uploadTime = mktime(&tmTemp);
        } else {
            //cout <<  << endl;
            delete result;
            throw runtime_error("getLastUploadInfo: gblastupload is empty");
        }

        if (NULL != result) {
            delete result;
            result = NULL;
        }
    } catch (SQLException &e) {
        if (NULL != result) {
            delete result;
            result = NULL;
        }
        throw;
    }
}