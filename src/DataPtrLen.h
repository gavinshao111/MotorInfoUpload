/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataPtrLen.h
 * Author: 10256
 *
 * Created on 2017年2月28日, 下午7:11
 */

#ifndef DATAPTRLEN_H
#define DATAPTRLEN_H
#include <string>
#include <ByteBuffer.h>

class DataPtrLen{
public:
    bytebuf::ByteBuffer* m_dataBuf;
//    uint8_t* m_dataArray;
    //size_t m_length;
    
    std::string m_vin;
    bool m_isReissue;
    
    DataPtrLen(const std::string& vin, const size_t& length, const bool& isReissue);
//    DataPtrLen(uint8_t* ptr, const size_t& length);
    DataPtrLen(const DataPtrLen& orig);
    virtual ~DataPtrLen();

    time_t getCollectTime() const;
    
private:
    
};

#endif /* DATAPTRLEN_H */

