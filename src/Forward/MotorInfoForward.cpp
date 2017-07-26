/**
 * To do:
 * 1. 国标2：如车辆登出/平台登出/异常下线后需重新发送车辆登入
 * 当与公共平台重连后，平台重新登录，而车机不会发登录数据，是否需要代发车机登录数据？
 */


#include <cstdlib>
#include <iostream>
#include <vector>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <csignal>
#include"DataFormatForward.h"
#include "TcpServer.h"
#include "resource.h"
#include "Uploader.h"
#include "Constant.h"
#include "logger.h"

/*
 * 功能
 * 1. 执行平台登入、登出操作，共计执行5个循环，不读取回应报文
 * 2. 平台转发车机数据时不登入，直接转发，不读取回应报文
 * 3. 平台离线补发，不登入，直接转发，不读取回应报文,离线10分钟以上
 * 4. 多链路传输（默认单链路传输）,不登入，直接转发，不读取回应报文,15分钟以上
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输
 * 企业平台需要转发上级平台发来的车辆登录反馈给车机
 */

void uploadTask(const size_t& no);
void tcpServiceTask();
void signal_handler(int signal);

boost::condition_variable quit;
bool offline = false;
std::vector<boost::shared_ptr < boost::thread>> uploadThreads;

int main(int argc, char** argv) {
    gavinlog::init("log");
    try {

        resource::getResource();

        boost::thread TcpServiceThread(tcpServiceTask);

        size_t UploadChannelNumber = resource::getResource()->getUploadChannelNumber();
        if (UploadChannelNumber > Constant::maxUploadChannelNum)
            UploadChannelNumber = Constant::maxUploadChannelNum;

        for (size_t i = 0; i < UploadChannelNumber; i++) {
            boost::shared_ptr<boost::thread> uploadThread = boost::make_shared<boost::thread>(uploadTask, i);
            uploadThreads.push_back(uploadThread);
        }

#if true
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 平台符合性检测需要离线10min，通过 SIGUSR1 触发；增加辅链路，通过 SIGUSR2 触发
        if (resource::getResource()->getMode() == EnumRunMode::platformCompliance) {
            std::signal(SIGUSR1, signal_handler);
            std::signal(SIGUSR2, signal_handler);
        }

        GDEBUG("INIT") << "Service started";
        GINFO("INIT") << "Service started";
        boost::mutex mtx;
        boost::unique_lock<boost::mutex> lk(mtx);
        quit.wait(lk);

        // debug mode
        //        std::cout << "press any key to quit" << std::endl;
        //        std::cin.get();
        resource::getResource()->getIoService().stop();
        TcpServiceThread.join();
        for (std::vector<boost::shared_ptr < boost::thread>>::iterator it = uploadThreads.begin(); it != uploadThreads.end();) {
            boost::shared_ptr<boost::thread> tsptr = *it;
            tsptr->interrupt();
            tsptr->join();
            it = uploadThreads.erase(it);
        }
#endif
    } catch (std::exception &e) {
        GWARNING("main") << "exception: " << e.what();
    }
    GDEBUG("DONE") << "Service shutdown";
    GINFO("DONE") << "Service shutdown";

    return 0;
}

void signal_handler(int signal) {
    switch (signal) {
        case SIGUSR1:
            offline = true;
            break;
        case SIGUSR2:
        {
            size_t no = uploadThreads.size();
            boost::shared_ptr<boost::thread> uploadThread = boost::make_shared<boost::thread>(uploadTask, no);
            uploadThreads.push_back(uploadThread);
            break;
        }
        default:
            quit.notify_all();
    }
}

void uploadTask(const size_t& no) {
    try {
        Uploader uploader(no, (const EnumRunMode&) resource::getResource()->getMode());
        uploader.task();
    } catch (std::exception &e) {
        GWARNING("uploadTask") << "exception: " << e.what();
    }
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

/**
 * 平台符合性检测：
 * 1、 执行平台登入、登出操作，共计执行5个循环；
 * 2、 执行车辆登入、登出操作，共计执行5个循环；
 * 3、 发送单一车辆行驶数据，车辆行驶状态数据维持发送 15分钟以上；
 * 4、 发送单一车辆充电数据，车辆充电状态数据维持发送15分钟以上；
 * 5、 发送单一车辆SOC为100%时车辆充电状态数据，维持发送5分钟以上；
 * 6、 发送单一车辆报警数据，需维持该报警并持续发送5分钟以上；
 * 7、 发送车辆补发数据，车辆离线状态数据应维持10分钟以上；
 * (注意：车辆补发数据指车辆与企业平台断开连接时产生的数据)
 * 8、 发送平台补发数据，平台离线状态数据应维持10分钟以上；
 * (注意：平台补发数据指企业平台与国家平台断开连接时产生的数据，车辆与企业平台应维持连接，并持续发送数据)
 * 以上所有项目通过后，执行多车单链路、多车多链路测试。
 * 9、 多车单链路：企业平台应通过同一条tcp链路发送不同运行状态下的不同车辆数据到国家平台，车辆数不应少于3台，数据应维持发送15分钟以上；
 * 通过多车单链路测试后， 应向检测人员申请开启多链路，多链路的平台唯一码、密码与原链路相同，仅用户名为原用户名+“1”
 * 10、 多车多链路：企业应通过主链路及新开通的辅链路发送不同运行状态下的不同
 * 车辆数据到国家平台，车辆数不应少于3台，数据应维持发送15分钟以上。
 * 
 * 1. 5次登录登出
 * 2. 转发车辆数据
 * 3. 离线10分钟
 * 4. 多链路
 */

void tcpServiceTask() {
    try {
        boost::asio::io_service& ioService = resource::getResource()->getIoService();
        TcpServer tcpServer(ioService, resource::getResource()->getEnterprisePlatformTcpServicePort());
        ioService.run();
    } catch (std::exception &e) {
        GWARNING("tcpServiceTask") << "exception: " << e.what();
    }
}
