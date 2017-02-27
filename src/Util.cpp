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

Util::Util() {
}

Util::Util(const Util& orig) {
}

Util::~Util() {
}

void Util::BigToLittleEndian(uint8_t src[], const size_t& size) {
    if (NULL == src || 2 > size)
        return;

    uint8_t* copy = new uint8_t[size] ;
    memcpy(copy, src, size);
    
    int j = 0;
    for (; j < size; j++) {
        src[j] = copy[size - j - 1];
    }
    delete copy;
}

void Util::printBinary(const uint8_t& src) {
    for (int i = 0; i < 8; i++) {
        printf("%d ", src >> (7 - i) & 1);
    }
    putchar('\n');
}