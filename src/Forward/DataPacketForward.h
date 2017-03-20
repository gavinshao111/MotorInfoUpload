/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataPacketForward.h
 * Author: 10256
 *
 * Created on 2017年3月9日, 下午3:18
 */

#ifndef DATAPACKETFORWARD_H
#define DATAPACKETFORWARD_H

#include <ByteBuffer.h>
#include <string>

class DataPacketForward {
public:
    bytebuf::ByteBuffer* m_dataBuf;
    std::string m_vin;
    
    DataPacketForward(const std::string& vin, const size_t& length);
    DataPacketForward(const DataPacketForward& orig);
    virtual ~DataPacketForward();
    
    time_t getCollectTime() const;
    void setReissue();
    
private:

};

#endif /* DATAPACKETFORWARD_H */

