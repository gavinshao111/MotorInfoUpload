/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DataAddrLen.h
 * Author: 10256
 *
 * Created on 2017年1月13日, 上午10:43
 */

#ifndef DATAADDRLEN_H
#define DATAADDRLEN_H

class DataAddrLen {
public:
    DataAddrLen();
    DataAddrLen(const DataAddrLen& orig);
    virtual ~DataAddrLen();
private:
    char* address;
    size_t length;
};

#endif /* DATAADDRLEN_H */

