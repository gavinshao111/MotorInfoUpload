/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Generator.h
 * Author: 10256
 *
 * Created on 2017年3月9日, 下午4:24
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include <sstream>
#include "DataFormatForward.h"
#include "GMqttClient.h"

class Generator {
public:
    StaticResourceForward& s_staticResource;
    DataQueue_t& s_carDataQueue;
    DataQueue_t& s_responseDataQueue;
    TcpConn_t& s_tcpConn;
    gmqtt::GMqttClient m_MQClient;
    
    Generator(StaticResourceForward& staticResource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection);
    Generator(const Generator& orig);
    virtual ~Generator();
    
    void run();
private:
    void msgArrvdHandler(const std::string& topic, bytebuf::ByteBuffer& payload);
    void subscribeMq();
    std::stringstream m_stream;
};

#endif /* GENERATOR_H */

