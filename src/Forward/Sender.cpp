/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Sender.cpp
 * Author: 10256
 * 
 * Created on 2017年3月8日, 下午7:43
 */

#include "Sender.h"
#include <stdexcept>
#include <iomanip>
#include <string.h>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/asio.hpp>
#include <bits/stl_map.h>
#include <bits/stl_pair.h>
#include <bits/basic_string.h>
#include "ByteBuffer.h"
#include "../Util.h"
#include "Resource.h"
#include "Constant.h"

#include <iostream>

using namespace bytebuf;
using namespace std;
using namespace gsocket;

Sender::Sender() :
m_serialNumber(1),
m_lastloginTime(0),
m_lastSendTime(0),
m_responseBuf(512),
m_senderStatus(senderstatus::EnumSenderStatus::responseOk),
s_carDataQueue(Resource::GetResource()->GetVehicleDataQueue()),
s_publicServer(Resource::GetResource()->GetPublicServer()),
m_vin(Constant::VinInital),
m_logger(Resource::GetResource()->GetLogger()) {
    if (Resource::GetResource()->GetPublicServerUserName().length() > sizeof (m_loginData.username)
            || (Resource::GetResource()->GetPublicServerPassword().length() > sizeof (m_loginData.password)))
        throw runtime_error("Sender(): PublicServerUserName or PublicServerPassword Illegal");

    Resource::GetResource()->GetPublicServerUserName().copy((char*) m_loginData.username, sizeof (m_loginData.username));
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

Sender::~Sender() {
}

// 平台运行后登入，不再登出，直到TCP连接中断。

void Sender::run() {
    try {
        setupConnAndLogin();
        for (;;) {
            m_carData = s_carDataQueue.take();
            if (!m_carData->hasRemaining()) {
                m_logger.warn("Sender::run()", "take an empty realtimeData");
                continue;
            }
            m_packetHdr = (DataPacketHeader_t*) m_carData->array();
            m_vin.assign((char*) m_packetHdr->vin, sizeof (m_packetHdr->vin));
//            m_logger.info(m_vin, "Sender get vehicle data from data queue");

            forwardCarData();
        }
        logout();
    } catch (exception &e) {
        m_logger.error("Sender exception");
        m_logger.errorStream << e.what() << std::endl;
    }
    s_publicServer.Close();
    m_logger.info("DONE", "Sender quiting...");
}

/*
 * 1. 平台连续5次登入登出
 * 2. 转发车辆连续5次登入登出
 * 3. 转发单一车辆行驶状态下的数据1小时
 * 4. 转发车辆充电状态数据1小时
 * 5. 转发车辆soc为100%数据5分钟
 * 6. 转发报警数据5分钟
 * 7. 车辆离线10分钟，转发车辆补发数据
 */
void Sender::runForCarCompliance() {

    try {
        setupConnection();

        for (int i = 0; i < 5; i++) {
            setupConnAndLogin(false);
            logout();
        }
        m_logger.info("Sender", "login logout 5 times done ");

        // 转发测试前先清空 dataQueue
        s_carDataQueue.clear();

        // 此时等待车机发送测试数据，开始转发测试
        for (;;) {
            m_logger.info("Sender", "waiting for forward data...");
            m_carData = s_carDataQueue.take();
            if (!m_carData->hasRemaining()) {
                m_logger.warn("Sender::runForCarCompliance()", "take an empty realtimeData");
                continue;
            }
            m_packetHdr = (DataPacketHeader_t*) m_carData->array();
            m_vin.assign((char*) m_packetHdr->vin, sizeof (m_packetHdr->vin));
            m_logger.info(m_vin, "Sender get vehicle data from data queue");

            tcpSendData(m_packetHdr->cmdId);
        }
    } catch (exception &e) {
        m_logger.error("Sender exception");
        m_logger.errorStream << e.what() << std::endl;        
    }
    s_publicServer.Close();
    m_logger.info("DONE", "Sender quiting...");    
}

void Sender::setupConnection() {
    if (s_publicServer.isConnected())
        return;

    s_publicServer.Close();
    s_publicServer.Connect();
    for (; !s_publicServer.isConnected(); sleep(Resource::GetResource()->GetReSetupPeroid())) {
        m_logger.warn("Sender", "connect refused by Public Server. Reconnecting...");
        s_publicServer.Connect();
    }
    m_logger.info("Sender", "connection with public platform established");
}

/*
 * Generator 在收到MQ然后将车机数据放入 dataQueue 时需要先判断网络是否里连接，若否设为补发。
 * 国标：重发本条实时信息（应该是调整后重发）。每隔1min重发，最多发送3次
 * 国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
 * 我的理解：由于平台无法做到实时调整，只能转发错误回应给车机，继续发下一条。所以不再重发本条实时信息，等车机调整后直接转发。
 * 
 * 国标：客户端平台达到重发次数后仍未收到应答应断开会话连接。
 * 我的理解：我平台每次转发车辆数据，但是上级平台未响应时，都重新登入再转发，所以不存在所谓“达到重发次数”
 * 国标2：如客户端平台未收到应答，平台重新登入
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输。
 * 我平台转发所有上级平台的错误应答，和车辆登入的成功应答。其他应答不转发，默认成功。
 */
void Sender::forwardCarData() {
    // 转发车辆数据未响应或连接断开，需重新登录，然后重发本条数据。
    bool resend;
    do {
        resend = false;
        if (!s_publicServer.isConnected()) {
            setupConnAndLogin();
        }
        m_senderStatus = senderstatus::EnumSenderStatus::init;
        m_packetHdr = (DataPacketHeader_t*) m_carData->array(); // Login 后 read response, m_packetHdr 指向回应包的头部，再次指向转发包的头部
        tcpSendData(m_packetHdr->cmdId);
        if (m_senderStatus == senderstatus::EnumSenderStatus::connectionClosed) {
            resend = true;
            continue;
        }
        readResponse(Resource::GetResource()->GetReadResponseTimeOut());

        switch (m_senderStatus) {
            case senderstatus::EnumSenderStatus::connectionClosed:
            case senderstatus::EnumSenderStatus::timeout:
                resend = true;
                break;
            case senderstatus::EnumSenderStatus::responseOk:
                m_logger.info(m_vin, "Sender forward car data response ok");
                break;
            case senderstatus::EnumSenderStatus::responseNotOk:
            {
                m_logger.info(m_vin);
                m_logger.infoStream << "Sender forward car data response not ok, response flag: " 
                        << (int) m_packetHdr->responseFlag << std::endl;
            }
            case senderstatus::EnumSenderStatus::carLoginOk:
            {
                Resource::ConnTable_t::iterator iter = Resource::GetResource()->GetVechicleConnTable().find(m_vin);
                if (iter == Resource::GetResource()->GetVechicleConnTable().end())
                    m_logger.warn(m_vin, "vin not existing in VechicleConnTable when Sneder try to forward response");
                else
                    iter->second->write(m_responseBuf);
                break;
            }
            case senderstatus::EnumSenderStatus::responseFormatErr:
                throw runtime_error("Sender::forwardCarData(): bad response format");
            default:
                m_stream.str("");
                m_stream << "Sender::forwardCarData(): unrecognize SenderStatus code: " << m_senderStatus;
                throw runtime_error(m_stream.str());
        }
    } while (resend);
}

/**
 * 未响应则内部一直重复登入，登入回应失败直接抛异常。返回说明登入成功。
 * 
 * @param needResponse
 */
void Sender::setupConnAndLogin(const bool& needResponse/* = true*/) {
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
        if (!s_publicServer.isConnected()) {
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
        m_logger.info("Sender", "waiting for public server's response...");
        readResponse(Resource::GetResource()->GetReadResponseTimeOut());

        switch (m_senderStatus) {
            case senderstatus::EnumSenderStatus::connectionClosed:
                setupConnection();
                break;
            case senderstatus::EnumSenderStatus::timeout:
            {
                size_t timeToSleep = Resource::GetResource()->GetLoginTimes() >= i ?
                        Resource::GetResource()->GetLoginIntervals() : Resource::GetResource()->GetLoginIntervals2();
                m_stream.str("");
                m_stream << "read response " << Resource::GetResource()->GetReadResponseTimeOut()
                        << "s timeout when login, sleep " << timeToSleep << "s and resend";
                m_logger.info("Sender", m_stream.str());
                m_logger.warn("Sender", m_stream.str());
                sleep(timeToSleep);
                break;
            }
            case senderstatus::EnumSenderStatus::responseOk:
                resend = false;
                break;
            case senderstatus::EnumSenderStatus::responseNotOk:
                m_stream.str("");
                m_stream << "Sender::setupConnAndLogin(): response not ok, response flag: " << (int) ((DataPacketHeader_t*) m_responseBuf.array())->responseFlag;
                throw runtime_error(m_stream.str());
            case senderstatus::EnumSenderStatus::responseFormatErr:
                throw runtime_error("Sender::setupConnAndLogin(): bad response format");
            default:
                m_stream.str("");
                m_stream << "Sender::setupConnAndLogin(): unrecognize SenderStatus code: " << m_senderStatus;
                throw runtime_error(m_stream.str());
        }
    } while (resend);

    m_serialNumber++;
    m_lastloginTime = time(NULL);
    m_logger.info("Sender", "platform login done");
}

void Sender::logout() {
    updateLogoutData();
    tcpSendData(enumCmdCode::platformLogout);
    m_logger.info("Sender", "platform logout done");
}

void Sender::updateLoginData() {
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

void Sender::updateLogoutData() {
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
 * 如果连接断开 m_senderStatus 设为 senderstatus::EnumSenderStatus::connectionClosed
 * 
 * @param cmd
 */
void Sender::tcpSendData(const uint8_t& cmd) {
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
                m_stream << m_vin << "] Sender::tcpSendData(): Illegal cmd: " << cmd;
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
        s_publicServer.Write(*dataToSend);
        // 为了打印日志，将pos指针退回
        dataToSend->movePosition(sizeToSend, true);
        m_lastSendTime = time(NULL);
        
        m_stream.str("");
        m_stream << sizeToSend << " bytes of " << cmdTypeStr << " data uploaded";
        m_logger.info(m_vin, m_stream.str());

        ofstream file;
        file.open("log/message.txt", ofstream::out | ofstream::binary);
        outputMsg(file, m_vin, collectTime, m_lastSendTime, dataToSend.get());
        file.close();
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        m_logger.warn("Sender::tcpSendData exception");
        m_logger.warnStream << e.what() << std::endl;
        m_senderStatus = senderstatus::EnumSenderStatus::connectionClosed;
    }
}

/*
 * 国标：当服务端发送应答时，变更应答标志，保留报文时间，并重新计算校验位，删除其他数据。
 * 理解为回应报文结构，头部变更应答标志，其余不变，数据单元只保留数据采集时间，其余删除，重新计算校验位
 * 国标2：车辆登入报文作为车辆上线时间节点存在，需收到成功应答后才能进行车辆实时报文的传输。
 * 企业平台转发上级平台的回应报文：车辆登入成功，所有的失败报文。
 * read server response
 */
void Sender::readResponse(const int& timeout) {
    TimeForward_t* responseTime;

    m_responseBuf.clear();
    uint16_t responseDataUnitLen;
    DataPacketHeader_t* packetHdrTmp = (DataPacketHeader_t*) m_responseBuf.array();
    try {
        s_publicServer.Read(m_responseBuf, 2, timeout * 1000);
        if (packetHdrTmp->startCode[0] != '#' || packetHdrTmp->startCode[1] != '#') {
            m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
            return;
        }

        s_publicServer.Read(m_responseBuf, sizeof (DataPacketHeader_t) - 2, timeout * 1000);
        m_vin.assign((char*) packetHdrTmp->vin, sizeof (packetHdrTmp->vin));
        responseDataUnitLen = boost::asio::detail::socket_ops::network_to_host_short(
                packetHdrTmp->dataUnitLength);

        if (responseDataUnitLen > m_responseBuf.remaining()) {
            m_logger.warn("Sender::readResponse");
            m_logger.warnStream << "Illegal responseHdr->responseDataUnitLength: " << responseDataUnitLen << std::endl;
            m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
            return;
        }
        s_publicServer.Read(m_responseBuf, responseDataUnitLen + 1, timeout * 1000000);
    } catch (SocketTimeoutException& e) {
        m_senderStatus = senderstatus::EnumSenderStatus::timeout;
        return;
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        m_logger.warn("Sender::readResponse");
        m_logger.warnStream << e.what() << std::endl;
        m_senderStatus = senderstatus::EnumSenderStatus::connectionClosed;
        return;
    }
    m_responseBuf.flip();

    uint8_t chechCode = Util::generateBlockCheckCharacter(m_responseBuf, 2, m_responseBuf.remaining() - 3);
    if (chechCode != m_responseBuf.get(m_responseBuf.limit() - 1)) {
        m_logger.warn(m_vin, "BCC in public server's response check fail");
        m_logger.warnStream << "Expected to be " << chechCode << ", actually is "
                << m_responseBuf.get(m_responseBuf.limit() - 1) << std::endl;
        m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
        return;
    }
    if (responseDataUnitLen != sizeof (TimeForward_t)) {
        m_logger.warn(m_vin);
        m_logger.warnStream << "responseDataUnitLength in public server's response expected to 6, actually is " 
                << responseDataUnitLen << std::endl;
        m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
        return;
    }
    m_packetHdr = packetHdrTmp;

    responseTime = (TimeForward_t*) (m_responseBuf.array() + sizeof (DataPacketHeader_t));

//    m_stream.str("");
//    m_stream << "response time: " << (int) responseTime->year << '-' << (int) responseTime->mon << '-' << (int) responseTime->mday << " "
//            << (int) responseTime->hour << ':' << (int) responseTime->min << ':' << (int) responseTime->sec;
//    Util::output(m_vin, m_stream.str());
//    m_stream.str("");
//    m_stream << "response encryptionAlgorithm: " << (int) packetHdrTmp->encryptionAlgorithm;
//    Util::output(m_vin, m_stream.str());

    if (packetHdrTmp->responseFlag == responseflag::enumResponseFlag::success)
        if (packetHdrTmp->cmdId == enumCmdCode::vehicleLogin)
            m_senderStatus = senderstatus::EnumSenderStatus::carLoginOk;
        else
            m_senderStatus = senderstatus::EnumSenderStatus::responseOk;
    else
        m_senderStatus = senderstatus::EnumSenderStatus::responseNotOk;
    m_responseBuf.rewind();
}

void Sender::outputMsg(ostream& out, const string& vin, const time_t& collectTime, const time_t& sendTime, const ByteBuffer* data/* = NULL*/) {
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