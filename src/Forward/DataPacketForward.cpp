/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ForwardDataPacket.cpp
 * Author: 10256
 * 
 * Created on 2017年3月9日, 下午3:18
 */

#include "DataPacketForward.h"
#include "../DataFormat.h"
#include "DataFormatForward.h"

using namespace bytebuf;
using namespace std;

DataPacketForward::DataPacketForward(const string& vin, const size_t& length)
: m_vin(vin),
m_dataBuf(length) {
}

//DataPacketForward::DataPacketForward(const DataPacketForward& orig) {
//}

DataPacketForward::~DataPacketForward() {
}

void DataPacketForward::setReissue() {
    if (m_dataBuf->capacity() < sizeof (DataPacketHeader_t))
        throw runtime_error("setReissue(): m_dataBuf->capacity() is too small");

    DataPacketHeader_t* header = (DataPacketHeader_t*) m_dataBuf->array();
    header->CmdId = enumCmdCode::reissueUpload;
}
// 输出日志需用到

time_t DataPacketForward::getCollectTime() const {
    if (m_dataBuf->capacity() < sizeof (DataPacketHeader_t) + sizeof (TimeForward_t))
        throw runtime_error("getCollectTime(): m_dataBuf->capacity() is too small");
    TimeForward_t* time = (TimeForward_t*) (m_dataBuf->array() + sizeof (DataPacketHeader_t));
    struct tm timeTM;
    timeTM.tm_year = time->year + 100;
    timeTM.tm_mon = time->mon - 1;
    timeTM.tm_mday = time->mday;
    timeTM.tm_hour = time->hour;
    timeTM.tm_min = time->min;
    timeTM.tm_sec = time->sec;

    return mktime(&timeTM);
}