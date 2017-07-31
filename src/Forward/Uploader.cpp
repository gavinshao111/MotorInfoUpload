/* 
 * File:   Uploader.cpp
 * Author: 10256
 * 
 * Created on 2017年7月1日, 下午4:48
 */

#include "Uploader.h"
#include <stdexcept>
#include <iomanip>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <bits/stl_map.h>
#include <bits/stl_pair.h>
#include <bits/basic_string.h>
#include "ByteBuffer.h"
#include "../Util.h"
#include "resource.h"
#include "Constant.h"
#include "ResponseReader.h"
#include <iostream>
#if __cplusplus < 201103L
#include <boost/lexical_cast.hpp>
#define to_str(value) boost::lexical_cast<string>(value)
#else
#define to_str(value) to_string((int)value)
#endif
#include "logger.h"
#include "utility.h"

using namespace bytebuf;
using namespace std;
using namespace gsocket;

extern bool offline;
bool Uploader::isConnectWithPublicServer(false);
// no 从0开始

Uploader::Uploader(const size_t& no) :
m_serialNumber(1),
m_lastloginTime(0),
m_lastSendTime(0),
m_uploaderStatus(uploaderstatus::EnumUploaderStatus::init),
r_resource(resource::getResource()),
r_carDataQueue(r_resource->getVehicleDataQueue()),
m_mode((EnumRunMode)r_resource->getMode()),
m_publicServer(r_resource->getPublicServerIp(), r_resource->getPublicServerPort()),
m_vin(Constant::vinInital),
m_responseReader(no, m_publicServer) {
    const string& usernameInIni = r_resource->getPublicServerUserName();
    if (usernameInIni.length() > sizeof (m_loginData.username)
            || (r_resource->getPublicServerPassword().length() > sizeof (m_loginData.password)))
        throw runtime_error("Uploader(): PublicServerUserName or PublicServerPassword Illegal");
    m_id = "Uploader." + to_str(no);
    usernameInIni.copy((char*) m_loginData.username, sizeof (m_loginData.username));
    // 平台符合性检测：多链路的平台唯一码、密码与原链路相同，仅用户名为原用户名+“1”
    // 老赵：第no条辅链路，就加no个字符“1”
    for (int i = 0; i < no; i++) {
        if (sizeof (m_loginData.username) <= usernameInIni.length() + i)
            break;
        m_loginData.username[usernameInIni.length() + i] = '1';
    }
    r_resource->getPublicServerPassword().copy((char*) m_loginData.password, sizeof (m_loginData.password));
    m_loginData.encryptionAlgorithm = r_resource->getEncryptionAlgorithm();
    m_loginData.header.encryptionAlgorithm = m_loginData.encryptionAlgorithm;
    m_logoutData.header.encryptionAlgorithm = m_loginData.encryptionAlgorithm;

    m_loginData.header.cmdId = enumCmdCode::platformLogin;
    m_logoutData.header.cmdId = enumCmdCode::platformLogout;

    r_resource->getPaltformId().copy((char*) m_loginData.header.vin, VINLEN);
    r_resource->getPaltformId().copy((char*) m_logoutData.header.vin, VINLEN);

    uint16_t dataUnitLength = sizeof (LoginDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_loginData.header.dataUnitLength = boost::asio::detail::socket_ops::host_to_network_short(dataUnitLength);
    dataUnitLength = sizeof (LogoutDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_logoutData.header.dataUnitLength = boost::asio::detail::socket_ops::host_to_network_short(dataUnitLength);
}

Uploader::~Uploader() {
}

void Uploader::task() {
    boost::thread responseReaderThread(boost::bind(&ResponseReader::task, boost::ref(m_responseReader)));
    try {
        switch (m_mode) {
            case EnumRunMode::vehicleCompliance:
            case EnumRunMode::platformCompliance:
                setupConnection();
                for (int i = 0; i < 5; i++) {
                    setupConnAndLogin();
                    boost::this_thread::sleep(boost::posix_time::seconds(2));
                    logout();
                    boost::this_thread::sleep(boost::posix_time::seconds(2));
                }
                GINFO(m_id) << "login & logout 5 times done ";
                GDEBUG(m_id) << "login & logout 5 times done ";
                setupConnAndLogin();
                break;
            case EnumRunMode::release:
                //setupConnAndLogin();
                break;
            default:
                throw runtime_error("unrecognized run mode: " + to_str(m_mode));
        }
        for (;;) {
            // 平台符合性检测需要离线10min
            if (m_mode == EnumRunMode::platformCompliance && offline) {
                m_publicServer.close();
                isConnectWithPublicServer = false;
                GINFO(m_id) << "close session with public server, offline for 10 min...";
                GDEBUG(m_id) << "close session with public server, offline for 10 min...";
                boost::this_thread::sleep(boost::posix_time::minutes(10));
                offline = false;
            }
            if (!m_publicServer.isConnected()) {
                setupConnAndLogin();
            }
            try {
                m_carData = r_carDataQueue.take();
            } catch (boost::thread_interrupted&) {
                break;
            }
            if (!m_carData->hasRemaining()) {
                GWARNING("Uploader::run()") << "take an empty realtimeData";
                continue;
            }
            m_packetHdr = (DataPacketHeader_t*) m_carData->array();
            m_vin.assign((char*) m_packetHdr->vin, sizeof (m_packetHdr->vin));

            forwardCarData();
        }
        logout();
    } catch (exception &e) {
        GWARNING(m_id) << "exception: " << e.what();
    } catch (boost::thread_interrupted&) {
    }
    m_publicServer.close();
    isConnectWithPublicServer = false;
    responseReaderThread.interrupt();
    responseReaderThread.join();
    GINFO(m_id) << "quiting...";
}

void Uploader::setupConnection() {
    if (m_publicServer.isConnected()) {
        return;
    }
    m_publicServer.close();
    m_publicServer.connect();
    for (; !m_publicServer.isConnected(); boost::this_thread::sleep(boost::posix_time::seconds(r_resource->getReSetupPeroid()))) {
        GWARNING(m_id) << "connect refused by Public Server. Reconnecting...\ndata queue size: " << r_carDataQueue.remaining();
        m_publicServer.connect();
    }
    GINFO(m_id) << "connection with public platform established";
    isConnectWithPublicServer = true;
}

/*
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
        //        if (m_packetHdr->cmdId == enumCmdCode::realtimeUpload)
        //            m_packetHdr->cmdId = enumCmdCode::reissueUpload;
        r_carDataQueue.put(m_carData, true);
    }
}

/**
 * 未响应则内部一直重复登入，登入回应失败直接抛异常。返回说明登入成功。
 * 
 */
void Uploader::setupConnAndLogin() {
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

        if (m_serialNumber > r_resource->getMaxSerialNumber()
                || nowDay != timeTM->tm_mday
                || nowMon != timeTM->tm_mon
                || nowYear != timeTM->tm_year)
            m_serialNumber = 1;

        updateLoginData();

        tcpSendData(enumCmdCode::platformLogin);
        GINFO(m_id) << "waiting for public server's response...";
        switch (m_responseReader.waitNextStatus()) {
            case responsereaderstatus::EnumResponseReaderStatus::connectionClosed:
                isConnectWithPublicServer = false;
                break;
            case responsereaderstatus::EnumResponseReaderStatus::timeout:
            {
                size_t timeToSleep = r_resource->getLoginTimes() >= i ?
                        r_resource->getLoginIntervals() : r_resource->getLoginIntervals2();
                GINFO(m_id) << "read response " << r_resource->getReadResponseTimeOut()
                        << "s timeout when login, sleep " << timeToSleep << "s and resend";
                GWARNING(m_id) << "read response " << r_resource->getReadResponseTimeOut()
                        << "s timeout when login, sleep " << timeToSleep << "s and resend";

                boost::this_thread::sleep(boost::posix_time::seconds(timeToSleep));
                break;
            }
            case responsereaderstatus::EnumResponseReaderStatus::responseOk:
                resend = false;
                break;
            case responsereaderstatus::EnumResponseReaderStatus::responseNotOk:
                throw runtime_error("Uploader::setupConnAndLogin(): response not ok, response flag: " + to_str((int) m_responseReader.responseFlag()));
            case responsereaderstatus::EnumResponseReaderStatus::responseFormatErr:
                //                throw runtime_error("Uploader::setupConnAndLogin(): bad response format");
                GWARNING(m_id) << "bad response format when setupConnAndLogin";
                break;
            default:
                throw runtime_error("Uploader::setupConnAndLogin(): illegal ResponseReaderStatus code: " + to_str(m_responseReader.status()));
        }
    } while (resend);

    m_serialNumber++;
    m_lastloginTime = time(NULL);
    GINFO(m_id) << "platform login done";
}

void Uploader::logout() {
    updateLogoutData();
    tcpSendData(enumCmdCode::platformLogout);
    GINFO(m_id) << "waiting for public server's response...";
    if (m_responseReader.waitNextStatus() != responsereaderstatus::EnumResponseReaderStatus::responseOk) {
        throw runtime_error("response not ok when logout, response status: " + to_str(m_responseReader.status()));
    }

    GINFO(m_id) << "platform logout done";
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
    m_vin.assign(r_resource->getPaltformId());
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
    m_vin.assign(r_resource->getPaltformId());
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
                cmdTypeStr = Constant::cmdPlatformLoginStr;
                break;
            case enumCmdCode::platformLogout:
                dataToSend = boost::make_shared<ByteBuffer>((uint8_t*) & m_logoutData, sizeof (LogoutDataForward_t));
                m_logoutData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LogoutDataForward_t) - 3);
                collectTime = time(NULL);
                cmdTypeStr = Constant::cmdPlatformLogoutStr;
                break;
            case enumCmdCode::realtimeUpload:
                cmdTypeStr = Constant::cmdRealtimeUploadStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::reissueUpload:
                cmdTypeStr = Constant::cmdReissueUploadStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::vehicleLogin:
                cmdTypeStr = Constant::cmdVehicleLoginStr;
                dataToSend = m_carData;
                break;
            case enumCmdCode::vehicleLogout:
                cmdTypeStr = Constant::cmdVehicleLogoutStr;
                dataToSend = m_carData;
                break;
            default:
                throw runtime_error("[" + m_vin + "] Uploader::tcpSendData(): Illegal cmd: " + to_str(cmd));
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

        GINFO(m_vin) << sizeToSend << " bytes of " << cmdTypeStr << " data uploaded";

        outputMsg(r_resource->getSystem(), m_vin, collectTime, m_lastSendTime, dataToSend.get());
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        GWARNING("Uploader::tcpSendData") << "exception: " << e.what();
        m_uploaderStatus = uploaderstatus::EnumUploaderStatus::connectionClosed;
    }
}

void Uploader::outputMsg(const enumSystem& system, const string& vin,
        const time_t& collectTime, const time_t& sendTime, ByteBuffer* data/* = NULL*/) {
    if (data == NULL)
        return;

    string data_str;
    switch (system) {
        case enumSystem::oct:
            data_str = data->to_oct();
            break;
        case enumSystem::dec:
            data_str = data->to_dec();
            break;
        case enumSystem::bin:
        case enumSystem::hex:
            data_str = data->to_hex();
            break;
        default:
            throw runtime_error("unrecognized system enumeration: " + to_str(system));
    }
    GREPORT << setiosflags(ios::left)
            << setw(21) << setfill(' ') << vin
            << setw(23) << setfill(' ') << gutility::timeToStr(collectTime)
            << setw(23) << setfill(' ') << gutility::timeToStr(sendTime)
            << data_str;
}
