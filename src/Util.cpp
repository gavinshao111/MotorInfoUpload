/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Util.cpp
 * Author: 10256
 * 
 * Created on 2017年2月27日, 下午5:22
 */

#include "Util.h"
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <iostream>
#include "DataFormat.h"

using namespace std;

string Util::s_nowtimeStr = "";

Util::Util() {
}

Util::Util(const Util& orig) {
}

Util::~Util() {
}

void Util::BigLittleEndianTransfer(void* src, const size_t& size) {
    if (NULL == src || 2 > size)
        return;

    uint8_t* src2 = (uint8_t*) src;
    uint8_t* copy = new uint8_t[size];
    memcpy(copy, src2, size);

    int j = 0;
    for (; j < size; j++) {
        src2[j] = copy[size - j - 1];
    }
    delete copy;
}

void Util::printBinary(const uint8_t& src) {
    for (int i = 0; i < 8; i++) {
        printf("%d ", src >> (7 - i) & 1);
    }
    putchar('\n');
}

int Util::setupConnectionToTCPServer(const string& ip, const int& port, const bool& nonblock/* = false*/) {
    int fd = -1;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    inet_aton(ip.c_str(), (struct in_addr*) &servaddr.sin_addr.s_addr);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        throw runtime_error("Util::setupConnectionToTCPServer(): socket return -1");
    }

    if (nonblock) {
        int flags = fcntl(fd, F_GETFL, 0); //获取建立的sockfd的当前状态（非阻塞）
        fcntl(fd, F_SETFL, flags | O_NONBLOCK); //将当前sockfd设置为非阻塞    
    }

    if (connect(fd, (struct sockaddr*) &servaddr, sizeof (servaddr)) == -1) {
        throw runtime_error("Util::setupConnectionToTCPServer(): connect return -1");
    }
    return fd;
}

string Util::timeToStr(const time_t& time) {
    struct tm* timeTM;
    char strTime[22] = {0};

    timeTM = localtime(&time);
    strftime(strTime, sizeof (strTime) - 1, TIMEFORMAT, timeTM);
    string timeStr(strTime);
    return timeStr;
}

string Util::nowTimeStr() {
    return timeToStr(time(NULL));
}

void Util::sendByTcp(const int& fd, const void* ptr, const size_t& size) {
    size_t sentNum = 0;
    uint8_t* _ptr = (uint8_t*) ptr;
    size_t tmp;
    for (; sentNum < size;) {
        tmp = write(fd, _ptr + sentNum, size - sentNum);
        if (tmp == -1) {
            close(fd);
            char errMsg[256] = "";
            snprintf(errMsg, sizeof (errMsg) - 1, "DataSender::tcpSendData(): write to TCP server return -1. %d bytes sent", sentNum);
            throw runtime_error(errMsg);
        }
        sentNum += tmp;
    }
}

uint8_t Util::generateBlockCheckCharacter(const void* ptr, const size_t& size) {
    if (NULL == ptr || 1 > size)
        throw runtime_error("Util::generateBlockCheckCharacter(): Illegal source");

    uint8_t checkCode = 0;
    uint8_t* _ptr = (uint8_t*) ptr;
    for (size_t i = 0; i < size; i++)
        checkCode ^= _ptr[i];
    return checkCode;
}

uint8_t Util::generateBlockCheckCharacter(const bytebuf::ByteBuffer& byteBuffer, const size_t& offset, const size_t& size) {
    if (byteBuffer.remaining() < offset + size)
        throw runtime_error("Util::generateBlockCheckCharacter(): Illegal Argument");
    return generateBlockCheckCharacter(byteBuffer.array() + byteBuffer.position() + offset, size);
}

void Util::output(const string& id, const string& message) {
    output(id, message, "");
}
void Util::output(const string& id, const string& message1, const string& message2) {
    output(id, message1, message2, "");
}
void Util::output(const string& id, const string& message1, const string& message2, const string& message3) {
    output(id, message1, message2, message3, "");
}
void Util::output(const string& id, const string& message1, const string& message2, const string& message3, const string& message4) {
    output(id, message1, message2, message3, message4, "");
}
void Util::output(const string& id, const string& message1, const string& message2, const string& message3, const string& message4, const string& message5) {
    cout << "[" << id << "] " << message1 << message2 << message3 << message4 << message5 << " " << nowtimeStr() << endl;
}

const string& Util::nowtimeStr() {
    time_t now = time(NULL);
    s_nowtimeStr.assign(asctime(localtime(&now)));
    return s_nowtimeStr;
}