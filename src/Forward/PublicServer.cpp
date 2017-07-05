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

void PublicServer::Close() {
    if (m_socketSPtr.use_count() == 0)
        return;
    m_socketSPtr->Close();
}

void PublicServer::Connect(/*const size_t& timeout = 0*/) {
    try {
        if (m_socketSPtr.use_count() == 0) {
            m_socketSPtr = boost::make_shared<gsocket::GSocket>(m_ip, m_port);
        } else {
            m_socketSPtr->Connect();
        }
    } catch (gsocket::SocketConnectRefusedException& e) {
    }
}

void PublicServer::Read(bytebuf::ByteBuffer& data, const size_t& size, const size_t& timeout/* = 0*/) {
    m_socketSPtr->Read(data, size, timeout);
}

void PublicServer::Write(bytebuf::ByteBuffer& src, const size_t& size) {
    m_socketSPtr->Write(src, size);
}

void PublicServer::Write(bytebuf::ByteBuffer& src) {
    m_socketSPtr->Write(src);
}

bool PublicServer::isConnected() const {
    if (m_socketSPtr.use_count() == 0)
        return false;
    return m_socketSPtr->isConnected();
}

