#include <cstdlib>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>

#include"DataFormatForward.h"
#include "../BlockQueue.h"
#include "DataPacketForward.h"
#include "../DataFormat.h"
#include "Sender.h"
#include "Generator.h"
#include "GSocketClient.h"
//#include "glog/logging.h"

using namespace blockqueue;
using namespace std;
using namespace gavinsocket;

/*
 * 功能
 * 1. 执行平台登入、登出操作，共计执行5个循环，不读取回应报文
 * 2. 平台转发车机数据时不登入，直接转发，不读取回应报文
 * 3. 平台离线补发，不登入，直接转发，不读取回应报文,离线10分钟以上
 * 4. 多链路传输（默认单链路传输）,不登入，直接转发，不读取回应报文,15分钟以上
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输
 * 企业平台需要转发上级平台发来的车辆登录反馈给车机
 */

void senderTask(Sender* sender);
void senderForCarComplianceTask(Sender* sender);
void generatorTask(Generator* generator);
void readStdinTask();
void StaticResourceInit();
void deleteStaticResource();
StaticResourceForward staticResourceForward;
bool exitNow = false;
mutex generator_mtx;
condition_variable connlostOrExitNow;

int main(int argc, char** argv) {
    try {
//        google::InitGoogleLogging(argv[0]);
//        FLAGS_log_dir = "./log";

        StaticResourceInit();
        Sender* sender = new Sender(&staticResourceForward);
        Generator* generator = new Generator(&staticResourceForward);

        //        thread SenderThread(senderTask, sender);
        thread SenderThread(senderForCarComplianceTask, sender);
        thread GeneratorThread(generatorTask, generator);
        thread ReadStdinThread(readStdinTask);

        SenderThread.join();
        GeneratorThread.join();
        ReadStdinThread.join();

        delete sender;
        delete generator;
        deleteStaticResource();
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    cout << "done." << endl;
//    google::ShutdownGoogleLogging();

    return 0;
}

/*
 * init resource
 * 1. start Sender.
 * 2. subscribe on MQ
 * 3. when MQ arrive, put into data queue
 * Sender:
 * 1. if isConnected {
 *      send.
 *    else
 *      sleep(1);   // 每一秒去轮询是否建立了连接。
 */
void senderTask(Sender* sender) {
    cout << "sender:" << this_thread::get_id() << endl;
    sender->run();
}

/* 车辆符合性检测：
 * 1. 平台连续5次登入登出
 * 2. 转发车辆连续5次登入登出
 * 3. 转发单一车辆行驶状态下的数据1小时
 * 4. 转发车辆充电状态数据1小时
 * 5. 转发车辆soc为100%数据5分钟
 * 6. 转发报警数据5分钟
 * 7. 车辆离线10分钟，转发车辆补发数据
 * 日志内容应包含服务器发送时间、报文时间、国标报文内容及车辆vin
 */
void senderForCarComplianceTask(Sender* sender) {
    cout << "sender:" << this_thread::get_id() << endl;
    sender->runForCarCompliance();
}

void generatorTask(Generator* generator) {
    cout << "generator:" << this_thread::get_id() << endl;
    generator->run();

}

void readStdinTask() {
    for (; cin.get() != 'q';);
    exitNow = true;
    unique_lock<mutex> lk(generator_mtx);
    connlostOrExitNow.notify_one();
//    cout << "readStdinTask done" << endl;
}

void StaticResourceInit() {
    staticResourceForward.PublicServerIp = "10.34.16.94"; //"218.205.176.44";
    staticResourceForward.PublicServerPort = 1234;
    staticResourceForward.PublicServerUserName = "123456789012";
    staticResourceForward.PublicServerPassword = "12345678901234567890";
    staticResourceForward.EncryptionAlgorithm = enumEncryptionAlgorithm::null;
    staticResourceForward.MQServerUrl = "tcp://120.26.86.124:1883";
    staticResourceForward.MQTopic = "/+/lpcloud/candata";
    staticResourceForward.MQClientID = "MotorInfoUpload";
    staticResourceForward.MQServerUserName = "easydarwin";
    staticResourceForward.MQServerPassword = "123456";
    staticResourceForward.ReadResponseTimeOut = 10;
    staticResourceForward.LoginTimes = 3;
    staticResourceForward.LoginIntervals = 60;
    staticResourceForward.LoginIntervals2 = 1800;
    staticResourceForward.CarDataResendIntervals = 60;
    //    staticResourceForward.MaxSendCarDataTimes = 3;
    staticResourceForward.CarDataResendIntervals = 60;
    staticResourceForward.dataQueue = new BlockQueue<DataPacketForward*>(100);

    //城市邮政编码(310000) + VIN前三位（123?）+ 两位自定义数据(00?) + "000000"
    staticResourceForward.PaltformId = "31000012300000000";
    staticResourceForward.ReSetupPeroid = 1;
    staticResourceForward.tcpConnection = new GSocketClient(staticResourceForward.PublicServerIp, staticResourceForward.PublicServerPort, false);
    staticResourceForward.MaxSerialNumber = 65531;
}

void deleteStaticResource() {
    delete staticResourceForward.dataQueue;
    delete staticResourceForward.tcpConnection;
}