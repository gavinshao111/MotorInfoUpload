/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Logger.cpp
 * Author: 10256
 * 
 * Created on 2017年6月28日, 下午3:51
 */

#include "Logger.h"
#include "../Util.h"

using namespace std;

Logger::Logger() {
    infoStream.open("log/info.txt", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
    warnStream.open("log/warn.txt", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
//    errorStream.open("log/error.txt", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
    errorStream.open("log/warn.txt", std::ofstream::out | std::ofstream::app | std::ofstream::binary);
}

Logger::Logger(const Logger& orig) {
}

Logger::~Logger() {
    infoStream.close();
    warnStream.close();
    errorStream.close();
}

void Logger::info(const string& id) {
    output(infoStream, id, "");
}
void Logger::warn(const string& id) {
    output(warnStream, id, "");
}
void Logger::error(const string& id) {
    output(errorStream, id, "");
}
void Logger::info(const string& id, const string& message) {
    output(infoStream, id, message);
}
void Logger::warn(const string& id, const string& message) {
    output(warnStream, id, message);
}
void Logger::error(const string& id, const string& message) {
    output(errorStream, id, message);
}
void Logger::output(ofstream& ofs, const string& id, const string& message) {
    ofs << "\n[" << id << "] " << message << " " << Util::nowtimeStr() << endl;
}
