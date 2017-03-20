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
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <sstream>

#include "Generator.h"
#include "../Util.h"
#include "DataPacketForward.h"

extern "C" {
#include "MQTTClient.h"
}

using namespace std;

extern bool exitNow;

extern mutex generator_mtx;
extern condition_variable connlostOrExitNow;

static stringstream Stream;

Generator::Generator(StaticResourceForward* staticResource) :
m_staticResource(staticResource) {
}

Generator::Generator(const Generator& orig) {
}

Generator::~Generator() {
}

void Generator::run() {
    if (NULL == m_staticResource->dataQueue) {
        cout << "DataQueue is NULL" << endl;
        return;
    }

    try {
        for (; !exitNow;)
            subscribeMq(); // block here, when MQ arrive, msgarrvd() will be called
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    // make Sender return from take(), then exit.
    m_staticResource->dataQueue->put(NULL);
    cout << "Generator quiting..." << endl;
}

static void connlost(void *context, char *cause) {
    printf("\nConnection lost, cause: %s\n", cause);
    unique_lock<mutex> lk(generator_mtx);
    connlostOrExitNow.notify_one();
}

/*
 * @param topicLen
 * always == 0, i don't why.
 */
static int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    try {
        cout << "MQ arrived: " << topicName << endl;
        if (NULL == context)
            throw runtime_error("msgarrvd(): IllegalArgument context");

        uint16_t dataLength;
        Generator* generator = (Generator*) context;
        const string& topic = generator->m_staticResource->MQTopic;
        // topicName is expected like "/1234567/lpcloud/candata"
        if (NULL == topicName)
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        topicLen = strlen(topicName);
        size_t ofstAfterVin;

        if ('/' != *topicName)
            throw runtime_error("msgarrvd(): IllegalArgument topic");
        for (ofstAfterVin = 1; ofstAfterVin < topicLen; ofstAfterVin++) {
            if ('/' == *(topicName + ofstAfterVin))
                break;
        }

        if ((topicLen - ofstAfterVin) != (topic.length() - 2))
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        if (0 != strncmp(topicName + ofstAfterVin, topic.c_str() + 2, topic.length() - 2))
            throw runtime_error("msgarrvd(): IllegalArgument topic");

        string vin(topicName + 1, ofstAfterVin - 1);
        if (NULL == message->payload || 1 > message->payloadlen)
            throw runtime_error("msgarrvd(): Illegal payload");

        // MQ format: | dataLength(2) | data(dataLength) | ... |
        dataLength = *(uint16_t*) message->payload;
        Util::BigLittleEndianTransfer(&dataLength, 2);
        if (dataLength < 0 || dataLength > message->payloadlen - 2) {
            Stream.clear();
            Stream << "msgarrvd(): Illegal dataLength: " << dataLength;
            throw runtime_error(Stream.str());
        }
        DataPacketForward * carData = new DataPacketForward(vin, dataLength);
        carData->m_dataBuf->put((uint8_t*)message->payload, 2, dataLength);
        carData->m_dataBuf->flip();
        if (!generator->m_staticResource->tcpConnection->isConnected())
            carData->setReissue();

        generator->m_staticResource->dataQueue->put(carData);
#if DEBUG
        cout << vin << " put into dataQueue" << endl;
#endif
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void Generator::subscribeMq() {

#define QOS         1
#define MQTTCLIENT_PERSISTENCE_NONE 1

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    MQTTClient_create(&client, m_staticResource->MQServerUrl.c_str(), m_staticResource->MQClientID.c_str(),
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = m_staticResource->MQServerUserName.c_str();
    conn_opts.password = m_staticResource->MQServerPassword.c_str();

    MQTTClient_setCallbacks(client, this, connlost, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        MQTTClient_destroy(&client);
            Stream.clear();
        Stream << "Failed to connect, return " << rc;
        throw runtime_error(Stream.str());
    }
    MQTTClient_subscribe(client, m_staticResource->MQTopic.c_str(), QOS);

    cout << "Generator is subscribing on MQ" << endl;
    unique_lock<mutex> lk(generator_mtx);
    connlostOrExitNow.wait(lk); // 当MQ连接断开时，被唤醒返回，外部再次调用该函数重新连接MQ

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}
