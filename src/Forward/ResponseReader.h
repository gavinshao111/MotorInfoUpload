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
#include "Logger.h"
#include "DataFormatForward.h"

namespace responsereaderstatus {

    typedef enum {
        init = 0,
        responseOk = 1,
        timeout = 2,
        responseNotOk = 3,
        connectionClosed = 4,
        responseFormatErr = 5,
        carLoginOk = 6,
    } EnumResponseReaderStatus;
}
class ResponseReader {
public:
    ResponseReader(const size_t& no, PublicServer& publicServer);
//    ResponseReader(const ResponseReader& orig);
    virtual ~ResponseReader();
    
    void task();
    const responsereaderstatus::EnumResponseReaderStatus& status() const {
        return m_responseStatus;
    }
    uint8_t responseFlag() const {
        return m_packetHdr->responseFlag;
    }
    
private:
    PublicServer& r_publicServer;
    bytebuf::ByteBuffer m_responseBuf;
    std::stringstream m_stream;
    std::string m_vin;
    DataPacketHeader_t* m_packetHdr;
    Logger& r_logger;
    responsereaderstatus::EnumResponseReaderStatus m_responseStatus;
    std::string m_detail;
    std::string m_id;
    
    void readResponse(const size_t& timeout);
    
};

#endif /* RESPONSEREADER_H */

