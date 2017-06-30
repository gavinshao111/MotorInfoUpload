/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TcpSession.h
 * Author: 10256
 *
 * Created on 2017年5月16日, 下午5:12
 */

#ifndef TCPSESSION_H
#define TCPSESSION_H

#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <string>
#include "ByteBuffer.h"
#include "DataFormatForward.h"
#include "Logger.h"


class TcpServer;

class TcpSession : public boost::enable_shared_from_this<TcpSession> {
public:
    virtual ~TcpSession();

    boost::asio::ip::tcp::socket& socket();
    void readHeader();
    void write(const bytebuf::ByteBuffer& src);
    void write(const bytebuf::ByteBuffer& src, const size_t& offset, const size_t& length);
private:
    TcpSession(boost::asio::io_service&);
    TcpSession(const TcpSession&);
    void readDataUnit();
    void readHeaderHandler(const boost::system::error_code& error, size_t bytes_transferred);
    void readDataUnitHandler(const boost::system::error_code& error, size_t bytes_transferred);
    void writeHandler(const boost::system::error_code& error, size_t bytes_transferred);
    void parseDataUnit();
    void readTimeoutHandler(const boost::system::error_code& error);

    boost::asio::ip::tcp::socket m_socket;
    BytebufSPtr_t m_packetRef;
    bool m_quit;
    std::string m_vinStr;
    DataPacketHeader_t* m_hdr;
    uint16_t m_dataUnitLen;
    boost::asio::deadline_timer m_timer;
    size_t m_heartBeatCycle;
    std::stringstream m_stream;
    Logger& m_logger;

    friend class TcpServer;

};
typedef boost::shared_ptr<TcpSession> SessionRef_t;

#endif /* TCPSESSION_H */

