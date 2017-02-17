/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Info.h
 * Author: 10256
 *
 * Created on 2017年1月16日, 下午6:46
 */

#ifndef INFO_H
#define INFO_H

#include "DataFormat.h"
class Info{
public:
    virtual bool fillDataFromDB(void) = 0;    
private:
    PtrLen* data;
};

#endif /* INFO_H */

