/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MotorInfoUpload.cpp
 * Author: 10256
 * 
 * Created on 2017年1月13日, 下午2:36
 */

#include "DataGenerator.h"
#include <thread>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include <exception.h>
#include <warning.h>
#include "SafeCopy.h"
#include "SignalTypeCode.h"
#include "CarData.h"
#include <functional>

using namespace std;
using namespace sql;
using namespace safecopy;
using namespace bytebuf;
using namespace blockqueue;

const static string sqlTemplate_car_signal = "select signal_value, build_time from car_signal where carid=(?) and signal_type=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1";
const static string sqlTemplate_car_signal1 = "select signal_value, build_time from car_signal1 where carid=(?) and signal_type=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1";
const static string sqlTemplate_car_trace_gps_info = "select latitude, longitude from car_trace_gps_info where carid=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1";

#if TEST
extern string vinForTest;
extern string StrlastUploadTimeForTest;
#endif

DataGenerator::DataGenerator(StaticResource* staticResource) : m_staticResource(staticResource) {
}

DataGenerator::~DataGenerator() {
}

/*
 * 车辆补发数据，我平台如何处理
 */

/*
0 "##" | 2 cmdUnit | 4 vin | 21 加密方式 | 22 DataUnitLength=72 | 24 DataUnit | 96 校验码 | 97
        
DataUnit: 6+1+20+1+13+1+9+1+14+1+5=72
    | 24 时间 | 30 整车数据 | 51 驱动数据 | 65 定位数据 | 75 极值数据 | 91 报警数据 | 97 

整车：
30 1 | 31 车辆状态 | 32 充电状态 | 33 运行模式=1 | 34 车速 | 36 累计里程 | 40 总电压 | 42 总电流 | 44 soc | 45 dcdc | 46 档位 | 47 绝缘电阻 | 49 预留=0 | 51
驱动电机：
51 2 | 52 个数=1 | 53 序号=1 | 54 电机状态 | 55 控制器温度 | 56 转速 | 58 转矩 | 60 温度 | 61 电压 | 63 电流 | 65
车辆位置：
65 5 | 66 定位数据 | 75
极值数据：
75 6 | 76 最高电压系统号 | 77 最高电压单体号 | 78 最高电压值 | 80 最低电压系统号 | 81 最低电压单体号 | 82 最低电压值 | 84 最高温系统号 | 
85 最高温探针号 | 86 最高温度值 | 87 最低温系统号 | 88 最低温探针号 | 89 最低温度值 | 90
报警数据：
90 7 | 91 最高报警等级 | 92 通用报警标志 | 96
 */

/*
 * task:
 * 1.	补发之前的周期没上传的数据。线程B
 * 2.	从数据库取最新的数据，生成以车机id为key的map中。线程B
 * 3.	订阅MQ，当收到某车机数据时更新map中某车机数据，然后上传public平台。线程A
 * 4.	当周期时间点到达时，遍历map上传全部车机数据。线程B
 * 5.	数据发送任务：发送时先生成登录数据并发送，然后发送车机数据，然后登出，然后更新最后一次完成时间。
 */

void DataGenerator::run(void) {
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

    m_prepStmtForSig = m_staticResource->DBConn->prepareStatement(sqlTemplate_car_signal);
    m_prepStmtForBigSig = m_staticResource->DBConn->prepareStatement(sqlTemplate_car_signal1);
    m_prepStmtForGps = m_staticResource->DBConn->prepareStatement(sqlTemplate_car_trace_gps_info);

    thread generateDataThreadA(&DataGenerator::generateDataTaskA, this);
    thread generateDataThreadB(&DataGenerator::generateDataTaskB, this);

    generateDataThreadA.join();
    generateDataThreadB.join();

}

void DataGenerator::generateDataTaskA() {
    // do task 3
    try {
        subscribeMq(); // block here, when MQ arrive, msgarrvd() will be called
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
}

// task 1 -> 2 -> 4

void DataGenerator::generateDataTaskB() {
    try {
        time_t lastTime;
        time_t periodTaskBeginTime;

        periodTaskBeginTime = time(NULL);

        // test is in function
        getLastUploadInfo(m_staticResource, lastTime);

        updateCarList();
#if TEST
        assert(m_allCarArray.size() == 1);
        assert(m_allCarArray[0].vin.compare(vinForTest));
#endif

        // 国标：补发的上报数据应为7日内通信链路异常期间存储的数据。
        if (periodTaskBeginTime - lastTime > 7 * 24 * 3600) {
            lastTime = periodTaskBeginTime - 7 * 24 * 3600;
        }

        m_currUploadTime = lastTime;

        // task 1: 补发之前错过的周期数据，从数据库取出数据生成CarData，然后将数据拷贝丢入队列，然后删除CarData实例
        for (m_currUploadTime += m_staticResource->Period; m_currUploadTime < periodTaskBeginTime; m_currUploadTime += m_staticResource->Period) {
            // 如果时间范围内没有数据，则不上传
            setTimeLimit(m_currUploadTime, m_currUploadTime - m_staticResource->Period);
            createCarListDataFromDB(true, true);
        }

        /*
         * task 2: 从数据库取最新的数据，生成CarData装入以车机id为key的map中。
         * 此时最早时间不再限制，因为需要在map中填充基础数据，如果限制，可能会取不到数据。
         */
        setTimeLimit(periodTaskBeginTime);
        createCarListDataFromDB(false, false);

        freePrepStatement();
        //logout();

        //task4: 当周期时间点到达时，遍历map，将最近一个周期内没有上传的车机数据丢入队列，最近一个周期内已上传过的车机不重复上传。
        for (;;) {
            sleep(m_staticResource->Period - (time(NULL) - periodTaskBeginTime));
            periodTaskBeginTime = time(NULL);
            for (CarDataMap::iterator it = m_staticResource->carDataMap.begin(); it != m_staticResource->carDataMap.end(); it++) {
                if (periodTaskBeginTime - it->second->getCollectTime() > m_staticResource->Period)
                    m_staticResource->dataQueue->put(it->second->createDataCopy());
            }
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        freePrepStatement();
    }
}

/*
 * update m_allCarArray with car_base_info
 */
void DataGenerator::updateCarList() {
    ResultSet *result = NULL;
    try {
        //clearAndFreeAllCarArray();
        m_allCarArray.clear();
        CarBaseInfo carBaseInfo;
#if TEST
        result = m_staticResource->DBState->executeQuery("select carid,vehicle_ident_code from car_base_info order by carid and vehicle_ident_code = '" + vinForTest + "'");
#else        
        result = m_staticResource->DBState->executeQuery("select carid,vehicle_ident_code from car_base_info order by carid");
#endif
        for (; result->next();) {
            carBaseInfo.carId = result->getUInt64(1);
            carBaseInfo.vin = result->getString(2);
            m_allCarArray.push_back(carBaseInfo);
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
        throw e;
    }
}

/*
 * @param send
 * if send is true, 丢到发送队列，不插入到DataMap，否则只插入到DataMap，不丢到发送队列。
 * 遍历从 beginCarIndex 开始的AllCarArray，生成每辆车的数据
 * For (CarIndex : List1) {
 * 	获取离time最近的CarIndex各项数据，标为补发并上传。
 * }
 */
void DataGenerator::createCarListDataFromDB(const bool& isReissue, const bool& send) {
    for (int carIndex = 0; carIndex < m_allCarArray.size(); carIndex++) {
        CarData* carData = new CarData(m_allCarArray[carIndex], this);

        // 时间范围内取到数据才上传
        carData->createByDataFromDB(isReissue, m_currUploadTime);
        if (send && !carData->noneGetData()) {
            m_staticResource->dataQueue->put(carData->createDataCopy());
            delete carData;
        } else {
            /*
             * task 2: 从数据库取最新的数据，生成ByteBuffer装入以车机id为key的map中。
             * 此时最早时间不再限制，因为需要在map中填充基础数据，如果限制，可能会取不到数据。
             * 所以此时 createByDataFromDB 必须返回true，且每个信号都必须取到值
             */
            assert(carData->allGetData());
            m_staticResource->carDataMap.insert(pair<string, CarData*>(m_allCarArray[carIndex].vin, carData));
        }
    }
}

/*
 * @param uploadTime
 * 上次传输的时间距起始日的秒数
 * 
 * @param carIndex
 * index of last car complete upload
 */
void DataGenerator::getLastUploadInfo(StaticResource* staticResource, time_t& uploadTime) {
    ResultSet *result = NULL;
    try {
        result = staticResource->DBState->executeQuery("select infovalue from car_infokey where infokey = 'gblastupload'");
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
        throw e;
    }
}

static void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

/*
 * 解析topic，payload，得到车机ID （信号码，信号值）数组
 * 去dataMap找到这个车机的CarData，循环调用 updateBySigTypeCode m_dataQueue->put(m_carDataIterator->second->createDataCopy());
 */
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    try {
        if (NULL == context)
            throw runtime_error("msgarrvd(): IllegalArgument context");

        DataGenerator* dataGenerator = (DataGenerator*) context;
        // topicName is expected like "/1234567/lpcloud/candata"
        if (NULL == topicName || 1 > topicLen)
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        size_t ofstAfterVin;

        if ('/' != *topicName)
            throw runtime_error("msgarrvd(): IllegalArgument topic");
        for (ofstAfterVin = 0; ofstAfterVin < topicLen; ofstAfterVin++) {
            if ('/' == *(topicName + ofstAfterVin))
                break;
        }

        if ((topicLen - ofstAfterVin) != (dataGenerator->m_staticResource->MQTopic.length() - 2))
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        if (0 != strncmp(topicName + ofstAfterVin, dataGenerator->m_staticResource->MQTopic.c_str() + 2, dataGenerator->m_staticResource->MQTopic.length() - 2))
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        string vin(topicName + 1, ofstAfterVin - 1);
        unique_lock<mutex> lk(dataGenerator->m_mutex);

        CarData* carData;
        CarDataMap::iterator it = dataGenerator->m_staticResource->carDataMap.find(vin);

        /* 
         * a new car upload MQ first time, its signal data is expected complete, 
         * generate CarData and insert into dataMap, then send it.
         */
        if (dataGenerator->m_staticResource->carDataMap.end() == it) {
            //updateCarList();
            carData = new CarData(vin, dataGenerator);
            dataGenerator->m_staticResource->carDataMap.insert(pair<string, CarData*>(vin, carData));
        } else {
            carData = it->second;
        }
        carData->updateStructByMQ((uint8_t*) message->payload, message->payloadlen);
        dataGenerator->m_staticResource->dataQueue->put(carData->createDataCopy());
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
}

void DataGenerator::subscribeMq() {

#define QOS         1

#define MQTTCLIENT_PERSISTENCE_NONE 1

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, m_staticResource->MQServerUrl.c_str(), m_staticResource->MQClientID.c_str(),
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = m_staticResource->MQServerUserName.c_str();
    conn_opts.password = m_staticResource->MQServerPassword.c_str();

    MQTTClient_setCallbacks(client, this, connlost, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        MQTTClient_destroy(&client);
        throw runtime_error("Failed to connect, return " + rc);
    }
    MQTTClient_subscribe(client, m_staticResource->MQTopic.c_str(), QOS);

    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

void DataGenerator::freePrepStatement() {
    if (NULL != m_prepStmtForSig)
        delete m_prepStmtForSig;
    if (NULL != m_prepStmtForBigSig)
        delete m_prepStmtForBigSig;
    if (NULL != m_prepStmtForGps)
        delete m_prepStmtForGps;
}

/*
 * set time limit for
 * select signal_value, build_time from car_signal where carid=(?) and signal_type=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1
 * select signal_value, build_time from car_signal1 where carid=(?) and signal_type=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1
 * select latitude, longitude from car_trace_gps_info where carid=(?) and build_time<(?) and build_time>(?) order by build_time desc limit 1
 */
void DataGenerator::setTimeLimit(const time_t& to, const time_t& from /* = 0*/) {
    if (to < from)
        throw runtime_error("setTimeLimit(): IllegalArgument");
    struct tm* timeTM;
    char strTime[20] = {0}; // is like "2017-01-17 15:33:03"
    
    timeTM = localtime(&to);
    strftime(strTime, 19, TIMEFORMAT, timeTM);
    m_prepStmtForSig->setDateTime(3, strTime);
    m_prepStmtForBigSig->setDateTime(3, strTime);
    m_prepStmtForGps->setDateTime(2, strTime);

    memset(strTime, 0, sizeof (strTime));
    timeTM = localtime(&from);
    strftime(strTime, 19, TIMEFORMAT, timeTM);
    m_prepStmtForSig->setDateTime(4, strTime);
    m_prepStmtForBigSig->setDateTime(4, strTime);
    m_prepStmtForGps->setDateTime(3, strTime);
}