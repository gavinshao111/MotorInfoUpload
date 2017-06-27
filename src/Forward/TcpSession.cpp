/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TcpSession.cpp
 * Author: 10256
 * 
 * Created on 2017年5月16日, 下午5:12
 */

#include "TcpSession.h"

#include <iostream>
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <stdexcept>
#include <string>
#include <sstream>
#include <bits/stl_map.h>
#include <bits/basic_string.h>

#include "../Util.h"
#include "Resource.h"

using namespace boost::asio;

const static int lengthOfPacketLength = 2;
const static std::string VinInital = "Vin hasn't established";

TcpSession::TcpSession(io_service& ioservice) :
m_heartBeatCycle(Resource::GetResource()->GetHeartBeatCycle()),
m_socket(ioservice),
m_timer(ioservice),
m_quit(false),
m_vinStr(VinInital) {
}

TcpSession::~TcpSession() {
    // deadline_timer的析构函数什么也不做，因此不会导致发出的async_wait被cancel。
    m_timer.cancel();
    std::cout << m_vinStr << ": TcpSession destructed" << std::endl;
}

ip::tcp::socket& TcpSession::socket() {
    return m_socket;
}

void TcpSession::readHeader() {
    m_hdr = NULL;
    m_packetRef = boost::make_shared<bytebuf::ByteBuffer>(MAXPACK_LENGTH);

    m_timer.expires_from_now(boost::posix_time::seconds(m_heartBeatCycle));
    m_timer.async_wait(boost::bind(&TcpSession::readTimeoutHandler,
            shared_from_this(),
            placeholders::error));

    async_read(m_socket, buffer(m_packetRef->array(), sizeof (DataPacketHeader)),
            boost::bind(&TcpSession::readHeaderHandler,
            shared_from_this(),
            placeholders::error,
            placeholders::bytes_transferred));
}

void TcpSession::readHeaderHandler(const boost::system::error_code& error, size_t bytes_transferred) {
    // 如果此时对方关闭socket，error.value = 2, message = End of file
    m_timer.cancel();
    if (error) {
        if (m_vinStr.compare(VinInital) != 0) {
            int n = Resource::GetResource()->GetVechicleConnTable().erase(m_vinStr);
            std::cout << "TcpSession::readHeaderHandler erase " << m_vinStr << ": " << n << std::endl;
        }

        if (error == error::operation_aborted) {
            std::cout << "read operation canceled may because time out" << std::endl;
        } else {
            Util::output("TcpSession", "readHeaderHandler error: ", error.message());
            std::cout << "error code: " << error.value() << std::endl;
            std::unique_lock<std::mutex> lk(Resource::GetResource()->GetTableMutex());
        }
        return;
    }
    assert(bytes_transferred == sizeof (DataPacketHeader));
    m_packetRef->position(bytes_transferred);

    m_hdr = (DataPacketHeader_t*) m_packetRef->array();
    std::cout << "[TcpSession::readHeaderHandler] m_hdr->cmdId: " << (int) m_hdr->cmdId << std::endl;
    if (m_vinStr.compare(VinInital) == 0)
        m_vinStr.assign((char*) m_hdr->vin, sizeof (m_hdr->vin));

    readDataUnit();
}

void TcpSession::readDataUnit() {
    m_dataUnitLen = boost::asio::detail::socket_ops::network_to_host_short(m_hdr->dataUnitLength);
    std::cout << "get dataUnitLen: " << m_dataUnitLen << std::endl;
    //    m_packetRef = boost::make_shared<bytebuf::ByteBuffer>(sizeof(DataPacketHeader) + m_dataUnitLen + 1);

    m_timer.expires_from_now(boost::posix_time::seconds(m_heartBeatCycle));
    m_timer.async_wait(boost::bind(&TcpSession::readTimeoutHandler,
            shared_from_this(),
            placeholders::error));

    async_read(m_socket, buffer(m_packetRef->array() + m_packetRef->position(), m_dataUnitLen + 1),
            boost::bind(&TcpSession::readDataUnitHandler,
            shared_from_this(),
            placeholders::error,
            placeholders::bytes_transferred));
}

void TcpSession::readDataUnitHandler(const boost::system::error_code& error, size_t bytes_transferred) {
    m_timer.cancel();
    if (error) {
        if (m_vinStr.compare(VinInital) != 0) {
            int n = Resource::GetResource()->GetVechicleConnTable().erase(m_vinStr);
            std::cout << "TcpSession::readDataUnitHandler erase " << m_vinStr << ": " << n << std::endl;
        }

        if (error == error::operation_aborted) {
            std::cout << "read operation canceled may because time out" << std::endl;
        } else {
            Util::output("TcpSession", "readDataUnitHandler error: ", error.message());
            std::cout << "error code: " << error.value() << std::endl;
            std::unique_lock<std::mutex> lk(Resource::GetResource()->GetTableMutex());
        }
        return;
    }
    assert(bytes_transferred == m_dataUnitLen + 1);
    m_packetRef->limit(m_packetRef->position() + bytes_transferred);
    try {
        parseDataUnit();
    } catch (std::runtime_error& e) {
        Util::output("TcpSession", "dealData error: ", e.what());
    }
    if (!m_quit)
        readHeader();
}

void TcpSession::write(const bytebuf::ByteBuffer& src) {
    write(src, 0, src.remaining());
}

void TcpSession::write(const bytebuf::ByteBuffer& src, const size_t& offset, const size_t& length) {
    if (src.remaining() < length + offset)
        throw std::runtime_error("TcpSession::write(): src space remaining is less than the size to write");

    async_write(m_socket, buffer(src.array() + src.position() + offset, length),
            boost::bind(&TcpSession::writeHandler,
            shared_from_this(),
            placeholders::error,
            placeholders::bytes_transferred));
}

void TcpSession::writeHandler(const boost::system::error_code& error, size_t bytes_transferred) {
    // 此时对方关闭socket，m_socket.is_open()仍为true error == error::broken_pipe
    if (error) {
        Util::output("TcpSession", " writeHandler error: ", error.message());
        std::cout << "error code: " << error.value() << std::endl;
        std::unique_lock<std::mutex> lk(Resource::GetResource()->GetTableMutex());
        if (m_vinStr.compare(VinInital) != 0) {
            int n = Resource::GetResource()->GetVechicleConnTable().erase(m_vinStr);
            std::cout << "TcpSession::writeHandler erase " << m_vinStr << ": " << n << std::endl;
        }
        return;
    }
    std::stringstream msg;
    msg << bytes_transferred << " bytes sent";
    Util::output(m_vinStr, msg.str());
}

void TcpSession::readTimeoutHandler(const boost::system::error_code& error) {
    if (!error) {
        m_quit = true;
        Util::output(m_vinStr, " read Timeout");
        m_socket.close();
    }
}

// m_packetRef 包含了头部和数据单元和bcc，此时pos指向数据单元

void TcpSession::parseDataUnit() {
    assert(m_packetRef->position() == sizeof (DataPacketHeader));

    BytebufSPtr_t rtData;
    int cmdId = m_hdr->cmdId;
    if (cmdId == enumCmdCode::reissueUpload || cmdId == enumCmdCode::vehicleSignalDataUpload) {
        rtData = boost::make_shared<bytebuf::ByteBuffer>(m_packetRef->capacity());

        try {
            rtData->put(m_packetRef->array(), 0, sizeof (DataPacketHeader));
            // 最后一位为check code
            for (; m_packetRef->remaining() > 1;) {
                uint8_t typ = m_packetRef->get(m_packetRef->position());
                switch (typ) {
                    case VehicleDataStructInfo::CBV_typeCode:
                        rtData->put(*m_packetRef, VehicleDataStructInfo::CBV_size + 1);
                        break;
                    case VehicleDataStructInfo::DM_typeCode:
                        rtData->put(*m_packetRef, VehicleDataStructInfo::DM_size + 1);
                        break;
                    case VehicleDataStructInfo::L_typeCode:
                        rtData->put(*m_packetRef, VehicleDataStructInfo::L_size + 1);
                        break;
                    case VehicleDataStructInfo::EV_typeCode:
                        rtData->put(*m_packetRef, VehicleDataStructInfo::EV_size + 1);
                        break;
                    case VehicleDataStructInfo::A_typeCode:
                    {
                        // 类型码(1) + 报警等级(1) + 通用报警标志(4)
                        rtData->put(*m_packetRef, 6);
                        size_t n = m_packetRef->get(m_packetRef->position()); // 可充电储能装置故障总数
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 驱动电机故障总数
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 发动机故障总数
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 其他故障总数
                        rtData->put(*m_packetRef, 1 + n * 4);
                        break;
                    }
                    case VehicleDataStructInfo::RESV_typeCode:
                    {
                        m_packetRef->movePosition(1);
                        size_t sysn = m_packetRef->get();
                        for (size_t i = 0; i < sysn; i++) {
                            m_packetRef->movePosition(9);
                            size_t m = m_packetRef->get(); // 本帧单体电池总数
                            m_packetRef->movePosition(2 * m);
                        }
                        break;
                    }
                    case VehicleDataStructInfo::REST_typeCode:
                    {
                        m_packetRef->movePosition(1);
                        size_t sysn = m_packetRef->get();
                        for (size_t i = 0; i < sysn; i++) {
                            m_packetRef->movePosition(1);
                            size_t m = ntohs(m_packetRef->getShort()); // 可充电储能温度探针个数
                            m_packetRef->movePosition(m);
                        }
                        break;
                    }
                    default: {
                        std::cout << "invalid packet format, type: " << typ << std::endl;
                        throw std::runtime_error("invalid packet format");
                    }
                }
            }
        } catch (bytebuf::ByteBufferException& e) {
            Util::output(m_vinStr, "TcpSession::dealData() error: ", e.what());
            throw std::runtime_error("invalid packet format");
        }
        assert(m_packetRef->remaining() == 1);
        //            throw std::runtime_error("invalid packet format");

        if (cmdId == enumCmdCode::vehicleSignalDataUpload && !Resource::GetResource()->GetTcpConnWithPublicPlatform().isConnected())
            ((DataPacketHeader_t*) rtData->array())->cmdId = enumCmdCode::reissueUpload;

        rtData->put(Util::generateBlockCheckCharacter(rtData->array() + 2, rtData->position()));
        rtData->flip();
    } else if (cmdId == enumCmdCode::vehicleLogin || cmdId == enumCmdCode::vehicleLogout) {
        m_packetRef->position(0);
        Resource::GetResource()->GetVehicleDataQueue().put(m_packetRef);
        std::unique_lock<std::mutex> lk(Resource::GetResource()->GetTableMutex());
        if (cmdId == enumCmdCode::vehicleLogin) {
            std::pair < std::map<std::string, SessionRef_t>::iterator, bool> ret;
            ret = Resource::GetResource()->GetVechicleConnTable().insert(std::pair<std::string, SessionRef_t>(m_vinStr, shared_from_this()));
            std::cout << "TcpSession::parseDataUnit login insert " << m_vinStr << ": " << ret.second << std::endl;
        } else {
            int n = Resource::GetResource()->GetVechicleConnTable().erase(m_vinStr);
            std::cout << "TcpSession::parseDataUnit logout erase " << m_vinStr << ": " << n << std::endl;
            m_quit = true;
        }
    } else if (cmdId == enumCmdCode::heartBeat) {
        // write ok response to car
        if (m_packetRef->remaining() == 1 && m_dataUnitLen == 0) {
            m_hdr->responseFlag = responseflag::enumResponseFlag::success;
            // regenerate bcc

            uint8_t bcc = Util::generateBlockCheckCharacter((uint8_t*) m_hdr + 2, sizeof (DataPacketHeader_t) - 2);
            m_packetRef->put(bcc);
            m_packetRef->flip();

            write(*m_packetRef);
            return;
        }
        throw std::runtime_error("invalid packet format");
    } else {
        std::stringstream errMsg;
        errMsg << "invalid cmd id: " << cmdId;
        throw std::runtime_error(errMsg.str());
    }
}
