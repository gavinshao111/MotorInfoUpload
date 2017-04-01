#define UseBoostMutex true
#define CarCompliance true

#include <cstdlib>
#include <iostream>
#include <boost/thread.hpp>

#include"DataFormatForward.h"
//#include "../BlockQueue.h"
//#include "DataPacketForward.h"
#include "../DataFormat.h"
#include "Sender.h"
#include "Generator.h"
//#include "GSocketClient.h"
//#include "glog/logging.h"

//using namespace std;



/*
 * 功能
 * 1. 执行平台登入、登出操作，共计执行5个循环，不读取回应报文
 * 2. 平台转发车机数据时不登入，直接转发，不读取回应报文
 * 3. 平台离线补发，不登入，直接转发，不读取回应报文,离线10分钟以上
 * 4. 多链路传输（默认单链路传输）,不登入，直接转发，不读取回应报文,15分钟以上
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输
 * 企业平台需要转发上级平台发来的车辆登录反馈给车机
 */

void senderTask(StaticResourceForward& resource, DataQueue_t& dataQueue, TcpConn_t& tcpConnection);
void generatorTask(StaticResourceForward& resource, DataQueue_t& dataQueue, TcpConn_t& tcpConnection);
void StaticResourceInit(StaticResourceForward& resource);

bool exitNow = false;
int main(int argc, char** argv) {
    try {
        //        google::InitGoogleLogging(argv[0]);
        //        FLAGS_log_dir = "./log";

        StaticResourceForward resource;
        StaticResourceInit(resource);
        DataQueue_t CarDataQueue(1024);
        DataQueue_t ResponseDataQueue(1024);
        TcpConn_t tcpConnection((resource.PublicServerIp, resource.PublicServerPort));

        boost::thread SenderThread(senderTask, resource, CarDataQueue, ResponseDataQueue, tcpConnection);
        boost::thread GeneratorThread(generatorTask, resource, CarDataQueue, ResponseDataQueue, tcpConnection);

        for (; std::cin.get() != 'q';);
        SenderThread.interrupt();
        GeneratorThread.interrupt();
        
        SenderThread.join();
        GeneratorThread.join();

        //        deleteStaticResource();
    } catch (std::exception &e) {
        std::cout << "ERROR: Exception in " << __FILE__;
        std::cout << " (" << __func__ << ") on line " << __LINE__ << std::endl;
        std::cout << "ERROR: " << e.what() << std::endl;
    }
    std::cout << "done." << std::endl;
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
void senderTask(StaticResourceForward& resource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection) {
    Sender sender(resource, CarDataQueue, ResponseDataQueue, tcpConnection);
    std::cout << "sender:" << boost::this_thread::get_id() << std::endl;
#if CarCompliance
    sender.runForCarCompliance();
#else
    sender.run();
#endif
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

void generatorTask(StaticResourceForward& resource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection) {
    Generator generator(resource, CarDataQueue, ResponseDataQueue, tcpConnection);
    std::cout << "generator:" << boost::this_thread::get_id() << std::endl;
    generator.run();

}

void StaticResourceInit(StaticResourceForward& resource) {
    resource.PublicServerIp = "10.34.16.94"; //"218.205.176.44";
    resource.PublicServerPort = 1234;
    resource.PublicServerUserName = "123456789012";
    resource.PublicServerPassword = "12345678901234567890";
    resource.EncryptionAlgorithm = enumEncryptionAlgorithm::null;
    resource.MQServerUrl = "tcp://120.26.86.124:1883";
    resource.MQTopicForCarData = "/+/lpcloud/candata/gbt32960/upload";
    resource.MQTopicForResponseData = "/+/lpcloud/candata/gbt32960/response";
//    resource.MQTopicRegexForCarData = "^/[a-zA-Z0-9]{17}/lpcloud/candata/gbt32960/upload$";
//    resource.MQTopicRegexForResponseData = "^/[a-zA-Z0-9]{17}/lpcloud/candata/gbt32960/response$";
    
    resource.MQClientID = "MotorInfoUpload";
    resource.MQServerUserName = "easydarwin";
    resource.MQServerPassword = "123456";
    
    std::string pathOfED = getenv("ED");
    resource.pathOfServerPublicKey += pathOfED;
    resource.pathOfServerPublicKey.append("/emqtt.pem");
    resource.pathOfPrivateKey += pathOfED;
    resource.pathOfPrivateKey.append("/emqtt.key");

    resource.ReadResponseTimeOut = 10;
    resource.LoginTimes = 3;
    resource.LoginIntervals = 60;
    resource.LoginIntervals2 = 1800;
    resource.CarDataResendIntervals = 60;
    //    resource.MaxSendCarDataTimes = 3;
    resource.CarDataResendIntervals = 60;
    //    resource.dataQueue = new BlockQueue<DataPacketForward*>(100);
//    resource.dataQueue = boost::make_shared<BlockQueue < DataPacketForward*>>(100);

    //城市邮政编码(310000) + VIN前三位（123?）+ 两位自定义数据(00?) + "000000"
    resource.PaltformId = "31000012300000000";
    resource.ReSetupPeroid = 1;
    //    resource.tcpConnection = new GSocketClient(resource.PublicServerIp, resource.PublicServerPort, false);
//    resource.tcpConnection = boost::make_shared<GSocketClient>(resource.PublicServerIp, resource.PublicServerPort);
    resource.MaxSerialNumber = 65531;
}

//void deleteStaticResource() {
//    delete staticResourceForward.dataQueue;
//    delete staticResourceForward.tcpConnection;
//}