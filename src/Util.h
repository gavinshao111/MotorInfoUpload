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
#include "ByteBuffer.h"

class Util {
public:
    static void BigLittleEndianTransfer(void* src, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const void* ptr, const size_t& size);
    static uint8_t generateBlockCheckCharacter(const bytebuf::ByteBuffer& byteBuffer,
            const size_t& offset, const size_t& size);

private:
};

#endif /* UTIL_H */

