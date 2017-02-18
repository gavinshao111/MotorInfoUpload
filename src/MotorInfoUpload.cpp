#include <cstdlib>
#include <iostream>
#include <string.h>
#include <string>
#include <time.h>
#include <thread>
#include <exception.h>
#include <warning.h>

#include "mysql_connection.h"
#include "mysql_driver.h"
#include "driver.h"
#include "statement.h"
#include "prepared_statement.h"
#include "DataFormat.h"
#include "DataGenerator.h"
#include "DataSender.h"

using namespace std;
using namespace sql;
using namespace blockqueue;
using namespace bytebuf;
/*
 * BYTE 无符号单字节整型（字节，8位）
 * WORD 无符号双字节整型（字，16位）
 * DWORD 无符号四字节整型（双字，32位）
 * STRING ASCII字符码，若无数据则放一个0终结符，编码表示参见GB/T 1988中5.1所述含汉字时，
 *  采用区位码编码，占用2个字节，编码表示参见GB 18030中6所述
 */


void dataGeneratorTask(DataGenerator* dataGenerator);
void dataSenderTask(DataSender* dataSender);
void setupDBConnection(StaticResource& staticResource);
void closeDBConnection(StaticResource& staticResource);
#if TEST
extern const string vinForTest = "0123456789abcdefg";
extern const string StrlastUploadTimeForTest = "2017-02-17 11:23:03";
#endif
int main(int argc, char** argv) {
    StaticResource staticResource;
    try {
        staticResource.PublicServerIp = "10.34.16.76";
        staticResource.PublicServerPort = 1234;
        staticResource.PublicServerUserName = "helloworld";
        staticResource.PublicServerPassword = "helloworld";
        staticResource.Period = 3600;
        staticResource.EncryptionAlgorithm = enumEncryptionAlgorithm::null;
        staticResource.DBHostName = "tcp://120.26.86.124:3306";
        staticResource.DBUserName = "root";
        staticResource.DBPassword = "10214";
        staticResource.MQServerUrl = "tcp://120.26.86.124:1883";
        staticResource.MQTopic = "/+/lpcloud/candata";
        staticResource.MQClientID = "MotorInfoUpload";
        staticResource.MQServerUserName = "easydarwin";
        staticResource.MQServerPassword = "123456";
        staticResource.LoginTimeout = 60;
        staticResource.LoginTimeout2 = 180;
        staticResource.RealtimeDataResendTime = 60;

        staticResource.DBDriver = get_driver_instance();
        staticResource.dataQueue = new BlockQueue<DataPtrLen*>(100);
        setupDBConnection(staticResource);


        if (false) {
            time_t time;
            char strCurrUploadTime[20];
            struct tm* currUploadTimeTM;
            PreparedStatement* prepStmt = NULL;
            const static string sqlTemplate = "update car_infokey set infovalue=(?) where infokey='gblastupload'";
            int result = -1;
            
            prepStmt = staticResource.DBConn->prepareStatement(sqlTemplate);
            prepStmt->setString(1, "2017-02-17 11:23:03");
            result = prepStmt->executeUpdate();
            delete prepStmt;
            cout << result << endl;
            
            memset(strCurrUploadTime, 0, sizeof (strCurrUploadTime));
            DataGenerator::getLastUploadInfo(&staticResource, time);
            currUploadTimeTM = localtime(&time);
            strftime(strCurrUploadTime, 19, TIMEFORMAT, currUploadTimeTM);
            cout << strCurrUploadTime << endl;
        } else {
            DataGenerator* dataGenerator = new DataGenerator(&staticResource);
            DataSender* dataSender = new DataSender(&staticResource);

            thread DataGeneratorThread(dataGeneratorTask, dataGenerator);
            thread DataSenderThread(dataSenderTask, dataSender);

            DataGeneratorThread.join();
            DataSenderThread.join();
        }
        goto Finally;

    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
        goto Finally;
    }
Finally:
    closeDBConnection(staticResource);
    cout << "done." << endl;
    return 0;
}

void dataGeneratorTask(DataGenerator* dataGenerator) {
    dataGenerator->run();
}

void dataSenderTask(DataSender* dataSender) {
    dataSender->run();
}

void setupDBConnection(StaticResource& staticResource) {
    staticResource.DBConn = staticResource.DBDriver->connect(staticResource.DBHostName, staticResource.DBUserName, staticResource.DBPassword);
    //conn->setAutoCommit(false);
    staticResource.DBState = staticResource.DBConn->createStatement();
    staticResource.DBState->execute("use lpcarnet");
}

void closeDBConnection(StaticResource& staticResource) {
    if (NULL != staticResource.DBState) {
        staticResource.DBState->close();
        delete staticResource.DBState;
    }
    if (NULL != staticResource.DBConn && staticResource.DBConn->isClosed()) {
        staticResource.DBConn->close();
        delete staticResource.DBConn;
    }
}
