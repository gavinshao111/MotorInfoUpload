/*
g++ MQTTClient_subscribe.cpp -I$ED/paho.mqtt.c/src -L$ED/paho.mqtt.c/build/output -lpaho-mqtt3c -DNO_PERSISTENCE=1 -o sub
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <exception>
#include <stdexcept>

using namespace std;

#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define MQTTCLIENT_PERSISTENCE_NONE 1
volatile MQTTClient_deliveryToken deliveredtoken;
void printBinaryNumber(const int8_t& src);
void BigToLittleEndian(uint8_t src[], const size_t& size);
void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i = 0;
    int8_t* ptr;
    uint32_t sigTypeCode = 0;
    int8_t sigValLen = 0;
    size_t length = 0;
    uint8_t signalVal[4] = {0};

    printf("Message arrived, topic: %s\n", topicName);

    ptr = (int8_t*) message->payload;
    length = message->payloadlen;

    if (i + 1 > length)
        throw runtime_error("updateStructByMQ(): MQ payload too small");
    sigValLen = *(ptr + i);

    if (0 == sigValLen)
        return 1;
    else if (sigValLen > 4 || sigValLen < 0) {
        char errMsg[256] = "CarData::updateStructByMQ(): Illegal sigValLen: ";
        snprintf(errMsg + strlen(errMsg), 4, "%d", sigValLen);
        throw runtime_error(errMsg);
    }
    i++;

    if (i + 4 > length)
        throw runtime_error("updateStructByMQ(): MQ payload too small");

    sigTypeCode = *(uint32_t*)(ptr + i);
    BigToLittleEndian((uint8_t*)&sigTypeCode, 4);
    i += 4;

    if (i + sigValLen > length)
        throw runtime_error("updateStructByMQ(): MQ payload too small");


    uint32_t  val = *(uint32_t*)(ptr + i);
    BigToLittleEndian((uint8_t*)&val, sigValLen);
//    printf("%d\n", val);
    
    i += sigValLen;

    cout << "sigValLen: " << (int) sigValLen << ", sigTypeCode: " << sigTypeCode << ", signalVal: " << val << endl;
    putchar('\n');
    //    for (i = 0; i < message->payloadlen; i++) {
    //        putchar(*payloadptr++);
    //    }
    //    putchar('\n');
    //    putchar('\n');

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {

    string MQServerUrl = "tcp://120.26.86.124:1883";
    string topic = "/+/lpcloud/candata";


    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, MQServerUrl.c_str(), CLIENTID,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = "easydarwin";
    conn_opts.password = "123456";

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
            "Press Q<Enter> to quit\n\n", topic.c_str(), CLIENTID, QOS);
    MQTTClient_subscribe(client, topic.c_str(), QOS);

    do {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}
void printBinaryNumber(const int8_t& src) {
    for (int i = 0; i < 8; i++) {
        printf("%d ", src >> (7 - i) & 1);
    }
    putchar('\n');
}
void BigToLittleEndian(uint8_t src[], const size_t& size) {
    if (NULL == src || 0 == size)
        return;

    uint8_t* copy = new uint8_t[size] ;
    memcpy(copy, src, size);
    
    int j = 0;
    for (; j < size; j++) {
        src[j] = copy[size - j - 1];
    }
    delete copy;
}