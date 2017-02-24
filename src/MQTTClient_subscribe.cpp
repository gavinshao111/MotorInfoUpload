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

using namespace std;

#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define MQTTCLIENT_PERSISTENCE_NONE 1
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i = 0;
    uint8_t* payloadptr;

    //    printf("Message arrived\n");
    //    printf("     topic: %s\n", topicName);
    //    printf("   message: ");
    //
    payloadptr = (uint8_t*) message->payload;
    uint32_t sigTypeCode = 0;
    //    uint16_t v = *(uint16_t*)payloadptr;
    //    cout << "v: " << v << endl;
    assert(message->payloadlen == 4);

    sigTypeCode += *(payloadptr) << 24;
    sigTypeCode += *(payloadptr + 1) << 16;
    sigTypeCode += *(payloadptr + 2) << 8;
    sigTypeCode += *(payloadptr + 3);
    printf("%d\n", sigTypeCode);

    sigTypeCode = 0;
    sigTypeCode += *(payloadptr) * 0x1000000;
    sigTypeCode += *(payloadptr + 1) * 0x10000;
    sigTypeCode += *(payloadptr + 2) * 0x100;
    sigTypeCode += *(payloadptr + 3);
    printf("%d\n", sigTypeCode);
    
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
    string topic = "/1234/videoinfoAsk";


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
