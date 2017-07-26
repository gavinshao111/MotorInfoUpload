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
#include <cstring>
#include <stdexcept>
#include "Forward/Constant.h"

using namespace std;

void Util::BigLittleEndianTransfer(void* src, const size_t& size) {
    if (NULL == src || 2 > size)
        return;

    uint8_t* src2 = (uint8_t*) src;
    uint8_t* copy = new uint8_t[size];
//    memcpy(copy, src2, size);
    std::copy(src2, src2 + size, copy);

    int j = 0;
    for (; j < size; j++) {
        src2[j] = copy[size - j - 1];
    }
    delete copy;
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
