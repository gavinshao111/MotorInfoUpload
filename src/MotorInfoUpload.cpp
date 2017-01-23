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

#include "MotorInfoUpload.h"
//#include "CompletelyBuildedVehicle.h"
#include <thread>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "prepared_statement.h"

#include <exception.h>
#include <warning.h>

#include "SafeCopy.h"

using namespace std;
using namespace sql;
using namespace safecopy;
using namespace bytebuf;

#define USEPreparedStat true
static char sqlBuf[500];

#if USEPreparedStat
const static string sqlTemplate = "select signal_value, build_time from car_signal where carid=(?) and signal_type=(?) and build_time<(?) order by build_time desc limit 1";
#else
static char sqlTemplate[163] = "select signal_value, build_time from car_signal where carid=           and signal_type=    and build_time<'                   ' order by build_time desc limit 1";
const static byte caridOfst = 60;
const static byte signal_typeOfst = 187;
const static byte build_timeOfst = 107;
const static string caridPlaceHolder = "          ";
const static string signal_typePlaceHolder = "   ";
const static string build_timePlaceHolder = "                   ";
/*
 * total length is 174, offset after "carid=" is 73, offset after "signal_type=" is 100, offset after "build_time<'" is 120
 */
#endif

MotorInfoUpload::MotorInfoUpload() {
}

MotorInfoUpload::~MotorInfoUpload() {
    delete m_dataQueue;
}

bool MotorInfoUpload::initialize() {
    CServerIp = "10.34.16.76";
    CServerPort = 1234;
    m_period = 24 * 3600;

    m_driver = get_driver_instance();

    m_encryptionAlgorithm = enumEncryptionAlgorithm::null;
    CDBHostName = "tcp://120.26.86.124:3306";
    CDBUserName = "root";
    CDBPassword = "10214";
    LoginTimeout = 60;
    LoginTimeout2 = 180;
    m_dataQueue = new blockqueue::BlockQueue(100);

    return true;
}

bool MotorInfoUpload::login(void) {
    CarData* pLoginData = new CarData(INFO_SIZE);

    for (byte i = 0;; i++) {
        genLoginData(pLoginData->m_data);
        m_dataQueue->put(pLoginData);
        byte rt = readResponse(LoginTimeout);
        if (OK == rt)
            break;
        else if (TIMEOUT != rt) {
            cout << "readResponse return " << rt << endl;
            return false;
        }
        if (3 == i) {
            i = 0;
            sleep(LoginTimeout2);
        }
    }
    return true;
}

void MotorInfoUpload::uploadData(void) {

    thread generateDataThread(&MotorInfoUpload::generateDataTask, this);
    thread sendDataThread(&MotorInfoUpload::sendDataTask, this);

    generateDataThread.join();
    sendDataThread.join();

}

void MotorInfoUpload::logout(void) {
}

void MotorInfoUpload::generateDataTask() {
    if (NULL == m_dataQueue) {
        cout << "m_dataQueue is NULL" << endl;
        return;
    }

    if (!login()) return;

    // 一次循环就是一次完整的上传流程，包括补发之前通讯异常期间没上传的数据，包括本周期需要上传的实时数据。

    int reissueCarIndex;
    time_t lastTime;
    time_t currUploadTime; // 当前需要上传的时间
    time_t periodTaskBeginTime;
    struct tm* currUploadTimeTM;

    char strCurrUploadTime[20]; // is like "2017-01-17 15:33:03"
    for (;;) {
        if (!setupDBConnection())
            return;

        if (!updateCarList())
            return;

        if (!getLastUploadInfo(reissueCarIndex, lastTime))
            return;
        reissueCarIndex++;

        periodTaskBeginTime = time(NULL);
        // 国标：补发的上报数据应为7日内通信链路异常期间存储的数据。
        if (periodTaskBeginTime - lastTime > 7 * 24 * 3600) {
            lastTime = periodTaskBeginTime - 7 * 24 * 3600;
            reissueCarIndex = 0;
        }

        currUploadTime = lastTime;
        currUploadTimeTM = localtime(&currUploadTime);

        // 补发发生异常时的那次周期传输任务中断后没上传的车机的数据
        memset(strCurrUploadTime, 0, sizeof (strCurrUploadTime));
        strftime(strCurrUploadTime, 19, TIMEFORMAT, currUploadTimeTM);

#if USEPreparedStat
        m_prep_stmt->setDateTime(3, strCurrUploadTime);
#else
        memcpy(sqlTemplate + build_timeOfst, build_timePlaceHolder.data(), build_timePlaceHolder.length());
        memcpy(sqlTemplate + build_timeOfst, strCurrUploadTime, strlen(strCurrUploadTime));
#endif

        if (!genCarData(reissueCarIndex, true, currUploadTimeTM))
            return;

        // 补发之前错过的周期数据
        for (currUploadTime += m_period; currUploadTime < periodTaskBeginTime - m_period; currUploadTime += m_period) {
            if (!genCarData(1, true, currUploadTimeTM))
                return;
        }

        // 上传本周期数据,此时 currUploadTime == now
        if (!genCarData(1, false, currUploadTimeTM))
            return;

        closeDBConnection();
        sleep(m_period - (time(NULL) - periodTaskBeginTime));
    }
}

/*
 * take data from m_dataQueue 
 * do{
 *     send data
 * } while(read response is not ok);
 * update LastUploadInfo
 * 
 * we assert the network will be recovered soon.
 */
void MotorInfoUpload::sendDataTask() {
    CarData* carData = m_dataQueue->take();
    //printf("%s: %.*s\n", "sendDataTask", (int)ptrLen->length, ptrLen->array);
}

void MotorInfoUpload::genLoginData(ByteBuffer* loginData) {

}

/*
 * block to read server response
 * @return
 * 0 if response ok
 * 1 if timeout
 * 2 response not ok
 * -1 read fail
 */
uint8_t MotorInfoUpload::readResponse(const int& timeout) {
    return OK;
}

/*
 * update m_allCarArray with car_base_info
 */
bool MotorInfoUpload::updateCarList() {
    ResultSet *result;
    try {
        clearAndFreeAllCarArray();
        CarBaseInfo* pCarBaseInfo = new CarBaseInfo;
        result = m_state->executeQuery("select carid,vehicle_ident_code from car_base_info order by carid");

        for (; result->next();) {
            pCarBaseInfo->carId = result->getUInt64(1);
            pCarBaseInfo->vin = result->getString(2);
            m_allCarArray.push_back(pCarBaseInfo);
        }

        delete result;
        return true;
    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        delete result;
        closeDBConnection();
        return false;
    }
}

/*
 * 遍历从 beginCarIndex 开始的AllCarArray，生成每辆车的数据
 * For (CarIndex : List1) {
 * 	获取离time最近的CarIndex各项数据，标为补发并上传。
 * }
 */
bool MotorInfoUpload::genCarData(const int& beginCarIndex, const bool isReissue, struct tm* currTime) {
    if (beginCarIndex < 0 || NULL == currTime)
        return false;

    try {
        for (int carIndex = beginCarIndex; carIndex < m_allCarArray.size(); carIndex++) {

            CarData* carData = new CarData(INFO_SIZE);
            carData->m_carIndex = carIndex;
            carData->m_data->put("##");
            carData->m_data->put(isReissue ? 0x03 : 0x02);
            carData->m_data->put(0xfe);
            carData->m_data->put(m_allCarArray[carIndex]->vin);
            carData->m_data->put(m_encryptionAlgorithm);
            size_t posOfDataUnitLen = carData->m_data->getPosition();
            carData->m_data->movePosition(true, 2);

            // put data unit

            // put data collect time
            carData->m_data->put((byte) currTime->tm_year - 100); // time is from 1900
            carData->m_data->put((byte) currTime->tm_mon);
            carData->m_data->put((byte) currTime->tm_mday);
            carData->m_data->put((byte) currTime->tm_hour);
            carData->m_data->put((byte) currTime->tm_min);
            carData->m_data->put((byte) currTime->tm_sec);

            if (genCompletelyBuildedVehicleData(m_allCarArray[carIndex]->carId, carData->m_data))
                return false;

            //61为充电状态信号码
            uint32_t isInParkingCharge;
            byte sigArray[] = {61};
            if (compressSignalData(isInParkingCharge, m_allCarArray[carIndex]->carId, sigArray, 0, 1))
                return false;

            if (1 == isInParkingCharge) {
                if (genDriveMotorData(m_allCarArray[carIndex]->carId, carData->m_data))
                    return false;
                if (genEngineData(m_allCarArray[carIndex]->carId, carData->m_data))
                    return false;
            }
            if (genLocationData(m_allCarArray[carIndex]->carId, carData->m_data))
                return false;
            if (genExtremeValueData(m_allCarArray[carIndex]->carId, carData->m_data))
                return false;
            if (genAlarmData(m_allCarArray[carIndex]->carId, carData->m_data))
                return false;

            // put data len
            ushort dataLen = carData->m_data->getPosition() - posOfDataUnitLen - 2;
            carData->m_data->modifyMemery((byte*) & dataLen, 0, 2, posOfDataUnitLen);

            // put check code. 国标：采用BCC（异或校验）法，校验范围从命令单元的第一个字节开始，同后一字节异或，直到校验码前一字节为止，校验码占用一个字节
            size_t currPos = carData->m_data->getPosition();
            byte checkCode = carData->m_data->at(2);
            for (int i = 3; i <= currPos; i++)
                checkCode ^= carData->m_data->at(i);
            carData->m_data->put(checkCode);

            carData->m_data->flip();
        }
        return true;
    } catch (byte e) {
        cout << "ERROR: ByteBuffer Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "error code: " << (ushort) e << endl;
        closeDBConnection();
        return false;
    }

}

/*
 * @param uploadTime
 * 上次传输的时间距起始日的秒数
 * 
 * @param carIndex
 * index of last car complete upload
 */
bool MotorInfoUpload::getLastUploadInfo(int& carIndex, time_t& uploadTime) {
    ResultSet *result;
    try {
        result = m_state->executeQuery("select * from car_lastupload");
        if (result->next()) {
            carIndex = result->getInt("carIndex");
            string strtime = result->getString("time");
            struct tm tmTemp;
            strptime(strtime.data(), TIMEFORMAT, &tmTemp);
            uploadTime = mktime(&tmTemp);
        } else {
            cout << "car_lastupload is empty." << endl;
            delete result;
            return false;
        }

        delete result;
        return true;

    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        delete result;
        closeDBConnection();
        return false;
    }
}

bool MotorInfoUpload::setupDBConnection() {
    try {
        m_conn = m_driver->connect(CDBHostName, CDBUserName, CDBPassword);
        //conn->setAutoCommit(false);
        m_state = m_conn->createStatement();
        m_state->execute("use lpcarnet");
#if USEPreparedStat
        m_prep_stmt = m_conn->prepareStatement(sqlTemplate);
#endif    
        return true;
    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        closeDBConnection();
        return false;
    }
}

void MotorInfoUpload::closeDBConnection() {
#if USEPreparedStat
    if (NULL != m_prep_stmt)
        delete m_prep_stmt;
#endif

    if (NULL != m_state) {
        m_state->close();
        delete m_state;
    }
    if (NULL != m_conn) {
        m_conn->close();
        delete m_conn;
    }
}

bool MotorInfoUpload::genCompletelyBuildedVehicleData(const uint64_t& carId, ByteBuffer* DataOut) {
    // 整车数据信号码
    const SignalInfo CcompletelyBuildedVehicleSigInfo[] = {
        {60, 1},
        {61, 1},
        {62, 1},
        {63, 2},
        {64, 4},
        {65, 2},
        {66, 2},
        {67, 1},
        {68, 1},
        {69, 1},
        {70, 2},
        {0, 2}};

    DataOut->put(1);
    if (!compressSignalDataWithLength(DataOut, carId, CcompletelyBuildedVehicleSigInfo, 0, 4))
        return false;

    // get mileages not done!!!!

    if (!compressSignalDataWithLength(DataOut, carId, CcompletelyBuildedVehicleSigInfo, 5, 7))
        return false;

    // 2 bytes reverse
    DataOut->movePosition(true, 2);
    return true;
}

bool MotorInfoUpload::genDriveMotorData(const uint64_t& carId, ByteBuffer* DataOut) {
    const SignalInfo SigInfo[] = {
        {71, 1},
        {72, 1},
        {73, 1},
        {74, 2},
        {75, 2},
        {76, 1},
        {77, 2},
        {78, 2}};

    DataOut->put(2);
    // put number of Drive Motors
    DataOut->put(1);
    return compressSignalDataWithLength(DataOut, carId, SigInfo, 0, sizeof (SigInfo));
}

bool MotorInfoUpload::genEngineData(const uint64_t& carId, ByteBuffer* DataOut) {
    const SignalInfo SigInfo[] = {
        {79, 1},
        {80, 2},
        {81, 2}};

    DataOut->put(4);
    return compressSignalDataWithLength(DataOut, carId, SigInfo, 0, sizeof (SigInfo));
}

bool MotorInfoUpload::genLocationData(const uint64_t& carId, ByteBuffer* DataOut) {
    DataOut->put(5);
    // not done
    return true;
}

bool MotorInfoUpload::genExtremeValueData(const uint64_t& carId, ByteBuffer* DataOut) {
    const SignalInfo SigInfo[] = {
        {46, 1},
        {47, 1},
        {48, 2},
        {49, 1},
        {50, 1},
        {51, 2},
        {52, 1},
        {53, 1},
        {54, 1},
        {55, 1},
        {56, 1},
        {57, 1}};

    DataOut->put(6);
    return compressSignalDataWithLength(DataOut, carId, SigInfo, 0, sizeof (SigInfo));
}

bool MotorInfoUpload::genAlarmData(const uint64_t& carId, ByteBuffer* DataOut) {
    const SignalInfo SigInfo[] = {
        {82, 1}};
    const byte signalCodeArray[] = {88, 89, 90, 91, 92, 93, 147, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105};

    DataOut->put(7);
    if (compressSignalDataWithLength(DataOut, carId, SigInfo, 0, sizeof (SigInfo)))
        return false;
    uint32_t generalAlarm;
    if (compressSignalData(generalAlarm, carId, signalCodeArray, 0, sizeof (signalCodeArray)))
        return false;

    DataOut->put((byte*) & generalAlarm, 0, 4);
    // not done...
}

bool MotorInfoUpload::compressSignalDataWithLength(ByteBuffer* DataOut, const uint64_t& carId, const SignalInfo signalInfoArray[], const int& offset, const int& size) {
    if (offset < 0 | size < 0 | size + offset > sizeof (signalInfoArray) | NULL == DataOut)
        return false;

    ResultSet* result;

    try {
        ushort sigVal;
#if USEPreparedStat
        m_prep_stmt->setUInt64(1, carId);
#else
        static char strCarId[11] = {0};
        static char strSignalTypeCode[4] = {0};
        snprintf(strCarId, 10, "%ld", carId);
        memcpy(sqlTemplate + caridOfst, caridPlaceHolder.data(), caridPlaceHolder.length());
        memcpy(sqlTemplate + caridOfst, strCarId, strlen(strCarId));
#endif

        for (int i = offset; i < size + offset; i++) {
#if USEPreparedStat
            m_prep_stmt->setUInt(2, signalInfoArray[i].signalTypeCode);
            result = m_prep_stmt->executeQuery();
#else            
            snprintf(strSignalTypeCode, 3, "%d", signalInfoArray[i].signalTypeCode);
            memcpy(sqlTemplate + signal_typeOfst, signal_typePlaceHolder.data(), signal_typePlaceHolder.length());
            memcpy(sqlTemplate + signal_typeOfst, strSignalTypeCode, strlen(strSignalTypeCode));

            result = m_state->executeQuery(sqlTemplate);
#endif
            if (result->next())
                sigVal = result->getUInt("signal_value");
            else
                sigVal = 0;

            if (DataOut->remaining() < signalInfoArray[i].length) {
                cout << "[ERROR] DataOut.array is too samll." << endl;
                delete result;
                return false;
            }
            DataOut->put((byte*) & sigVal, 0, signalInfoArray[i].length);

            delete result;
        }

        return true;
    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        delete result;
        closeDBConnection();
        return false;
    } catch (byte bytebufE) {
        cout << "ERROR: ByteBuffer Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "error code: " << (ushort) bytebufE << endl;
        closeDBConnection();
        delete result;
        return false;
    }
}

bool MotorInfoUpload::compressSignalData(uint32_t& DataOut, const uint64_t& carId, const byte signalCodeArray[], const int& offset, const int& size) {
    if (offset < 0 | size < 0 | size + offset > sizeof (signalCodeArray))
        return false;

    ResultSet* result;

    try {
        ushort sigVal;
#if USEPreparedStat
        m_prep_stmt->setUInt64(1, carId);
#else
        static char strCarId[11] = {0};
        static char strSignalTypeCode[4] = {0};
        snprintf(strCarId, 10, "%ld", carId);
        memcpy(sqlTemplate + caridOfst, caridPlaceHolder.data(), caridPlaceHolder.length());
        memcpy(sqlTemplate + caridOfst, strCarId, strlen(strCarId));
#endif

        for (int i = 0; i < size; i++) {
#if USEPreparedStat
            m_prep_stmt->setUInt(2, signalCodeArray[i + offset]);
            result = m_prep_stmt->executeQuery();
#else            
            snprintf(strSignalTypeCode, 3, "%d", signalInfoArray[i].signalTypeCode);
            memcpy(sqlTemplate + signal_typeOfst, signal_typePlaceHolder.data(), signal_typePlaceHolder.length());
            memcpy(sqlTemplate + signal_typeOfst, strSignalTypeCode, strlen(strSignalTypeCode));

            result = m_state->executeQuery(sqlTemplate);
#endif
            if (result->next())
                sigVal = result->getUInt("signal_value");
            else
                sigVal = 0;

            if (1 == sigVal)
                DataOut & (1 << (32 - i));

            delete result;
        }

        return true;
    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        delete result;
        closeDBConnection();
        return false;
    } catch (byte bytebufE) {
        cout << "ERROR: ByteBuffer Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "error code: " << (ushort) bytebufE << endl;
        closeDBConnection();
        delete result;
        return false;
    }
}

void MotorInfoUpload::clearAndFreeAllCarArray(void) {
    for (int i = 0; i < m_allCarArray.size(); i++)
        delete m_allCarArray[i];
    m_allCarArray.clear();
}