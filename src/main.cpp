#include <cstdlib>
#include <iostream>
#include <string.h>
#include <string>
#include <time.h>


#include "mysql_connection.h"
#include "mysql_driver.h"
#include "driver.h"
#include "statement.h"
#include "prepared_statement.h"
#include "DataFormat.h"
#include "MotorInfoUpload.h"
#include <exception.h>
#include <warning.h>

using namespace std;
using namespace sql;
/*
 * BYTE 无符号单字节整型（字节，8位）
 * WORD 无符号双字节整型（字，16位）
 * DWORD 无符号四字节整型（双字，32位）
 * STRING ASCII字符码，若无数据则放一个0终结符，编码表示参见GB/T 1988中5.1所述含汉字时，
 *  采用区位码编码，占用2个字节，编码表示参见GB 18030中6所述
 */

int main(int argc, char** argv) {

//    MotorInfoUpload* motorInfoUpload = new MotorInfoUpload("10.34.16.76", 9527);
//    //motorInfoUpload.login();
//    motorInfoUpload->uploadData();
//    //motorInfoUpload.logout();
    
    
    
    Driver* driver;
    Connection* conn;
    Statement* state;
    ResultSet* result;
    PreparedStatement* prep_stmt;
    
    try {
        driver = get_driver_instance();
        conn = driver->connect("tcp://120.26.86.124:3306", "root", "10214");
        //conn->setAutoCommit(false);
        state = conn->createStatement();
        state->execute("use lpcarnet");
        
        time_t lt = time(NULL);
        struct tm* ptr = localtime(&lt);
        char szBuffer[30] = {0};
        const char* pFormat = "%Y-%m-%d %H:%M:%S";    
        strftime(szBuffer, 30, pFormat, ptr);    
        //cout << szBuffer << endl;

        char sql[512] = {0};
        snprintf(sql, sizeof(sql), "select * from car_signal where carid=123 and signal_type=20 and build_time<'%s' order by build_time desc limit 1", szBuffer);
        //result = state->executeQuery(sql);

        prep_stmt = conn->prepareStatement("select * from car_signal where carid=(?) and signal_type=(?) and build_time<(?) order by build_time desc limit 1");
        //prep_stmt = conn->prepareStatement("select * from car_signal where carid=(?) and signal_type=(?) order by build_time desc limit 1");
        prep_stmt->setUInt64(1, 123);
        prep_stmt->setUInt(2, 20);
        prep_stmt->setDateTime(3, szBuffer);
        result = prep_stmt->executeQuery();

        //result = state->executeQuery("select * from car_signal where carid=123 and signal_type=20 order by build_time desc limit 1");
        
        
        for(; result->next();){
            cout << result->getString("signalid") << endl;
            byte a = result->getUInt("signal_value");
            printf("%d\n", a);
            //cout << result->getString("build_time") << endl;
        }
        
        prep_stmt = conn->prepareStatement("select * from car_signal where carid=(?) and signal_type=(?) and build_time<'(?)' order by build_time desc limit 1");
        prep_stmt->setUInt64(1, 123);
        prep_stmt->setUInt(2, 20);
        prep_stmt->setDateTime(2, szBuffer);
        
        
        state->close();
        conn->close();

        delete result;
        delete state;
        delete conn;
    } catch (SQLException &e) {
        cout << "ERROR: SQLException in " << __FILE__;
        cout << " (" << __func__<< ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what();
        return EXIT_FAILURE;
    } catch (std::runtime_error &e) {
        cout << "ERROR: runtime_error in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    cout<<"done."<<endl;    
    return 0;
}
