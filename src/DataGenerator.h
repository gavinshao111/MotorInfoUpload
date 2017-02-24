/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataGenerator.h
 * Author: 10256
 *
 * Created on 2017年1月13日, 下午2:36
 */

#ifndef MOTORINFOUPLOAD_H
#define MOTORINFOUPLOAD_H
#include <vector>
#include "DataFormat.h"

#include <time.h>
#include "driver.h"
#include "mysql_connection.h"
#include "mysql_driver.h"
#include "statement.h"
#include "prepared_statement.h"
#include <mutex>
#include "DBConnection.h"

extern "C" {
#include "MQTTClient.h"
}

class DataGenerator : public DBConnection {
public:
    DataGenerator(StaticResource* staticResource);    
    virtual ~DataGenerator();
    /*
     * start 2 threads, one generate data and put to queue, the other take data from queue then send to server.
     */
    void run(void);
//    static void getLastUploadInfo(StaticResource* staticResource, time_t& uploadTime);

    friend class CarData;
    friend int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
    
private:
//    sql::Connection* DBConn;
//    sql::Statement* DBState;
//    StaticResource* m_staticResource;
    std::vector<CarBaseInfo> m_allCarArray;    //所有车机list
    sql::PreparedStatement* m_prepStmtForSig;
//    sql::PreparedStatement* m_prepStmtForBigSig;
    sql::PreparedStatement* m_prepStmtForGps;
//    sql::PreparedStatement* m_prepStmtForDecimal;
    
    // 数据采集时间,，当前需要上传的时间点
    time_t m_currUploadTime;
    std::mutex m_mutex;
    
    void generateDataTaskA(void);
    void generateDataTaskB(void);
    void updateCarList(void);
    void createCarListDataFromDB(const bool& isReissue, const bool& send);
    void subscribeMq();
    void freePrepStatement();
    void setTimeLimit(const time_t& to, const time_t& from = 0);
//    void closeDBConnection();
};

#endif /* MOTORINFOUPLOAD_H */

