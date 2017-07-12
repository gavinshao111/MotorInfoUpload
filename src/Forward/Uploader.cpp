/* 
 * File:   Uploader.cpp
 * Author: 10256
 * 
 * Created on 2017年7月1日, 下午4:48
 */

#include "Uploader.h"
#include <stdexcept>
#include <iomanip>
#include <string.h>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <bits/stl_map.h>
#include <bits/stl_pair.h>
#include <bits/basic_string.h>
#include "ByteBuffer.h"
#include "../Util.h"
#include "Resource.h"
#include "Constant.h"
#include "ResponseReader.h"
#include <iostream>

using namespace bytebuf;
using namespace std;
using namespace gsocket;

// no 从0开始

Uploader::Uploader(const size_t& no, const EnumRunMode& mode) :
m_mode(mode),
m_serialNumber(1),
m_lastloginTime(0),
m_lastSendTime(0),
m_uploaderStatus(uploaderstatus::EnumUploaderStatus::init),
r_carDataQueue(Resource::GetResource()->GetVehicleDataQueue()),
m_publicServer(Resource::GetResource()->GetPublicServerIp(), Resource::GetResource()->GetPublicServerPort()),
m_vin(Constant::VinInital),
m_responseReader(no, m_publicServer),
r_logger(Resource::GetResource()->GetLogger()) {
    const string& usernameInIni = Resource::GetResource()->GetPublicServerUserName();
    if ((usernameInIni.length() + no) > sizeof (m_loginData.username)
            || (Resource::GetResource()->GetPublicServerPassword().length() > sizeof (m_loginData.password)))
        throw runtime_error("Uploader(): PublicServerUserName or PublicServerPassword Illegal");
    m_stream << "Uploader." << no;
    m_id = m_stream.str();
    usernameInIni.copy((char*) m_loginData.username, sizeof (m_loginData.username));
    // 平台符合性检测：多链路的平台唯一码、密码与原链路相同，仅用户名为原用户名+“1”
    // 老赵：第no条辅链路，就加no个字符“1”
    for (int i = 0; i < no; i++) {
        m_loginData.username[usernameInIni.length() + i] = '1';
    }
    Resource::GetResource()->GetPublicServerPassword().copy((char*) m_loginData.password, sizeof (m_loginData.password));
    m_loginData.encryptionAlgorithm = Resource::GetResource()->GetEncryptionAlgorithm();
    m_loginData.header.encryptionAlgorithm = m_loginData.encryptionAlgorithm;
    m_logoutData.header.encryptionAlgorithm = m_loginData.encryptionAlgorithm;

    m_loginData.header.cmdId = enumCmdCode::platformLogin;
    m_logoutData.header.cmdId = enumCmdCode::platformLogout;

    Resource::GetResource()->GetPaltformId().copy((char*) m_loginData.header.vin, VINLEN);
    Resource::GetResource()->GetPaltformId().copy((char*) m_logoutData.header.vin, VINLEN);

    uint16_t dataUnitLength = sizeof (LoginDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_loginData.header.dataUnitLength = boost::asio::detail::socket_ops::host_to_network_short(dataUnitLength);
    dataUnitLength = sizeof (LogoutDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_logoutData.header.dataUnitLength = boost::asio::detail::socket_ops::host_to_network_short(dataUnitLength);

    ofstream file;
    file.open("log/message.txt", ofstream::out | ofstream::trunc | ofstream::binary);
    // 输出到日志 vin - collect time - send time - data(decimal)
    file << setiosflags(ios::left)
            << setw(21) << setfill(' ') << "vin"
            << setw(23) << setfill(' ') << "collect_time"
            << setw(23) << setfill(' ') << "send_time"
            << "data\n"
            << endl;
    file.close();
}

Uploader::~Uploader() {
}

// 平台运行后登入，不再登出，直到TCP连接中断。

void Uploader::task() {
    boost::thread responseReaderThread(boost::bind(&ResponseReader::task, boost::ref(m_responseReader)));
    try {
        switch (m_mode) {
            case EnumRunMode::vehicleCompliance:
                setupConnection();
                for (int i = 0; i < 5; i++) {
                    setupConnAndLogin(false);
                    logout();
                }
                r_logger.info(m_id, "login logout 5 times done ");
                break;
            case EnumRunMode::release:
                //setupConnAndLogin();
                break;
            case EnumRunMode::platformCompliance:
                throw runtime_error("platformCompliance not supportted yet");
                break;
            default:
                m_stream << "unrecognize run mode: " << (int) m_mode;
                throw runtime_error(m_stream.str());
        }
        for (;;) {
            if (!m_publicServer.isConnected()) {
                setupConnAndLogin();
            }
            try {
                m_carData = r_carDataQueue.take();
            } catch (boost::thread_interrupted&) {
//                cout << m_id << " catch thread_interrupted" << endl;
                break;
            }
            if (!m_carData->hasRemaining()) {
                r_logger.warn("Uploader::run()", "take an empty realtimeData");
                continue;
            }
            m_packetHdr = (DataPacketHeader_t*) m_carData->array();
            m_vin.assign((char*) m_packetHdr->vin, sizeof (m_packetHdr->vin));
            //            m_logger.info(m_vin, "Uploader get vehicle data from data queue");

            forwardCarData();
        }
        logout();
    } catch (exception &e) {
        r_logger.error(m_id, "exception");
        r_logger.errorStream << e.what() << std::endl;
    }
    m_publicServer.close();
    responseReaderThread.interrupt();
    responseReaderThread.join();
    r_logger.info(m_id, "quiting...");
}

void Uploader::setupConnection() {
    if (m_publicServer.isConnected()) {
        return;
    }
    m_publicServer.close();
    m_publicServer.connect();
    for (; !m_publicServer.isConnected(); sleep(Resource::GetResource()->GetReSetupPeroid())) {
        r_logger.warn(m_id, "connect refused by Public Server. Reconnecting...");
        m_publicServer.connect();
    }
    r_logger.info(m_id, "connection with public platform established");
}

/*
 * Generator 在收到MQ然后将车机数据放入 dataQueue 时需要先判断网络是否里连接，若否设为补发。
 * 国标：重发本条实时信息（应该是调整后重发）。每隔1min重发，最多发送3次
 * 国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
 * 我的理解：由于平台无法做到实时调整，只能转发错误回应给车机，继续发下一条。所以不再重发本条实时信息，等车机调整后直接转发。
 * 
 * 国标：客户端平台达到重发次数后仍未收到应答应断开会话连接。
 * 我的理解：我平台每次转发车辆数据，但是上级平台未响应时，都重新登入再转发，所以不存在所谓“达到重发次数”
 * 
 * 两个国标都没提到转发实时或补发数据未收到回应的处理，企业平台该是不处理，继续转发下一条
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输。
 * 我平台转发所有上级平台的错误应答，和车辆登入的成功应答。其他应答不转发，默认成功。
 */
void Uploader::forwardCarData() {
    // 若发送异常，则当前车机数据包设为补发数据并放回发送队列。
    m_uploaderStatus = uploaderstatus::EnumUploaderStatus::init;
    tcpSendData(m_packetHdr->cmdId);
    if (m_uploaderStatus == uploaderstatus::EnumUploaderStatus::connectionClosed) {
        if (m_packetHdr->cmdId == enumCmdCode::vehicleSignalDataUpload)
            m_packetHdr->cmdId = enumCmdCode::reissueUpload;
        r_carDataQueue.put(m_carData, true);
    }
}

/**
 * 未响应则内部一直重复登入，登入回应失败直接抛异常。返回说明登入成功。
 * 
 * @param needResponse
 */
void Uploader::setupConnAndLogin(const bool& needResponse/* = true*/) {
    time_t now;
    struct tm* timeTM;

    /* 
     * 登录没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 重复resendLoginTime次没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    bool resend;
    size_t i = 0;
    do {
        resend = true;
        i++;
        if (!m_publicServer.isConnected()) {
            setupConnection();
        }

        now = time(NULL);
        timeTM = localtime(&now);
        int nowDay = timeTM->tm_mday;
        int nowMon = timeTM->tm_mon;
        int nowYear = timeTM->tm_year;
        localtime(&m_lastloginTime);

        if (m_serialNumber > Resource::GetResource()->GetMaxSerialNumber()
                || nowDay != timeTM->tm_mday
                || nowMon != timeTM->tm_mon
                || nowYear != timeTM->tm_year)
            m_serialNumber = 1;

        updateLoginData();

        tcpSendData(enumCmdCode::platformLogin);
        if (!needResponse)
            break;
        r_logger.info(m_id, "waiting for public server's response...");
        //        readResponse(Resource::GetResource()->GetReadResponseTimeOut());

        bool rereadResponse;
        do {
            rereadResponse = false;
            switch (m_responseReader.status()) {
                case responsereaderstatus::EnumResponseReaderStatus::init:
                    rereadResponse = true; // 可能ResponseReader正在sleep
                    usleep(500 * 1000);
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::connectionClosed:
                    //                setupConnection();
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::timeout:
                {
                    size_t timeToSleep = Resource::GetResource()->GetLoginTimes() >= i ?
                            Resource::GetResource()->GetLoginIntervals() : Resource::GetResource()->GetLoginIntervals2();
                    m_stream.str("");
                    m_stream << "read response " << Resource::GetResource()->GetReadResponseTimeOut()
                            << "s timeout when login, sleep " << timeToSleep << "s and resend";
                    r_logger.info(m_id, m_stream.str());
                    r_logger.warn(m_id, m_stream.str());
                    sleep(timeToSleep);
                    break;
                }
                case responsereaderstatus::EnumResponseReaderStatus::responseOk:
                    resend = false;
                    break;
                case responsereaderstatus::EnumResponseReaderStatus::responseNotOk:
                    m_stream.str("");
                    m_stream << "Uploader::setupConnAndLogin(): response not ok, response flag: " << (int) m_responseReader.responseFlag();
                    throw runtime_error(m_stream.str());
                case responsereaderstatus::EnumResponseReaderStatus::responseFormatErr:
                    //                throw runtime_error("Uploader::setupConnAndLogin(): bad response format");
                    r_logger.warn("Uploader::setupConnAndLogin", "bad response format");
                    break;
                default:
                    m_stream.str("");
                    m_stream << "Uploader::setupConnAndLogin(): unrecognize ResponseReaderStatus code: " << m_responseReader.status();
                    throw runtime_error(m_stream.str());
            }
        } while (rereadResponse);
    } while (resend);

    m_serialNumber++;
    m_lastloginTime = time(NULL);
    r_logger.info(m_id, "platform login done");
}

void Uploader::logout() {
    updateLogoutData();
    tcpSendData(enumCmdCode::platformLogout);
    r_logger.info(m_id, "platform logout done");
}

void Uploader::updateLoginData() {
    time_t now;
    struct tm* nowTM;
    now = time(NULL);
    nowTM = localtime(&now);

    m_loginData.time.year = nowTM->tm_year - 100;
    m_loginData.time.mon = nowTM->tm_mon + 1;
    m_loginData.time.mday = nowTM->tm_mday;
    m_loginData.time.hour = nowTM->tm_hour;
    m_loginData.time.min = nowTM->tm_min;
    m_loginData.time.sec = nowTM->tm_sec;
    m_loginData.serialNumber = m_serialNumber;
    m_loginData.serialNumber = boost::asio::detail::socket_ops::host_to_network_short(m_serialNumber);
}

void Uploader::updateLogoutData() {
    time_t now;
    struct tm* nowTM;
    now = time(NULL);
    nowTM = localtime(&now);

    m_logoutData.time.year = nowTM->tm_year - 100;
    m_logoutData.time.mon = nowTM->tm_mon + 1;
    m_logoutData.time.mday = nowTM->tm_mday;
    m_logoutData.time.hour = nowTM->tm_hour;
    m_logoutData.time.min = nowTM->tm_min;
    m_logoutData.time.sec = nowTM->tm_sec;
    m_logoutData.serialNumber = m_loginData.serialNumber;
}

/**
 * 根据指令发送 1.平台登入 2.平台登出 3.转发车机数据 并将发送数据写入日志
 * 如果连接断开 m_senderStatus 设为 senderstatus::EnumUploaderStatus::connectionClosed
 * 
 * @param cmd
 */
void Uploader::tcpSendData(const uint8_t& cmd) {
    size_t sizeToSend;
    BytebufSPtr_t dataToSend;
    time_t collectTime;
    string cmdTypeStr;
    try {
        switch (cmd) {
            case enumCmdCode::platformLogin:
                dataToSend = boost::make_shared<ByteBuffer>((uint8_t*) & m_loginData, sizeof (LoginDataForward_t));
                m_loginData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LoginDataForward_t) - 3);
                collectTime = time(NULL);
                cmdTypeStr = Constant::CmdPlatformLoginStr;
                break;
            case enumCmdCode::platformLogout:
                dataToSend = boost::make_shared<ByteBuffer>((uint8_t*) & m_logoutData, sizeof (LogoutDataForward_t));
                m_logoutData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LogoutDataForward_t) - 3);
                collectTime = time(NULL);
                cmdTypeStr = Constant::CmdPlatformLogoutStr;
                break;
            case enumCmdCode::vehicleSignalDataUpload:
                cmdTypeStr = Constant::CmdRealtimeUploadStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::reissueUpload:
                cmdTypeStr = Constant::CmdReissueUploadStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::vehicleLogin:
                cmdTypeStr = Constant::CmdVehicleLoginStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::vehicleLogout:
                cmdTypeStr = Constant::CmdVehicleLogoutStr;
                dataToSend = m_carData;
                break;
            default:
                m_stream.str("[");
                m_stream << m_vin << "] Uploader::tcpSendData(): Illegal cmd: " << cmd;
                throw runtime_error(m_stream.str());
        }

        TimeForward_t* ptime = (TimeForward_t*) (dataToSend->array() + sizeof (DataPacketHeader));
        struct tm timeTM;
        timeTM.tm_year = ptime->year + 100;
        timeTM.tm_mon = ptime->mon - 1;
        timeTM.tm_mday = ptime->mday;
        timeTM.tm_hour = ptime->hour;
        timeTM.tm_min = ptime->min;
        timeTM.tm_sec = ptime->sec;
        collectTime = mktime(&timeTM);

        sizeToSend = dataToSend->remaining();
        m_publicServer.write(*dataToSend);
        // 为了打印日志，将pos指针退回
        dataToSend->movePosition(sizeToSend, true);
        m_lastSendTime = time(NULL);

        m_stream.str("");
        m_stream << sizeToSend << " bytes of " << cmdTypeStr << " data uploaded";
        r_logger.info(m_vin, m_stream.str());

        ofstream file;
        file.open("log/message.txt", ofstream::out | ofstream::binary | ofstream::app);
        outputMsg(file, m_vin, collectTime, m_lastSendTime, dataToSend.get());
        file.close();
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        r_logger.warn("Uploader::tcpSendData exception");
        r_logger.warnStream << e.what() << std::endl;
        m_uploaderStatus = uploaderstatus::EnumUploaderStatus::connectionClosed;
    }
}

void Uploader::outputMsg(ostream& out, const string& vin, const time_t& collectTime, const time_t& sendTime, const ByteBuffer* data/* = NULL*/) {
    out << setiosflags(ios::left)
            << setw(21) << setfill(' ') << vin
            << setw(23) << setfill(' ') << Util::timeToStr(collectTime)
            << setw(23) << setfill(' ') << Util::timeToStr(sendTime);
    if (data != NULL) {
        data->outputAsDec(out);
    }
    out << endl;
    out.flush();
}