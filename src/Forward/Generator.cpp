/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Generator.cpp
 * Author: 10256
 * 
 * Created on 2017年3月9日, 下午4:24
 */

#include <iostream>
#include <exception>
#include <sstream>
#include <boost/smart_ptr/make_shared_object.hpp>

#include "Generator.h"
#include "../Util.h"
#include "DataPacketForward.h"

using namespace std;
using namespace bytebuf;
//extern bool exitNow;

//extern mutex generator_mtx;
//extern condition_variable connlostOrExitNow;

Generator::Generator(StaticResourceForward& staticResource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection) :
s_staticResource(staticResource),
s_carDataQueue(CarDataQueue),
s_responseDataQueue(ResponseDataQueue),
s_tcpConn(tcpConnection),
m_MQClient(staticResource.MQServerUrl, staticResource.MQClientID, staticResource.MQServerUserName, staticResource.MQServerPassword) {
}

Generator::Generator(const Generator& orig) {
}

Generator::~Generator() {
}

void Generator::run() {
    try {
        m_MQClient.setSslOption(s_staticResource.pathOfServerPublicKey, s_staticResource.pathOfPrivateKey);
        m_MQClient.setMsgarrvdCallback(boost::bind(&Generator::msgArrvdHandler, this, _1, _2));
        m_MQClient.connect(true, 10);
        m_MQClient.subscribe(s_staticResource.MQTopicForCarData);
        
        // to do: get response packet, get vin from it, then publish to MQ
        s_responseDataQueue.take();

    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    // make Sender return from take(), then exit.
    s_carDataQueue->put(NULL);
    cout << "Generator quiting..." << endl;
}

/*
 * @param topicLen
 * always == 0, i don't why.
 */
void Generator::msgArrvdHandler(const string& topic, ByteBuffer& payload) {
    try {
        // MQ format: | dataLength(2) | data(dataLength) | ... |
        for (; payload.hasRemaining();) {
            uint16_t dataLength = payload.getShort();
            Util::BigLittleEndianTransfer(&dataLength, 2);
            if (dataLength < 0 || dataLength > payload.remaining()) {
                m_stream.clear();
                m_stream << "Generator::msgArrvdHandler(): Illegal dataLength: " << dataLength;
                throw runtime_error(m_stream.str());
            }
            boost::shared_ptr<ByteBuffer> carData = boost::make_shared<ByteBuffer>(dataLength);
            carData->put(payload, 0, dataLength);
            carData->flip();
            if (!s_tcpConn->isConnected()) {
                DataPacketHeader_t* hdr = (DataPacketHeader_t*)carData->array();
                hdr->CmdId = enumCmdCode::reissueUpload;
            }

            s_carDataQueue->put(carData);
#if DEBUG
        // topic is like /0123456789hellowo/lpcloud/candata/gbt32960/upload
            string vin = topic;
            vin.substr(1, VINLEN);
            cout<< "Generator::msgArrvdHandler(): "  << vin << " put into dataQueue" << endl;
#endif
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
}

//void Generator::subscribeMq() {
//
//#define QOS         1
//#define MQTTCLIENT_PERSISTENCE_NONE 1
//
//    MQTTClient client;
//    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
//    int rc;
//    MQTTClient_create(&client, s_staticResource->MQServerUrl.c_str(), s_staticResource->MQClientID.c_str(),
//            MQTTCLIENT_PERSISTENCE_NONE, NULL);
//    conn_opts.keepAliveInterval = 20;
//    conn_opts.cleansession = 1;
//    conn_opts.username = s_staticResource->MQServerUserName.c_str();
//    conn_opts.password = s_staticResource->MQServerPassword.c_str();
//
//    MQTTClient_setCallbacks(client, this, connlost, msgarrvd, NULL);
//
//    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
//        MQTTClient_destroy(&client);
//        m_stream.clear();
//        m_stream << "Failed to connect, return " << rc;
//        throw runtime_error(m_stream.str());
//    }
//    MQTTClient_subscribe(client, s_staticResource->MQTopicForCarData.c_str(), QOS);
//
//    cout << "Generator is subscribing on MQ" << endl;
//    unique_lock<mutex> lk(generator_mtx);
//    connlostOrExitNow.wait(lk); // 当MQ连接断开时，被唤醒返回，外部再次调用该函数重新连接MQ
//
//    MQTTClient_disconnect(client, 10000);
//    MQTTClient_destroy(&client);
//}
