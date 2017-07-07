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

#include "socket.h"
#include <string>
#include <boost/shared_ptr.hpp>

class PublicServer {
public:
    PublicServer(const std::string& ip, const int& port);
    virtual ~PublicServer();

    void close();
    void connect(/*const size_t& timeout = 0*/);
    void read(bytebuf::ByteBuffer& data, const size_t& size, const size_t& timeout = 0);

    void write(bytebuf::ByteBuffer& src, const size_t& size);

    void write(bytebuf::ByteBuffer& src);
    bool isConnected() const;
    
private:
    PublicServer(const PublicServer& orig);
    boost::shared_ptr<gsocket::socket> m_socketSPtr;
    std::string m_ip;
    int m_port;
};

#endif /* PUBLICSERVER_H */

