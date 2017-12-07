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

void tcpServiceTask();
void signal_handler(int signal);

boost::condition_variable quit;
bool offline = false;

int main(int argc, char** argv) {
    gavinlog::init("log");
    try {

        auto& uploader_pool = resource::getResource()->get_uploader_pool();

        auto uploader = boost::make_shared<Uploader>(0);
        uploader->start();
        uploader_pool.push_back(uploader);

        boost::thread TcpServiceThread(tcpServiceTask);

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

        GDEBUG(__func__) << "stopping service...";
        resource::getResource()->getIoService().stop();
        TcpServiceThread.join();
    } catch (std::exception &e) {
        GWARNING(__func__) << "exception: " << e.what();
    }
    GDEBUG("DONE") << "Service shutdown";
    GINFO("DONE") << "Service shutdown";

    return 0;
}

void signal_handler(int signal) {
    switch (signal) {
        case SIGUSR1:
            GDEBUG(__func__) << "SIGUSR1";
            offline = true;
            break;
        case SIGUSR2:
        {
            GDEBUG(__func__) << "SIGUSR2";
            size_t UploadChannelCount = resource::getResource()->get_upload_channels_count();
            if (UploadChannelCount > Constant::maxUploadChannelNum)
                UploadChannelCount = Constant::maxUploadChannelNum;

            for (size_t i = 1; i < UploadChannelCount; ++i) {
                auto uploader = boost::make_shared<Uploader>(i);
                uploader->start();
                resource::getResource()->get_uploader_pool().push_back(uploader);
            }
            break;
        }
        default:
            quit.notify_all();
    }
}

void tcpServiceTask() {
    try {
        auto& ioService = resource::getResource()->getIoService();
        TcpServer tcpServer(ioService, resource::getResource()->getEnterprisePlatformTcpServicePort());
        ioService.run();
    } catch (std::exception &e) {
        GWARNING(__func__) << "exception: " << e.what();
    }
}
