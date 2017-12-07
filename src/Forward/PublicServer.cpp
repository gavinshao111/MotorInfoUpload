/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PublicServer.cpp
 * Author: 10256
 * 
 * Created on 2017年6月29日, 下午9:08
 */

#include "PublicServer.h"
#include <stdexcept>
#include <boost/make_shared.hpp>

using namespace std;

PublicServer::PublicServer(const string& ip, const int& port) : m_ip(ip), m_port(port) {
}

PublicServer::PublicServer(const PublicServer& orig) {
}

PublicServer::~PublicServer() {
}

void PublicServer::close() {
    if (m_socket_sptr)
        m_socket_sptr->close();
}

void PublicServer::connect(/*const size_t& timeout = 0*/) {
    try {
        if (m_socket_sptr)
            m_socket_sptr->connect();
        else
            m_socket_sptr = boost::make_shared<gsocket::socket>(m_ip, m_port);
    } catch (gsocket::SocketConnectRefusedException& e) {
    }
}

void PublicServer::read(bytebuf::ByteBuffer& data, const size_t& size, const size_t& timeout/* = 0*/) {
    m_socket_sptr->read(data, size, timeout);
}

void PublicServer::write(bytebuf::ByteBuffer& src, const size_t& size) {
    m_socket_sptr->write(src, size);
}

void PublicServer::write(bytebuf::ByteBuffer& src) {
    m_socket_sptr->write(src);
}

bool PublicServer::is_connected() const {
    if (!m_socket_sptr)
        return false;
    return m_socket_sptr->isConnected();
}

