/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PublicServer.h
 * Author: 10256
 *
 * Created on 2017年6月29日, 下午9:08
 */

#ifndef PUBLICSERVER_H
#define PUBLICSERVER_H

#include "GSocket.h"
#include <string>
#include <boost/shared_ptr.hpp>

class PublicServer {
public:
    PublicServer();
    PublicServer(const PublicServer& orig);
    virtual ~PublicServer();

    void Close();
    void Connect(/*const size_t& timeout = 0*/);
    void Read(bytebuf::ByteBuffer& data, const size_t& size, const size_t& timeout = 0);

    void Write(bytebuf::ByteBuffer& src, const size_t& size);

    void Write(bytebuf::ByteBuffer& src);
    bool isConnected() const;
    
    void setConnectionOption(const std::string& ip, const int& port);
private:
    boost::shared_ptr<gsocket::GSocket> m_socketSPtr;
    std::string m_ip;
    int m_port;
    
//    gsocket::GSocket& m_socket;
};

#endif /* PUBLICSERVER_H */

