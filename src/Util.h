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

class Util {
public:
    Util();
    Util(const Util& orig);
    virtual ~Util();
    static void BigLittleEndianTransfer(void* src, const size_t& size);
    static void printBinary(const uint8_t& src);
    static int setupConnectionToTCPServer(const std::string& ip, const int& port, const bool& nonblock = false);
    static std::string timeToStr(const time_t& time);
    static void sendByTcp(const int& fd, const void* ptr, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const void* ptr, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const uint8_t& first, const void* ptr, const size_t& size);
private:

};

#endif /* UTIL_H */

