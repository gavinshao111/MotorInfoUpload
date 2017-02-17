/*
g++ MQTTClient_subscribe.cpp -I$ED/paho.mqtt.c/src -L$ED/paho.mqtt.c/build/output -lpaho-mqtt3c -DNO_PERSISTENCE=1 -o sub
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <iostream>

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
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char *) message->payload;
    for (i = 0; i < message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

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
    string topic = "/+/videoinfoAsk";


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
