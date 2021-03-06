/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ResponseReader.h
 * Author: 10256
 *
 * Created on 2017年7月1日, 下午6:23
 */

#ifndef RESPONSEREADER_H
#define RESPONSEREADER_H

#include <sstream>
#include "PublicServer.h"
#include "ByteBuffer.h"
#include "DataFormatForward.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread.hpp>
#include "thread.h"

namespace responsereaderstatus {

    typedef enum {
        init = 0,
        responseOk = 1,
        timeout = 2,
        responseNotOk = 3,
        connectionClosed = 4,
        responseFormatErr = 5,
    } EnumResponseReaderStatus;
}
class ResponseReader : public gavin::thread {
public:
    ResponseReader(const size_t& no, PublicServer& publicServer);
    virtual ~ResponseReader();
    
    const responsereaderstatus::EnumResponseReaderStatus& status() const {
        return m_responseStatus;
    }

    const responsereaderstatus::EnumResponseReaderStatus& waitNextStatus(const size_t& seconds);

    uint8_t responseFlag() const {
        return m_packetHdr->responseFlag;
    }
    
    const enumCmdCode& responsePacketType() const {
        return m_responsePacketType;
    }
    
private:
    ResponseReader(const ResponseReader& orig);
    PublicServer& r_publicServer;
    bytebuf::ByteBuffer m_responseBuf;
    std::stringstream m_stream;
    std::string m_vin;
    DataPacketHeader_t* m_packetHdr;
    responsereaderstatus::EnumResponseReaderStatus m_responseStatus;
    enumCmdCode m_responsePacketType;
    std::string m_detail;
    std::string m_id;
    boost::mutex m_statusMtx;
    boost::condition_variable m_newStatus;
    
    void run() override;
    void readResponse(const size_t& timeout);
    void status(const responsereaderstatus::EnumResponseReaderStatus& status);

};

#endif /* RESPONSEREADER_H */

