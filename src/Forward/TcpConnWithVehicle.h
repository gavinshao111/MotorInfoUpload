/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TcpConnWithVehicle.h
 * Author: 10256
 *
 * Created on 2017年5月16日, 下午5:12
 */

#ifndef TCPCONNWITHVEHICLE_H
#define TCPCONNWITHVEHICLE_H

#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <string>
#include "ByteBuffer.h"
#include "DataFormatForward.h"


class TcpServer;

class TcpConnWithVehicle : public boost::enable_shared_from_this<TcpConnWithVehicle> {
public:
    virtual ~TcpConnWithVehicle();

    boost::asio::ip::tcp::socket& socket();
    void readHeader();
    void write(const bytebuf::ByteBuffer& src);
    void write(const bytebuf::ByteBuffer& src, const size_t& offset, const size_t& length);
private:
    TcpConnWithVehicle(boost::asio::io_service&);
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

    friend class TcpServer;

};
typedef boost::shared_ptr<TcpConnWithVehicle> SessionRef_t;

#endif /* TCPCONNWITHVEHICLE_H */

