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
#include "Resource.h"
#include "Constant.h"
#include "../Util.h"

using namespace bytebuf;
using namespace std;
using namespace gsocket;

ResponseReader::ResponseReader(const size_t& no, PublicServer& publicServer) :
m_responseBuf(512),
r_publicServer(publicServer),
m_vin(Constant::VinInital),
m_responseStatus(responsereaderstatus::EnumResponseReaderStatus::init),
r_logger(Resource::GetResource()->GetLogger()) {
    m_stream << "ResponseReader." << no;
    m_id = m_stream.str();
}

//ResponseReader::ResponseReader(const ResponseReader& orig) {
//}

ResponseReader::~ResponseReader() {
}

void ResponseReader::task() {
    try {
        int timeout = Resource::GetResource()->GetReadResponseTimeOut();
        for (;;) {
            if (!r_publicServer.isConnected()) {
                m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::init;
                usleep(500 * 1000);
                continue;
            }
            readResponse(timeout);
            switch (m_responseStatus) {
                case responsereaderstatus::EnumResponseReaderStatus::connectionClosed:
                    r_logger.warn(m_id, "connectionClosed");
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::timeout:
                    r_logger.warn(m_id, "read timeout");
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::responseOk:
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::responseNotOk:
                {
                    r_logger.info(m_vin);
                    r_logger.infoStream << "Sender forward car data response not ok, response flag: "
                            << (int) m_packetHdr->responseFlag << std::endl;
                }
                case responsereaderstatus::EnumResponseReaderStatus::carLoginOk:
                {
                    Resource::ConnTable_t::iterator iter = Resource::GetResource()->GetVechicleConnTable().find(m_vin);
                    if (iter == Resource::GetResource()->GetVechicleConnTable().end())
                        r_logger.warn(m_vin, "vin not existing in VechicleConnTable when ResponseReader try to forward response");
                    else
                        iter->second->write(m_responseBuf);
                    break;
                }
                case responsereaderstatus::EnumResponseReaderStatus::responseFormatErr:
                    r_logger.warn(m_vin, "bad response format: ");
                    r_logger.warnStream << m_stream.str() << std::endl;
                default:
                    m_stream.str("");
                    m_stream << "ResponseReader::forwardCarData(): unrecognize SenderStatus code: " << m_responseStatus;
                    throw runtime_error(m_stream.str());
            }
        }
    } catch (exception &e) {
        r_logger.error("ResponseReader exception");
        r_logger.errorStream << e.what() << std::endl;
    }
    r_logger.info("DONE", "ResponseReader quiting...");
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
        r_publicServer.Read(m_responseBuf, 2, timeout * 1000);
        if (packetHdrTmp->startCode[0] != '#' || packetHdrTmp->startCode[1] != '#') {
            m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseFormatErr;
            m_stream << "start code expect to be 35, 35(##), actually is "
                    << (int) packetHdrTmp->startCode[0] << (int) packetHdrTmp->startCode[1];
            return;
        }

        r_publicServer.Read(m_responseBuf, sizeof (DataPacketHeader_t) - 2, timeout * 1000);
        m_vin.assign((char*) packetHdrTmp->vin, sizeof (packetHdrTmp->vin));
        responseDataUnitLen = boost::asio::detail::socket_ops::network_to_host_short(
                packetHdrTmp->dataUnitLength);

        if (responseDataUnitLen > m_responseBuf.remaining()) {
            m_stream << "Illegal responseHdr->responseDataUnitLength: " << responseDataUnitLen;
            m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseFormatErr;
            return;
        }
        r_publicServer.Read(m_responseBuf, responseDataUnitLen + 1, timeout * 1000000);
    } catch (SocketTimeoutException& e) {
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::timeout;
        return;
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        r_logger.warn("ResponseReader::readResponse");
        r_logger.warnStream << e.what() << std::endl;
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::connectionClosed;
        return;
    }
    m_responseBuf.flip();

    uint8_t chechCode = Util::generateBlockCheckCharacter(m_responseBuf, 2, m_responseBuf.remaining() - 3);
    if (chechCode != m_responseBuf.get(m_responseBuf.limit() - 1)) {
        m_stream << "[" << m_vin << "] "
                << "BCC in public server's response check fail, expected to be " << chechCode
                << ", actually is " << m_responseBuf.get(m_responseBuf.limit() - 1);
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseFormatErr;
        return;
    }
    if (responseDataUnitLen != sizeof (TimeForward_t)) {
        m_stream << "[" << m_vin << "] "
                << "responseDataUnitLength in public server's response expected to 6, actually is "
                << responseDataUnitLen;
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseFormatErr;
        return;
    }
    m_packetHdr = packetHdrTmp;
    responseTime = (TimeForward_t*) (m_responseBuf.array() + sizeof (DataPacketHeader_t));

    //    m_stream.str("");
    //    m_stream << "response time: " << (int) responseTime->year << '-' << (int) responseTime->mon << '-' << (int) responseTime->mday << " "
    //            << (int) responseTime->hour << ':' << (int) responseTime->min << ':' << (int) responseTime->sec;
    //    Util::output(m_vin, m_stream.str());
    //    m_stream.str("");
    //    m_stream << "response encryptionAlgorithm: " << (int) packetHdrTmp->encryptionAlgorithm;
    //    Util::output(m_vin, m_stream.str());

    if (packetHdrTmp->responseFlag == responseflag::enumResponseFlag::success)
        if (packetHdrTmp->cmdId == enumCmdCode::vehicleLogin)
            m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::carLoginOk;
        else
            m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseOk;
    else
        m_responseStatus = responsereaderstatus::EnumResponseReaderStatus::responseNotOk;
    m_responseBuf.rewind();
}

