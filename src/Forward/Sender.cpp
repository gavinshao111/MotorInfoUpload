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
#include "ByteBuffer.h"
#include "../Util.h"
#include "GSocket.h"

using namespace bytebuf;
using namespace std;
using namespace gavinsocket;

extern bool exitNow;

Sender::Sender(StaticResourceForward& staticResource, DataQueue_t& CarDataQueue, DataQueue_t& ResponseDataQueue, TcpConn_t& tcpConnection) :
s_staticResource(staticResource),
s_carDataQueue(CarDataQueue),
s_responseDataQueue(ResponseDataQueue),
s_tcpConn(tcpConnection),
m_serialNumber(1),
m_reissueNum(0),
m_lastloginTime(0),
m_lastSendTime(0),
m_setupTcpInitial(true),
m_senderStatus(senderstatus::EnumSenderStatus::ok),
m_responseHdr(NULL) {
    if (m_staticResource->PublicServerUserName.length() != sizeof (m_loginData.username)
            || sizeof (m_loginData.password) != (m_staticResource->PublicServerPassword.length()))
        throw runtime_error("DataSender(): PublicServerUserName or PublicServerPassword Illegal");

    m_staticResource->PublicServerUserName.copy((char*) m_loginData.username, sizeof (m_loginData.username));
    m_staticResource->PublicServerPassword.copy((char*) m_loginData.password, sizeof (m_loginData.password));
    m_loginData.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;

    m_loginData.header.CmdId = 5;
    m_logoutData.header.CmdId = 6;

    m_staticResource->PaltformId.copy((char*) m_loginData.header.vin, VINLEN);
    m_staticResource->PaltformId.copy((char*) m_logoutData.header.vin, VINLEN);

    m_loginData.header.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;
    m_logoutData.header.encryptionAlgorithm = m_staticResource->EncryptionAlgorithm;

    m_loginData.header.dataUnitLength = sizeof (LoginDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_logoutData.header.dataUnitLength = sizeof (LogoutDataForward_t) - sizeof (DataPacketHeader) - 1;
    Util::BigLittleEndianTransfer(&m_loginData.header.dataUnitLength, 2);
    Util::BigLittleEndianTransfer(&m_logoutData.header.dataUnitLength, 2);
    m_readBuf = ByteBuffer::allocate(512);

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

Sender::Sender(const Sender& orig) {
}

Sender::~Sender() {
    m_readBuf->freeMemery();
    delete m_readBuf;
}

// 平台运行后登入，不再登出，直到TCP连接中断。

void Sender::run() {
    if (NULL == m_staticResource->dataQueue) {
        cout << "DataQueue is NULL" << endl;
        return;
    }
    try {

        //        DataPacketForward* carData;
        for (; !exitNow;) {
            if (!m_carData.get()) {
                m_carData = m_staticResource->dataQueue->take();
                if (!m_carData->hasRemaining()) {
                    delete m_carData;
                    m_carData = NULL;
                    cout << "Sender::run(): take an empty realtimeData" << endl;
                    continue;
                }
                //                m_packetForwardInfo.putData(carData);
                cout << "\n\nSender::run(): get realtimeData from " << m_carData->m_vin << endl;
            }

            sendData();
        }
        logout();
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    m_staticResource->tcpConnection->Close();
    cout << "Sender quiting..." << endl;
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
    if (NULL == m_staticResource->dataQueue) {
        cout << "DataQueue is NULL" << endl;
        return;
    }
    try {
        setupConnection();

        for (int i = 0; i < 5 && !exitNow; i++) {
            login(false);
            logout();
        }
        cout << "login logout 5 times done." << endl;

        // 转发测试前先清空 dataQueue
        m_staticResource->dataQueue->clearAndFreeElements();

        // 此时等待车机发送测试数据，开始转发测试
        //        DataPacketForward* carData;
        for (; !exitNow;) {
            if (NULL == m_carData) {
                cout << "Sender is wait for forward data..." << endl;
                m_carData = m_staticResource->dataQueue->take();
                if (NULL == m_carData || !m_carData->m_dataBuf->hasRemaining()) {
                    delete m_carData;
                    m_carData = NULL;
                    cout << "Sender::runForCarCompliance(): take an empty realtimeData" << endl;
                    continue;
                }
                //                m_packetForwardInfo.putData(carData);
                cout << "\n\nSender::run(): get realtimeData from " << m_carData->m_vin << endl;
            }

            forwardCarData(false);
            delete m_carData;
            m_carData = NULL;
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    m_staticResource->tcpConnection->Close();
    cout << "Sender quiting..." << endl;
}

void Sender::setupConnection() {
    for (; !exitNow && !m_staticResource->tcpConnection->Connect(); sleep(m_staticResource->ReSetupPeroid))
        cout << "[WARN] Sender::setupConnection(): Connect refused by Public Server. Reconnecting..." << endl;
    cout << "[DEBUG] Sender::setupConnection(): Connected" << endl;
}

/*
 * Generator 在收到MQ然后将车机数据放入 dataQueue 时需要先判断网络是否里连接，若否设为补发。
 * 国标：实时数据校验失败每隔1min（RealtimeDataResendIntervals）重发，失败3(MaxResendCarDataTimes)次后不再发送。
 * 国标2：如客户端平台收到应答错误，应及时与服务端平台进行沟通，对登入信息进行调整。
 * 我的理解：失败后不再发送这个包，输出错误信息提示。继续发下一个包。
 * 国标：客户端平台达到重发次数后仍未收到应答应断开会话连接。
 * 国标2：如客户端平台未收到应答，平台重新登入
 */
void Sender::sendData() {
    for (;;) {
        // need relogin after tcp broken down.
        if (!m_staticResource->tcpConnection->isConnected()) {
            setupConnection();
            // 未响应则内部一直重复登入，登入回应失败直接抛异常。返回说明登入成功。
            login();
        }
        forwardCarData();
        if (senderstatus::EnumSenderStatus::error == m_senderStatus)
            ; // GSocket Read 或 Write 异常，内部不捕获(除了连接被关闭)，任其往上抛 throw exception already
        else if (senderstatus::EnumSenderStatus::connectionClosed == m_senderStatus)
            ;
        else if (senderstatus::EnumSenderStatus::ok == m_senderStatus) {
            delete m_carData;
            m_carData = NULL;
            break;
        } else if (senderstatus::EnumSenderStatus::notOk == m_senderStatus) { // 按规定应调整后重发，由于无法做到实时调整，只能忽略错误的那条实时信息，继续发下一条。
            cout << "[WARN] Sender::sendData(): forward car data response not ok, response flag: " << (int) m_responseHdr->responseFlag
                    << "\nPlease check log. packet info:" << endl;
            outputMsg(cout, m_carData->m_vin, m_carData->getCollectTime(), m_lastSendTime);
            delete m_carData;
            m_carData = NULL;
            break;
        } else if (senderstatus::EnumSenderStatus::timeout == m_senderStatus)
            cout << "[WARN] Sender::sendData(): forward car data timeout" << endl;
        else {
            m_stream.clear();
            m_stream << "Sender::sendData(): unrecognize SenderStatus Code: " << m_senderStatus;
            throw runtime_error(m_stream.str());
        }
    }
}

// 未响应则内部一直重复登入，登入回应失败直接抛异常。返回说明登入成功。

void Sender::login(const bool& needResponse/* = true*/) {
    time_t now;
    struct tm* timeTM;

    /* 
     * 登录没有回应每隔1min（LoginTimeout）重新发送登入数据。
     * 重复resendLoginTime次没有回应，每隔30min（loginTimeout2）重新发送登入数据。
     */
    for (size_t i = 1;; i++) {
        now = time(NULL);
        timeTM = localtime(&now);
        int nowDay = timeTM->tm_mday;
        int nowMon = timeTM->tm_mon;
        int nowYear = timeTM->tm_year;
        localtime(&m_lastloginTime);

        if (m_serialNumber > m_staticResource->MaxSerialNumber
                || nowDay != timeTM->tm_mday
                || nowMon != timeTM->tm_mon
                || nowYear != timeTM->tm_year)
            m_serialNumber = 1;

        updateLoginData();

        tcpSendData(enumCmdCode::platformLogin);
        if (!needResponse)
            break;
        readResponse(m_staticResource->ReadResponseTimeOut);
        if (senderstatus::EnumSenderStatus::ok == m_senderStatus)
            break;
        else if (senderstatus::EnumSenderStatus::timeout == m_senderStatus) {
            if (m_staticResource->LoginTimes >= i)
                sleep(m_staticResource->LoginIntervals2);
            else
                sleep(m_staticResource->LoginIntervals);
        } else if (senderstatus::EnumSenderStatus::connectionClosed == m_senderStatus)
            ;
        else {
            m_stream.clear();
            m_stream << "Sender::login(): status code: " << m_senderStatus;
            throw runtime_error(m_stream.str());
        }
#if DEBUG
        cout << "[WARN] Sender::login(): login timeout, relogin " << i << " times..." << endl;
#endif
    }
    m_serialNumber++;
    m_lastloginTime = time(NULL);
#if DEBUG
    cout << "login done" << endl;
#endif
}

void Sender::logout() {
    updateLogoutData();
    tcpSendData(enumCmdCode::platformLogout);
#if DEBUG
    cout << "logout done" << endl;
#endif
}

void Sender::forwardCarData(const bool& needResponse/* = true*/) {
    tcpSendData(enumCmdCode::carSignalDataUpload);
    if (needResponse)
        readResponse(m_staticResource->ReadResponseTimeOut);

#if DEBUG
    cout << "send car signal data done" << endl;
#endif
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
    Util::BigLittleEndianTransfer(&m_loginData.serialNumber, 2);
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

void Sender::tcpSendData(const enumCmdCode & cmd) {
    size_t sizeToSend;
    ByteBuffer* dataToSend;
    string vin;
    time_t collectTime;
    try {
        switch (cmd) {
            case enumCmdCode::platformLogin:
                dataToSend = ByteBuffer::wrap((uint8_t*) & m_loginData, sizeof (LoginDataForward_t));
                m_loginData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LoginDataForward_t) - 3);
                collectTime = time(NULL);
                vin.append((char*) m_loginData.header.vin, VINLEN);
                break;
            case enumCmdCode::platformLogout:
                dataToSend = ByteBuffer::wrap((uint8_t*) & m_logoutData, sizeof (LogoutDataForward_t));
                m_logoutData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LogoutDataForward_t) - 3);
                collectTime = time(NULL);
                vin.append((char*) m_logoutData.header.vin, VINLEN);
                break;
            case enumCmdCode::carSignalDataUpload:
                dataToSend = m_carData->m_dataBuf;
                collectTime = m_carData->getCollectTime();
                vin = m_carData->m_vin;
                break;
            default:
                m_stream.clear();
                m_stream << "Illegal cmd: " << cmd;
                throw runtime_error(m_stream.str());
        }

        sizeToSend = dataToSend->remaining();
        m_staticResource->tcpConnection->Write(dataToSend);
        dataToSend->movePosition(sizeToSend, true);
        m_lastSendTime = time(NULL);
        cout << "Sender::tcpSendData(): " << sizeToSend << " bytes sent." << endl;

        ofstream file;
        file.open("log/message.txt", ofstream::out | ofstream::app | ofstream::binary);
        outputMsg(file, vin, collectTime, m_lastSendTime, dataToSend);
        file.close();
    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        cout << "[WARN] Sender::tcpSendData():\n" << e.what() << endl;
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

    m_readBuf->clear();
    m_responseHdr = (DataPacketHeader_t*) m_readBuf->array();
    try {
        if (!m_staticResource->tcpConnection->Read(m_readBuf, 2, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }
        if (m_responseHdr->startCode[0] != '#' || m_responseHdr->startCode[1] != '#') {
            m_stream.clear();
            m_stream << "Sender::readResponse(): responseFormatIncorrect: startCode[0]: "
                    << (int) m_responseHdr->startCode[0] << "startCode[1]: " << (int) m_responseHdr->startCode[1];
            throw runtime_error(m_stream.str());
        }

        if (!m_staticResource->tcpConnection->Read(m_readBuf, sizeof (DataPacketHeader_t) - 2, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }

        Util::BigLittleEndianTransfer(&m_responseHdr->dataUnitLength, 2);

        if (m_responseHdr->dataUnitLength > m_readBuf->remaining()) {
            m_stream.clear();
            m_stream << "Sender::readResponse(): Illegal responseHdr->dataUnitLength: " << m_responseHdr->dataUnitLength;
            throw runtime_error(m_stream.str());
        }
        if (!m_staticResource->tcpConnection->Read(m_readBuf, m_responseHdr->dataUnitLength + 1, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }

    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        cout << "[WARN] Sender::readResponse():\n" << e.what() << endl;
        m_senderStatus = senderstatus::EnumSenderStatus::connectionClosed;
        return;
    }
    m_readBuf->flip();

    uint8_t chechCode = Util::generateBlockCheckCharacter(*m_readBuf, 2, m_readBuf->remaining() - 3);
    if (chechCode != m_readBuf->get(m_readBuf->remaining()))
        cout << "[WARN] Sender::readResponse(): BCC in public server's response check fail" << endl;
    if (m_responseHdr->dataUnitLength != sizeof (TimeForward_t))
        cout << "[WARN] Sender::readResponse(): dataUnitLength in public server's response expected to 6, actually is " << m_responseHdr->dataUnitLength << endl;
    else {
        responseTime = (TimeForward_t*) (m_readBuf->array() + sizeof (DataPacketHeader_t));
        //        m_readBuf->movePosition(sizeof(DataPacketHeader_t));
        cout << "Sender::readResponse(): time: " << (int) responseTime->year << '-' << (int) responseTime->mon << '-' << (int) responseTime->mday << " "
                << (int) responseTime->hour << ':' << (int) responseTime->min << ':' << (int) responseTime->sec << endl;
    }

    string vin((char*) m_responseHdr->vin, VINLEN);
    cout << "Sender::readResponse(): responseHdr->vin: " << vin << endl;
    cout << "Sender::readResponse(): encryptionAlgorithm: " << (int) m_responseHdr->encryptionAlgorithm << endl;

    if (m_responseHdr->responseFlag == responseflag::enumResponseFlag::success)
        m_senderStatus = senderstatus::EnumSenderStatus::ok;
    else
        m_senderStatus = senderstatus::EnumSenderStatus::notOk;
}

void Sender::outputMsg(ostream& out, const string& vin, const time_t& collectTime, const time_t& sendTime, const ByteBuffer* data/* = NULL*/) {
    out << setiosflags(ios::left)
            << setw(21) << setfill(' ') << vin
            << setw(23) << setfill(' ') << Util::timeToStr(collectTime)
            << setw(23) << setfill(' ') << Util::timeToStr(sendTime);
    if (NULL != data)
        data->outputAsDec(out);
    out << endl;
    out.flush();
}

//PacketForwardInfo::PacketForwardInfo() {
//}
//
//PacketForwardInfo::PacketForwardInfo(const PacketForwardInfo& orig) {
//}
//
//PacketForwardInfo::~PacketForwardInfo() {
//}
//
//bool PacketForwardInfo::isEmpty() {
//    return NULL == m_carData;
//}
//
//void PacketForwardInfo::putData(DataPacketForward * carData) {
//    m_carData = carData;
//}
//
//void PacketForwardInfo::clear() {
//    if (!isEmpty())
//        delete m_carData;
//    m_carData = NULL;
//}
//
//string PacketForwardInfo::BaseInfoToString() {
//    m_stringstream.clear();
//    m_stringstream << m_carData->m_vin << ' ' << Util::timeToStr(m_carData->getCollectTime()) << ' ' << Util::timeToStr(m_sendTime);
//    return m_stringstream.str();
//}
//
//void PacketForwardInfo::output(ostream& out) {
//    out << setiosflags(ios::left)
//            << setw(21) << setfill(' ') << m_carData->m_vin
//            << setw(21) << setfill(' ') << Util::timeToStr(m_carData->getCollectTime())
//            << setw(21) << setfill(' ') << Util::timeToStr(m_sendTime)
//            << setw(21) << setfill(' ');
//    m_carData->m_dataBuf->outputAsDec(out);
//    out << endl;
//}
//
//void PacketForwardInfo::sendTime(const time_t & time) {
//    m_sendTime = time;
//}
//
//string & PacketForwardInfo::vin() {
//    return m_carData->m_vin;
//}
//
//DataPacketForward* PacketForwardInfo::getData() const {
//    return m_carData;
//}