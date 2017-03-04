/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DBConnection.h
 * Author: 10256
 *
 * Created on 2017年2月20日, 下午5:29
 */

#ifndef DBCONNECTION_H
#define DBCONNECTION_H
#include "DataFormat.h"

class DBConnection {
public:
    DBConnection(StaticResource* staticResource);
    DBConnection(const DBConnection& orig);
    virtual ~DBConnection();
protected:
    sql::Connection* m_DBConn;
    sql::Statement* m_DBState;
    StaticResource* m_staticResource;
    void setupDBConnection();
    virtual void closeDBConnection();
    void getLastUploadInfoFromDB(time_t& uploadTime);
};

#endif /* DBCONNECTION_H */

