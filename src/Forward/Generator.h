/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Generator.h
 * Author: 10256
 *
 * Created on 2017年3月9日, 下午4:24
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "DataFormatForward.h"

class Generator {
public:
    StaticResourceForward* m_staticResource;
    
    Generator(StaticResourceForward* staticResource);
    Generator(const Generator& orig);
    virtual ~Generator();
    
    void run();
private:
    
    void subscribeMq();
};

#endif /* GENERATOR_H */

