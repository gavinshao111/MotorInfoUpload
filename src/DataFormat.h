/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataFormat.h
 * Author: 10256
 *
 * Created on 2017年1月9日, 上午11:22
 */

#ifndef DATAFORMAT_H
#define DATAFORMAT_H
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "ByteBuffer.h"

#ifndef byte
typedef unsigned char byte;
#endif

#define BUF_SIZE 1024
#define INFO_SIZE 1024*5

typedef struct TypeSignalInfo{
    byte signalTypeCode;
    byte length;
}SignalInfo;

typedef struct TypeCarBaseInfo{
    uint64_t carId;
    std::string vin;
}CarBaseInfo;

class CarData {
public:
    uint64_t m_carIndex;
    bytebuf::ByteBuffer* m_data;
    
    CarData(const size_t& size) : m_carIndex(0), m_data(bytebuf::ByteBuffer::allocate(size)){}
    CarData(const CarData& orig) {}
    virtual ~CarData() {
        m_data->freeMemery();
        delete m_data;
    }

private:

};

enum enumEncryptionAlgorithm {
    null = 1,
    rsa = 2,
    aes128 = 3,    
};

#define TIMEFORMAT "%Y-%m-%d %H:%M:%S"

#define TIMEOUT 1
#define OK 0
    
#endif /* DATAFORMAT_H */

