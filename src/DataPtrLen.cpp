/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataPtrLen.cpp
 * Author: 10256
 * 
 * Created on 2017年2月28日, 下午7:11
 */

#include "DataPtrLen.h"
#include <stdio.h>
#include <stdexcept>
#include <time.h>
#include "DataFormat.h"
#include "CarData.h"
#include <string.h>

using namespace std;
using namespace bytebuf;

//DataPtrLen::DataPtrLen() {
//}

DataPtrLen::DataPtrLen(const string& vin, const size_t& length, const bool& isReissue)
: m_isReissue(isReissue),
m_vin(vin) {
    m_dataBuf = ByteBuffer::allocate(length);
}

//DataPtrLen::DataPtrLen(uint8_t* ptr, const size_t& length) : m_ptr(ptr), m_dataBuf.capacity()(length) {
//}

DataPtrLen::DataPtrLen(const DataPtrLen& orig) {
}

DataPtrLen::~DataPtrLen() {
    m_dataBuf->freeMemery();
}

//string DataPtrLen::getVin() const {
//    if (offsetof(CarSignalData, encryptionAlgorithm) > m_dataBuf.capacity()) {
//        char errMsg[256] = "Illegal capacity: ";
//        snprintf(errMsg + strlen(errMsg), 1, "%d", m_dataBuf.capacity());
//        throw runtime_error(errMsg);
//    }
//    string vin((char*) (m_dataArray + offsetof(CarSignalData, vin)), VINLEN);
//    return vin;
//}

time_t DataPtrLen::getCollectTime() const {
    if (offsetof(CarSignalData, CBV_typeCode) > m_dataBuf->capacity()) {
        char errMsg[256] = "Illegal capacity: ";
        snprintf(errMsg + strlen(errMsg), 1, "%d", m_dataBuf->capacity());
        throw runtime_error(errMsg);
    }

    char strtime[50] = {0};
    uint8_t* array = m_dataBuf->array();
    snprintf(strtime, 49, TIMEFORMAT_,
            array[offsetof(CarSignalData, year)] + 2000,
            array[offsetof(CarSignalData, mon)],
            array[offsetof(CarSignalData, mday)],
            array[offsetof(CarSignalData, hour)],
            array[offsetof(CarSignalData, min)],
            array[offsetof(CarSignalData, sec)]);

    struct tm tmTemp;
    strptime(strtime, TIMEFORMAT, &tmTemp);
    return mktime(&tmTemp);
}
