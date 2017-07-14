/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Util.h
 * Author: 10256
 *
 * Created on 2017年2月27日, 下午5:22
 */

#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <ctime>
#include "ByteBuffer.h"

class Util {
public:
    Util();
    Util(const Util& orig);
    virtual ~Util();
    
    static void BigLittleEndianTransfer(void* src, const size_t& size);
    static void printBinary(const uint8_t& src);
    static int setupConnectionToTCPServer(const std::string& ip, const int& port, const bool& nonblock = false);
    static void sendByTcp(const int& fd, const void* ptr, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const void* ptr, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const bytebuf::ByteBuffer& byteBuffer, const size_t& offset, const size_t& size);
//    static void output(const std::string& id, const std::string& message);
//    static void output(const std::string& id, const std::string& message1, const std::string& message2);
//    static void output(const std::string& id, const std::string& message1, const std::string& message2, const std::string& message3);
//    static void output(const std::string& id, const std::string& message1, const std::string& message2, const std::string& message3, const std::string& message4);
//    static void output(const std::string& id, const std::string& message1, const std::string& message2, const std::string& message3, const std::string& message4, const std::string& message5);

    static std::string timeToStr(const std::time_t& time, const std::string& format);
    static std::string timeToStr(const std::string& format);
    static std::string timeToStr(const std::time_t& time);
    static std::string timeToStr();
private:
};

#endif /* UTIL_H */

