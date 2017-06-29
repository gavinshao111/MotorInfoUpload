#include <cstdlib>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include"DataFormatForward.h"
#include "Sender.h"
#include "TcpServer.h"
#include "Resource.h"
#include "../Util.h"
//#include "glog/logging.h"

/*
 * 功能
 * 1. 执行平台登入、登出操作，共计执行5个循环，不读取回应报文
 * 2. 平台转发车机数据时不登入，直接转发，不读取回应报文
 * 3. 平台离线补发，不登入，直接转发，不读取回应报文,离线10分钟以上
 * 4. 多链路传输（默认单链路传输）,不登入，直接转发，不读取回应报文,15分钟以上
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输
 * 企业平台需要转发上级平台发来的车辆登录反馈给车机
 */

void senderTask();
void TcpServiceTask();

int main(int argc, char** argv) {
    try {
        Resource::GetResource();
        
        boost::thread SenderThread(senderTask);
        boost::thread TcpServiceThread(TcpServiceTask);

#if true
        // debug mode
        std::cout << "press any key to quit" << std::endl;
        std::cin.get();
        SenderThread.interrupt();
        Resource::GetResource()->GetIoService().stop();
        
        SenderThread.join();
        TcpServiceThread.join();
#endif
    } catch (std::exception &e) {
        Resource::GetResource()->GetLogger().error("main exception");
        Resource::GetResource()->GetLogger().errorStream << e.what() << std::endl;
    }
    std::cout << "done." << std::endl;

    return 0;
}

void senderTask() {
    Sender sender;
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

void TcpServiceTask() {
    boost::asio::io_service& ioService = Resource::GetResource()->GetIoService();
    TcpServer tcpServer(ioService, Resource::GetResource()->GetEnterprisePlatformTcpServicePort());
    ioService.run();
}
