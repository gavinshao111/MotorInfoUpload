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
#include "ByteBuffer.h"
#include "../Util.h"
#include "GSocket.h"

using namespace bytebuf;
using namespace std;
using namespace gavinsocket;

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
m_senderStatus(senderstatus::EnumSenderStatus::responseOk) {
    if (s_staticResource.PublicServerUserName.length() != sizeof (m_loginData.username)
            || sizeof (m_loginData.password) != (s_staticResource.PublicServerPassword.length()))
        throw runtime_error("DataSender(): PublicServerUserName or PublicServerPassword Illegal");

    s_staticResource.PublicServerUserName.copy((char*) m_loginData.username, sizeof (m_loginData.username));
    s_staticResource.PublicServerPassword.copy((char*) m_loginData.password, sizeof (m_loginData.password));
    m_loginData.encryptionAlgorithm = s_staticResource.EncryptionAlgorithm;

    m_loginData.header.cmdId = enumCmdCode::platformLogin;
    m_logoutData.header.cmdId = enumCmdCode::platformLogout;

    s_staticResource.PaltformId.copy((char*) m_loginData.header.vin, VINLEN);
    s_staticResource.PaltformId.copy((char*) m_logoutData.header.vin, VINLEN);

    m_loginData.header.encryptionAlgorithm = s_staticResource.EncryptionAlgorithm;
    m_logoutData.header.encryptionAlgorithm = s_staticResource.EncryptionAlgorithm;

    m_loginData.header.dataUnitLength = sizeof (LoginDataForward_t) - sizeof (DataPacketHeader) - 1;
    m_logoutData.header.dataUnitLength = sizeof (LogoutDataForward_t) - sizeof (DataPacketHeader) - 1;
    Util::BigLittleEndianTransfer(&m_loginData.header.dataUnitLength, 2);
    Util::BigLittleEndianTransfer(&m_logoutData.header.dataUnitLength, 2);
    //    m_readBuf = ByteBuffer::allocate(512);
    m_responseBuf = boost::make_shared<ByteBuffer>(512);

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

//Sender::Sender(const Sender& orig) {
//}

Sender::~Sender() {
}

// 平台运行后登入，不再登出，直到TCP连接中断。

void Sender::run() {
    try {
        setupConnAndLogin();
        for (;;) {
            m_carData = s_carDataQueue.take();
            if (!m_carData->hasRemaining()) {
                cout << "[WARN] Sender::run(): take an empty realtimeData\n" << endl;
                continue;
            }
            string vin((char*) ((DataPacketHeader_t*) m_carData->array())->vin, VINLEN);
            cout << "[INFO] Sender: get vehicle signal data from " << vin << endl << endl;

            forwardCarData();
        }
        logout();
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    s_tcpConn.Close();
    cout << "[DONE] Sender quiting...\n" << endl;
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
        cout << "[INFO] Sender: login logout 5 times done." << endl;

        // 转发测试前先清空 dataQueue
        s_carDataQueue.clear();

        // 此时等待车机发送测试数据，开始转发测试
        for (;;) {
            cout << "[INFO] Sender: waiting for forward data..." << endl;
            m_carData = s_carDataQueue.take();
            if (!m_carData->hasRemaining()) {
                cout << "[WARN] Sender::runForCarCompliance(): take an empty realtimeData" << endl;
                continue;
            }
            string vin((char*) ((DataPacketHeader_t*) m_carData->array())->vin, VINLEN);
            cout << "[INFO] Sender: get vehicle signal data from " << vin << endl << endl;

            tcpSendData(enumCmdCode::vehicleSignalDataUpload);
        }
    } catch (exception &e) {
        cout << "ERROR: Exception in " << __FILE__;
        cout << " (" << __func__ << ") on line " << __LINE__ << endl;
        cout << "ERROR: " << e.what() << endl;
    }
    s_tcpConn.Close();
    cout << "[DONE] Sender quiting...\n" << endl;
}

void Sender::setupConnection() {
    if (s_tcpConn.isConnected())
        return;
    
    s_tcpConn.Close();
    for (; !s_tcpConn.Connect(); sleep(s_staticResource.ReSetupPeroid))
        cout << "[WARN] Sender::setupConnection(): Connect refused by Public Server. Reconnecting...\n" << endl;
    cout << "[INFO] Sender: Connection with public platform established\n" << endl;
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
        if (!s_tcpConn.isConnected()) {
            setupConnAndLogin();
        }
        m_senderStatus = senderstatus::EnumSenderStatus::init;
        tcpSendData(enumCmdCode::vehicleSignalDataUpload);
        if (m_senderStatus == senderstatus::EnumSenderStatus::connectionClosed) {
            resend = true;
            continue;
        }
        readResponse(s_staticResource.ReadResponseTimeOut);

        switch (m_senderStatus) {
            case senderstatus::EnumSenderStatus::connectionClosed:
            case senderstatus::EnumSenderStatus::timeout:
                resend = true;
                break;
            case senderstatus::EnumSenderStatus::responseOk:
                break;
            case senderstatus::EnumSenderStatus::responseNotOk:
                cout << "[WARN] Sender::forwardCarData(): forward car data response not ok, response flag: " << (int) ((DataPacketHeader_t*) m_responseBuf->array())->responseFlag << endl << endl;
                //            outputMsg(cout, m_carData->m_vin, m_carData->getCollectTime(), m_lastSendTime);
            case senderstatus::EnumSenderStatus::carLoginOk: {
                string vin((char*) ((DataPacketHeader_t*) m_responseBuf->array())->vin, VINLEN);
                cout << "[DEBUG] Sender: put " << vin << " response packet(" 
                        << m_responseBuf->remaining() << " bytes) into responseDataQueue\n" << endl;
                s_responseDataQueue.put(m_responseBuf);
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
#if DEBUG
    cout << "[DEBUF] Sender: forwardCarData done\n" << endl;
#endif
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
        //    for (size_t i = 1;; i++) {
        if (!s_tcpConn.isConnected()) {
            setupConnection();
        }

        now = time(NULL);
        timeTM = localtime(&now);
        int nowDay = timeTM->tm_mday;
        int nowMon = timeTM->tm_mon;
        int nowYear = timeTM->tm_year;
        localtime(&m_lastloginTime);

        if (m_serialNumber > s_staticResource.MaxSerialNumber
                || nowDay != timeTM->tm_mday
                || nowMon != timeTM->tm_mon
                || nowYear != timeTM->tm_year)
            m_serialNumber = 1;

        updateLoginData();

        tcpSendData(enumCmdCode::platformLogin);
        if (!needResponse)
            break;
        cout << "[DEBUG] Sender: waiting for public server's response...\n" <<  endl;
        readResponse(s_staticResource.ReadResponseTimeOut);

        switch (m_senderStatus) {
            case senderstatus::EnumSenderStatus::connectionClosed:
                setupConnection();
                break;
            case senderstatus::EnumSenderStatus::timeout: {
                size_t timeToSleep = s_staticResource.LoginTimes >= i ? s_staticResource.LoginIntervals : s_staticResource.LoginIntervals2;
                cout << "[WARN] Sender::login(): read response " << s_staticResource.ReadResponseTimeOut 
                        << "s timeout, sleep " << timeToSleep << "s and resend\n" <<endl;
                sleep(timeToSleep);
                break;
            }
            case senderstatus::EnumSenderStatus::responseOk:
                resend = false;
                break;
            case senderstatus::EnumSenderStatus::responseNotOk:
                m_stream.str("");
                m_stream << "Sender::setupConnAndLogin(): response not ok, response flag: " << (int) ((DataPacketHeader_t*) m_responseBuf->array())->responseFlag;
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
#if DEBUG
    cout << "[INFO] Sender: login done\n" << endl;
#endif
}

void Sender::logout() {
    updateLogoutData();
    tcpSendData(enumCmdCode::platformLogout);
#if DEBUG
    cout << "[INFO] Sender: logout done\n" << endl;
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

/**
 * 根据指令发送 1.平台登入 2.平台登出 3.转发车机数据 并将发送数据写入日志
 * 如果连接断开 m_senderStatus 设为 senderstatus::EnumSenderStatus::connectionClosed
 * 
 * @param cmd
 */
void Sender::tcpSendData(const enumCmdCode & cmd) {
    size_t sizeToSend;
    BytebufSPtr_t dataToSend;
    time_t collectTime;
    try {
        switch (cmd) {
            case enumCmdCode::platformLogin:
                dataToSend = boost::make_shared<ByteBuffer>((uint8_t*) & m_loginData, sizeof (LoginDataForward_t));
                m_loginData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LoginDataForward_t) - 3);
                collectTime = time(NULL);
                break;
            case enumCmdCode::platformLogout:
                dataToSend = boost::make_shared<ByteBuffer>((uint8_t*) & m_logoutData, sizeof (LogoutDataForward_t));
                m_logoutData.checkCode = Util::generateBlockCheckCharacter(*dataToSend, 2, sizeof (LogoutDataForward_t) - 3);
                collectTime = time(NULL);
                break;
            case enumCmdCode::vehicleSignalDataUpload:
                dataToSend = m_carData;
                break;
            default:
                m_stream.str("");
                m_stream << "Sender::tcpSendData(): Illegal cmd: " << cmd;
                throw runtime_error(m_stream.str());
        }

        LogoutDataForward_t* pData = (LogoutDataForward_t*) dataToSend->array();
        string vin((char*) pData->header.vin, VINLEN);

        struct tm timeTM;
        timeTM.tm_year = pData->time.year + 100;
        timeTM.tm_mon = pData->time.mon - 1;
        timeTM.tm_mday = pData->time.mday;
        timeTM.tm_hour = pData->time.hour;
        timeTM.tm_min = pData->time.min;
        timeTM.tm_sec = pData->time.sec;
        collectTime = mktime(&timeTM);

        sizeToSend = dataToSend->remaining();
        s_tcpConn.Write(*dataToSend);
        // 为了打印日志，将pos指针退回
        dataToSend->movePosition(sizeToSend, true);
        m_lastSendTime = time(NULL);
        cout << "[INFO] Sender: " << sizeToSend << " bytes sent.\n" << endl;

        ofstream file;
        file.open("log/message.txt", ofstream::out | ofstream::app | ofstream::binary);
        outputMsg(file, vin, collectTime, m_lastSendTime, dataToSend.get());
        file.close();
        cout << "[DEBUG] Sender: sent data write to log ... done\n" << endl;
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

    m_responseBuf->clear();
    DataPacketHeader_t* responseHdr = (DataPacketHeader_t*) m_responseBuf->array();
    try {
        if (!s_tcpConn.Read(*m_responseBuf, 2, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }
        if (responseHdr->startCode[0] != '#' || responseHdr->startCode[1] != '#') {
            m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
            return;
            //            m_stream.str("");
            //            m_stream << "Sender::readResponse(): responseFormatIncorrect: startCode[0]: "
            //                    << (int) responseHdr->startCode[0] << "startCode[1]: " << (int) responseHdr->startCode[1];
            //            throw runtime_error(m_stream.str());
        }

        if (!s_tcpConn.Read(*m_responseBuf, sizeof (DataPacketHeader_t) - 2, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }

        Util::BigLittleEndianTransfer(&responseHdr->dataUnitLength, 2);

        if (responseHdr->dataUnitLength > m_responseBuf->remaining()) {
            //            m_stream.str("");
            //            m_stream << "Sender::readResponse(): Illegal responseHdr->dataUnitLength: " << responseHdr->dataUnitLength;
            //            throw runtime_error(m_stream.str());
            cout << "[WARN] Sender::readResponse(): Illegal responseHdr->dataUnitLength: " << responseHdr->dataUnitLength;
            m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
            return;
        }
        if (!s_tcpConn.Read(*m_responseBuf, responseHdr->dataUnitLength + 1, timeout * 1000000)) {
            m_senderStatus = senderstatus::EnumSenderStatus::timeout;
            return;
        }

    } catch (SocketException& e) { // 只捕获连接被关闭的异常，其他异常正常抛出
        cout << "[WARN] Sender::readResponse():\n" << e.what() << endl;
        m_senderStatus = senderstatus::EnumSenderStatus::connectionClosed;
        return;
    }
    m_responseBuf->flip();

    uint8_t chechCode = Util::generateBlockCheckCharacter(*m_responseBuf, 2, m_responseBuf->remaining() - 3);
    if (chechCode != m_responseBuf->get(m_responseBuf->limit() - 1)) {
        cout << "[WARN] Sender::readResponse(): BCC in public server's response check fail" << endl;
        cout << "Expected to be " << chechCode << ", actually is " << m_responseBuf->get(m_responseBuf->limit() - 1) << endl;
        m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
        return;
    }
    if (responseHdr->dataUnitLength != sizeof (TimeForward_t)) {
        cout << "[WARN] Sender::readResponse(): dataUnitLength in public server's response expected to 6, actually is " << responseHdr->dataUnitLength << endl;
        m_senderStatus = senderstatus::EnumSenderStatus::responseFormatErr;
        return;
    }

    responseTime = (TimeForward_t*) (m_responseBuf->array() + sizeof (DataPacketHeader_t));
    //        m_readBuf.movePosition(sizeof(DataPacketHeader_t));
    cout << "Sender: response time: " << (int) responseTime->year << '-' << (int) responseTime->mon << '-' << (int) responseTime->mday << " "
            << (int) responseTime->hour << ':' << (int) responseTime->min << ':' << (int) responseTime->sec << endl;

    string vin((char*) responseHdr->vin, VINLEN);
    cout << "Sender: response vin: " << vin << endl;
    cout << "Sender: response encryptionAlgorithm: " << (int) responseHdr->encryptionAlgorithm << endl;

    if (responseHdr->responseFlag == responseflag::enumResponseFlag::success)
        if (responseHdr->cmdId == enumCmdCode::vehicleLogin)
            m_senderStatus = senderstatus::EnumSenderStatus::carLoginOk;
        else
            m_senderStatus = senderstatus::EnumSenderStatus::responseOk;
    else
        m_senderStatus = senderstatus::EnumSenderStatus::responseNotOk;
    m_responseBuf->position(0);

    cout << endl;
}

void Sender::outputMsg(ostream& out, const string& vin, const time_t& collectTime, const time_t& sendTime, const ByteBuffer* data/* = NULL*/) {
    out << setiosflags(ios::left)
            << setw(21) << setfill(' ') << vin
            << setw(23) << setfill(' ') << Util::timeToStr(collectTime)
            << setw(23) << setfill(' ') << Util::timeToStr(sendTime);
    if (data != NULL)
        data->outputAsDec(out);
    out << endl;
    out.flush();
}