/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TcpServer.cpp
 * Author: 10256
 * 
 * Created on 2017年5月12日, 上午9:57
 */
#include "TcpServer.h"
#include <boost/bind.hpp>
#include "resource.h"
#include "TcpSession.h"
#include "logger.h"

using namespace boost::asio;

TcpServer::TcpServer(io_service& ioservice, const int& port) :
m_acceptor(ioservice, ip::tcp::endpoint(ip::tcp::v4(), port)) {
    startAccept();
}

TcpServer::~TcpServer() {
}

void TcpServer::acceptHandler(SessionRef_t session, const boost::system::error_code& error) {
    if (error) {
        GWARNING("TcpServer") << "acceptHandler error: " << error.message();
        m_acceptor.get_io_service().stop();
        return;
    }
//    GDEBUG("TcpServer") << "new connection";
    startAccept();
    session->readHeader();
}

void TcpServer::startAccept() {
    SessionRef_t session(new TcpSession(m_acceptor.get_io_service()));
    m_acceptor.async_accept(session->socket(),
            boost::bind(&TcpServer::acceptHandler, this, session, placeholders::error));
}