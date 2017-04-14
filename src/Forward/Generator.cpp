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
#include <boost/bind.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include "Generator.h"
#include "../Util.h"
#include "DataPacketForward.h"

using namespace std;
using namespace bytebuf;

Generator::Generator(StaticResourceForward& staticResource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection) :
s_staticResource(staticResource),
s_carDataQueue(CarDataQueue),
s_responseDataQueue(ResponseDataQueue),
s_tcpConn(tcpConnection),
m_MQClient(staticResource.MQServerUrl, staticResource.MQClientID, staticResource.MQServerUserName, staticResource.MQServerPassword) {
}

Generator::~Generator() {
}

void Generator::run() {
    try {
        m_MQClient.setSslOption(s_staticResource.pathOfServerPublicKey, s_staticResource.pathOfPrivateKey);
        m_MQClient.setMsgarrvdCallback(boost::bind(&Generator::msgArrvdHandler, this, _1, _2));
        m_MQClient.connect(true, 10);
        m_MQClient.subscribe(s_staticResource.MQTopicForUpload);
        cout << "[INIT] Generator is subscribing on " << s_staticResource.MQTopicForUpload
                << " and waiting for forward response\n" << endl;

        for (int i = 0;; i++) {
            BytebufSPtr_t response = s_responseDataQueue.take();
            string vin((char*) ((DataPacketHeader_t*) response->array())->vin, VINLEN);
            m_stream.str("");
            m_stream << '/' << vin << s_staticResource.MQTopicForResponse;
            m_MQClient.publish(m_stream.str(), *response);
            cout << "[INFO] Generator: response packet(" << response->remaining() << " bytes) to " << vin << " published\n" << endl;
        }


    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    cout << "[DONE] Generator quiting...\n" << endl;
}

void Generator::msgArrvdHandler(const string& topic, ByteBuffer& payload) {
    try {
        // MQ format: | dataLength(2) | data(dataLength) | ... |
        for (; payload.hasRemaining();) {
            uint16_t dataLength = payload.getShort();
            Util::BigLittleEndianTransfer(&dataLength, 2);
            if (dataLength < 0 || dataLength > payload.remaining()) {
                m_stream.str("");
                m_stream << "Generator::msgArrvdHandler(): Illegal dataLength: " << dataLength;
                throw runtime_error(m_stream.str());
            }
            BytebufSPtr_t carData = boost::make_shared<ByteBuffer>(dataLength);
            carData->put(payload, dataLength);
            carData->flip();
            if (!s_tcpConn.isConnected()) {
                ((DataPacketHeader_t*) carData->array())->cmdId = enumCmdCode::reissueUpload;
            }

            s_carDataQueue.put(carData);
#if DEBUG
            // topic is like /0123456789hellowo/lpcloud/candata/gbt32960/upload
            string vin = topic.substr(1, VINLEN);
            cout << "[INFO] Generator: receive vehicle signal data from " << vin << ", put into upload dataQueue\n" << endl;
#endif
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
}
