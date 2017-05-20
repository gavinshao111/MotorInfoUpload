/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TcpServer.h
 * Author: 10256
 *
 * Created on 2017年5月12日, 上午9:57
 */

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "TcpConnWithVehicle.h"

class TcpServer {
public:
    TcpServer(boost::asio::io_service& ioservice, const int& port);
    virtual ~TcpServer();

private:
    void startAccept();
    void acceptHandler(SessionRef_t session, const boost::system::error_code& error);

    boost::asio::ip::tcp::acceptor m_acceptor;

};

#endif /* TCPSERVER_H */

