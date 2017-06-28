/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Logger.h
 * Author: 10256
 *
 * Created on 2017年6月28日, 下午3:51
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

class Logger {
public:
    std::ofstream infoStream;
    std::ofstream warnStream;
    std::ofstream errorStream;

    
    Logger();
    Logger(const Logger& orig);
    virtual ~Logger();
    
    void info(const std::string& id);
    void warn(const std::string& id);
    void error(const std::string& id);
    void info(const std::string& id, const std::string& message);
    void warn(const std::string& id, const std::string& message);
    void error(const std::string& id, const std::string& message);
private:
    void output(std::ofstream& ofs, const std::string& id, const std::string& message);
};

#endif /* LOGGER_H */

