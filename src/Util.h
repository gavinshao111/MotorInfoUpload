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
class Util {
public:
    Util();
    Util(const Util& orig);
    virtual ~Util();
    static void BigToLittleEndian(uint8_t src[], const size_t& size);
    static void printBinary(const uint8_t& src);
private:

};

#endif /* UTIL_H */

