/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ResponseReader.cpp
 * Author: 10256
 * 
 * Created on 2017年7月1日, 下午6:23
 */

#include "ResponseReader.h"
#include <unistd.h>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <boost/chrono.hpp>
#include "resource.h"
#include "Constant.h"
#include "../Util.h"
#include "logger.h"

using namespace bytebuf;
using namespace std;
using namespace gsocket;

ResponseReader::ResponseReader(const size_t& no, PublicServer& publicServer) :
m_responseBuf(512),
r_publicServer(publicServer),
m_vin(Constant::vinInital),
m_responseStatus(responsereaderstatus::EnumResponseReaderStatus::init) {
    m_id = "ResponseReader." + to_string(no);
}

//ResponseReader::ResponseReader(const ResponseReader& orig) {
//}

ResponseReader::~ResponseReader() {
}

void ResponseReader::task() {
    try {
        int timeout = 0; //block to read
        for (;;) {
            if (!r_publicServer.isConnected()) {
                boost::unique_lock<boost::mutex> lk(m_statusMtx);
                m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::init;
                try {
                    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
                } catch (boost::thread_interrupted&) {
                    break;
                }
                continue;
            }
            readResponse(timeout);
            switch (m_responseStatus) {
                case responsereaderstatus::EnumResponseReaderStatus::connectionClosed:
                    GWARNING(m_id) << "connectionClosed";
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::timeout:
                    GWARNING(m_id) << "read timeout";
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::responseOk:
                    if (m_responsePacketType == enumCmdCode::vehicleLogin) {
                        resource::SessionTable_t::iterator iter =
                                resource::getResource()->getVechicleSessionTable().find(m_vin);
                        if (iter == resource::getResource()->getVechicleSessionTable().end())
                            GWARNING(m_vin) << "vin not existing in vechicle session table when try to forward response";
                        else {
                            iter->second->write(m_responseBuf);
                            GINFO(m_vin) << "response ok forwarded";
                        }
                    }
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::responseNotOk: {
                    GINFO(m_vin) << "response not ok, response flag: "
                            << (int) m_packetHdr->responseFlag;
                    resource::SessionTable_t::iterator iter =
                            resource::getResource()->getVechicleSessionTable().find(m_vin);
                    if (iter == resource::getResource()->getVechicleSessionTable().end())
                        GWARNING(m_vin) << "vin not existing in vechicle session table when try to forward response";
                    else {
                        iter->second->write(m_responseBuf);
                        GINFO(m_vin) << "response not ok forwarded";
                    }
                    break;
                }
                case responsereaderstatus::EnumResponseReaderStatus::responseFormatErr:
                {
                    GWARNING(m_vin) << "bad response format: " << m_stream.str();
                    break;
                }
                default:
                    throw runtime_error("ResponseReader::forwardCarData(): unrecognize SenderStatus code: "
                            + to_string((int) m_responseStatus));
            }
        }
    } catch (exception &e) {
        GWARNING("ResponseReader exception") << e.what();
    }
    GINFO(m_id) << "quiting...";
}

/*
 * 国标：当服务端发送应答时，变更应答标志，保留报文时间，并重新计算校验位，删除其他数据。
 * 理解为回应报文结构，头部变更应答标志，其余不变，数据单元只保留数据采集时间，其余删除，重新计算校验位
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输。
 * 企业平台转发上级平台的回应报文：车辆登入成功，所有的失败报文。
 * read server response
 */
void ResponseReader::readResponse(const size_t& timeout) {
    TimeForward_t* responseTime;

    m_responseBuf.clear();
    uint16_t responseDataUnitLen;
    DataPacketHeader_t* packetHdrTmp = (DataPacketHeader_t*) m_responseBuf.array();
    m_stream.str("");
    try {
        r_publicServer.read(m_responseBuf, 2, timeout);
        if (packetHdrTmp->startCode[0] != '#' || packetHdrTmp->startCode[1] != '#') {
            status(responsereaderstatus::EnumResponseReaderStatus::responseFormatErr);
            m_stream << "start code expect to be 35, 35(##), actually is "
                    << (int) packetHdrTmp->startCode[0] << ", " << (int) packetHdrTmp->startCode[1];
            return;
        }

        r_publicServer.read(m_responseBuf, sizeof (DataPacketHeader_t) - 2, timeout);
        m_responsePacketType = (enumCmdCode) packetHdrTmp->cmdId;
        m_vin.assign((char*) packetHdrTmp->vin, sizeof (packetHdrTmp->vin));
        responseDataUnitLen = boost::asio::detail::socket_ops::network_to_host_short(
                packetHdrTmp->dataUnitLength);

        if (responseDataUnitLen > m_responseBuf.remaining()) {
            m_stream << "Illegal responseHdr->responseDataUnitLength: " << responseDataUnitLen;
            status(responsereaderstatus::EnumResponseReaderStatus::responseFormatErr);
            return;
        }
        r_publicServer.read(m_responseBuf, responseDataUnitLen + 1, timeout);
    } catch (SocketTimeoutException& e) {
        status(responsereaderstatus::EnumResponseReaderStatus::timeout);
        return;
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        GWARNING(m_id) << "read response exception: " << e.what();
        status(responsereaderstatus::EnumResponseReaderStatus::connectionClosed);
        return;
    }
    m_responseBuf.flip();

    uint8_t chechCode = Util::generateBlockCheckCharacter(m_responseBuf, 2, m_responseBuf.remaining() - 3);
    if (chechCode != m_responseBuf.get(m_responseBuf.limit() - 1)) {
        m_stream << "[" << m_vin << "] "
                << "BCC in public server's response check fail, expected to be " << chechCode
                << ", actually is " << m_responseBuf.get(m_responseBuf.limit() - 1);
        status(responsereaderstatus::EnumResponseReaderStatus::responseFormatErr);
        return;
    }
    m_packetHdr = packetHdrTmp;
    responseTime = (TimeForward_t*) (m_responseBuf.array() + sizeof (DataPacketHeader_t));

    if (packetHdrTmp->responseFlag == responseflag::enumResponseFlag::success) {
        status(responsereaderstatus::EnumResponseReaderStatus::responseOk);
    } else {
        status(responsereaderstatus::EnumResponseReaderStatus::responseNotOk);
    }
    m_responseBuf.rewind();
}

const responsereaderstatus::EnumResponseReaderStatus& ResponseReader::waitNextStatus(const size_t& second) {
    boost::unique_lock<boost::mutex> lk(m_statusMtx);
    if (m_newStatus.wait_for(lk, boost::chrono::seconds(second)) == boost::cv_status::timeout)
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::timeout;
    return m_responseStatus;
}

void ResponseReader::status(const responsereaderstatus::EnumResponseReaderStatus& status) {
    boost::unique_lock<boost::mutex> lk(m_statusMtx);
    m_responseStatus = status;
    m_newStatus.notify_all();
}
