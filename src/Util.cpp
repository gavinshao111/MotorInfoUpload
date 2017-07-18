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
#include <iostream>
#include "Forward/Constant.h"

using namespace std;

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

string Util::timeToStr(const time_t& time, const string& format) {
    struct tm* timeTM;
    char strTime[100] = {0};

    timeTM = localtime(&time);
    strftime(strTime, sizeof (strTime) - 1, format.c_str(), timeTM);
    string timeStr(strTime);
    return timeStr;
}

string Util::timeToStr(const time_t& time) {
    return timeToStr(time, Constant::defaultTimeFormat);
}

string Util::timeToStr(const string& format) {
    return timeToStr(time(NULL), format);
}
string Util::timeToStr() {
    return timeToStr(Constant::defaultTimeFormat);
}

vector<string> Util::str_split(const string& src, const char& separator) {
    vector<string> dest;
    size_t curr = 0;
    size_t next = 0;
    
    for (; curr < src.length();) {
        next = src.find(separator, curr);
        if (next == string::npos) {
            string tmp = str_trim(src.substr(curr));
            if (tmp.size() > 0)
                dest.push_back(tmp);
            break;
        }
        string&& tmp = str_trim(src.substr(curr, next - curr));
        if (tmp.size() > 0)
            dest.push_back(tmp);
        curr = next + 1;
    }

    return dest;
}

string Util::str_trim(string&& src) {
    size_t offset = src.find_first_not_of(' ');
    if (offset == string::npos) return "";
    return src.substr(offset, src.find_last_not_of(' ') - offset + 1);
}
