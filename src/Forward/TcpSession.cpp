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
#include <fstream>
#include <boost/bind.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <stdexcept>
#include <string>
#include <sstream>
#include <bits/stl_map.h>
#include <bits/basic_string.h>
#include <boost/lexical_cast.hpp>

#include "../Util.h"
#include "resource.h"
#include "Constant.h"
#include "Uploader.h"

using namespace boost::asio;

TcpSession::TcpSession(io_service& ioservice) :
m_heartBeatCycle(resource::getResource()->getHeartBeatCycle()),
m_socket(ioservice),
m_timer(ioservice),
m_quit(false),
m_logger(resource::getResource()->getLogger()),
m_vin(Constant::vinInital) {
}

TcpSession::~TcpSession() {
    // deadline_timer的析构函数什么也不做，因此不会导致发出的async_wait被cancel。
    m_timer.cancel();
    //    std::cout << m_vinStr << ": TcpSession destructed" << ' ' << Util::nowTimeStr() << std::endl;
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
    try {
        m_timer.cancel();
        if (error) {
            if (m_vin.compare(Constant::vinInital) != 0) {
                boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
                int n = resource::getResource()->getVechicleSessionTable().erase(m_vin);
                //            std::cout << "TcpSession::readHeaderHandler erase " << m_vinStr << ": " << n << ' '<< Util::nowTimeStr() << std::endl;
            }

            if (error == error::operation_aborted) {
                m_logger.info("TcpSession::readHeaderHandler", "read operation canceled may because time out");
            } else if (error == error::eof) {
                m_logger.info("TcpSession::readHeaderHandler", "session closed by peer");
            } else {
                m_logger.error("TcpSession::readHeaderHandler error");
                m_logger.errorStream << "message: " << error.message() << ", code: " << error.value() << std::endl;
            }
            return;
        }
        assert(bytes_transferred == sizeof (DataPacketHeader));
        m_packetRef->position(bytes_transferred);

        m_hdr = (DataPacketHeader_t*) m_packetRef->array();
        if (m_vin.compare(Constant::vinInital) == 0) {
            m_vin.assign((char*) m_hdr->vin, sizeof (m_hdr->vin));
        }
        
        // special case: 只上传 86687302051846000 的数据
        if (m_vin.compare("86687302051846000") != 0)
            return;

        readDataUnit();
    } catch (std::runtime_error& e) {
        m_logger.error(m_vin, "readHeaderHandler exception");
        m_logger.errorStream << e.what() << std::endl;
    }
}

void TcpSession::readDataUnit() {
    m_dataUnitLen = boost::asio::detail::socket_ops::network_to_host_short(m_hdr->dataUnitLength);
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
    try {
        m_timer.cancel();
        if (error) {
            if (m_vin.compare(Constant::vinInital) != 0) {
                boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
                int n = resource::getResource()->getVechicleSessionTable().erase(m_vin);
                //            std::cout << "TcpSession::readDataUnitHandler erase " << m_vinStr << ": " << n << ' ' << Util::nowTimeStr() << std::endl;
            }

            if (error == error::operation_aborted) {
                m_logger.info(m_vin, "read operation canceled may because time out");
            } else if (error == error::eof) {
                m_logger.info(m_vin, "session closed by peer");
            } else {
                m_logger.error(m_vin, "TcpSession::readDataUnitHandler error");
                m_logger.errorStream << "message: " << error.message() << ", code: " << error.value() << std::endl;
            }
            return;
        }
        assert(bytes_transferred == m_dataUnitLen + 1);
        m_packetRef->limit(m_packetRef->position() + bytes_transferred);
        parseDataUnit();
        if (!m_quit)
            readHeader();
    } catch (std::runtime_error& e) {
        m_logger.error(m_vin, "readDataUnitHandler exception");
        m_logger.errorStream << e.what() << std::endl;
    }
}

void TcpSession::write(const bytebuf::ByteBuffer& src) {
    write(src, 0, src.remaining());
}

void TcpSession::write(const bytebuf::ByteBuffer& src, const size_t& offset, const size_t& length) {
    if (src.remaining() < length + offset)
        throw std::runtime_error("TcpSession::write(): src space remaining is less than the size to write");

    // ResponseReader 可能会调用写接口
    boost::unique_lock<boost::mutex> lk(m_mutex);
    async_write(m_socket, buffer(src.array() + src.position() + offset, length),
            boost::bind(&TcpSession::writeHandler,
            shared_from_this(),
            placeholders::error,
            placeholders::bytes_transferred));
}

void TcpSession::writeHandler(const boost::system::error_code& error, size_t bytes_transferred) {
    // 此时对方关闭socket，m_socket.is_open()仍为true error == error::broken_pipe 
    // 经实测，以上有误。对方关闭socket，这边不会有任何影响
    if (error) {
        m_logger.error("TcpSession::writeHandler");
        m_logger.errorStream << "message: " << error.message() << ", error code: " << error.value() << std::endl;
        boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
        if (m_vin.compare(Constant::vinInital) != 0) {
            int n = resource::getResource()->getVechicleSessionTable().erase(m_vin);
            //            std::cout << "TcpSession::writeHandler erase " << m_vinStr << ": " << n << ' ' << Util::nowTimeStr() << std::endl;
        }
        return;
    }
    //    m_stream.str("");
    //    m_stream << bytes_transferred << " bytes sent to vehicle";
    //    m_logger.info(m_vinStr, m_stream.str());
}

void TcpSession::readTimeoutHandler(const boost::system::error_code& error) {
    if (!error) {
        m_quit = true;
        m_logger.warn(m_vin, "read vehicle session timeout");
        m_socket.close();
    }
}

// m_packetRef 包含了头部和数据单元和bcc，此时pos指向数据单元

void TcpSession::parseDataUnit() {
    assert(m_packetRef->position() == sizeof (DataPacketHeader));
    int cmdId = m_hdr->cmdId;
    if (cmdId == enumCmdCode::reissueUpload || cmdId == enumCmdCode::realtimeUpload) {
        BytebufSPtr_t rtData = boost::make_shared<bytebuf::ByteBuffer>(m_packetRef->capacity());

        try {
            rtData->put(m_packetRef->array(), 0, sizeof (DataPacketHeader));
            // 前6位为采集时间，最后一位为check code
            rtData->put(*m_packetRef, sizeof (TimeForward_t));

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
                    default:
                    {
                        m_stream.str("");
                        m_stream << "invalid info type code: " << (int) typ;
                        throw std::runtime_error(m_stream.str());
                    }
                }
            }
        } catch (bytebuf::ByteBufferException& e) {

            m_stream.str("");
            m_stream << "invalid packet format\n" << e.what();
            throw std::runtime_error(m_stream.str());
        }
        if (m_packetRef->remaining() != 1)
            throw std::runtime_error("invalid packet format: m_packetRef->remaining expect to be 1 after parse data unit");
        
        if (cmdId == enumCmdCode::realtimeUpload && !Uploader::isConnectWithPublicServer)
            ((DataPacketHeader_t*) rtData->array())->cmdId = enumCmdCode::reissueUpload;

        uint16_t newDataUnitLength = rtData->position() - sizeof (DataPacketHeader_t);
        ((DataPacketHeader_t*) rtData->array())->dataUnitLength = htons(newDataUnitLength);
        rtData->put(Util::generateBlockCheckCharacter(rtData->array() + 2, rtData->position() - 2));
        rtData->flip();
        resource::getResource()->getVehicleDataQueue().put(rtData);
        std::string type = cmdId == enumCmdCode::reissueUpload ? 
            Constant::cmdReissueUploadStr : Constant::cmdRealtimeUploadStr;
        m_logger.info(m_vin, type + " data put into queue, now queue size: "
                + boost::lexical_cast<std::string>(resource::getResource()->getVehicleDataQueue().remaining()));
    } else if (cmdId == enumCmdCode::vehicleLogin || cmdId == enumCmdCode::vehicleLogout) {
        m_packetRef->position(0);
        resource::getResource()->getVehicleDataQueue().put(m_packetRef);
        std::string type = cmdId == enumCmdCode::vehicleLogin ? 
            Constant::cmdVehicleLoginStr : Constant::cmdVehicleLogoutStr;
        m_logger.info(m_vin, type + " data put into queue, now queue size: "
                + boost::lexical_cast<std::string>(resource::getResource()->getVehicleDataQueue().remaining()));
        boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
        if (cmdId == enumCmdCode::vehicleLogin) {
            std::pair < std::map<std::string, SessionRef_t>::iterator, bool> ret;
            ret = resource::getResource()->getVechicleSessionTable().insert(std::pair<std::string, SessionRef_t>(m_vin, shared_from_this()));
            //            std::cout << "TcpSession::parseDataUnit login insert " << m_vinStr << ": " << ret.second << ' '<< Util::nowTimeStr() << std::endl;
        } else {
            int n = resource::getResource()->getVechicleSessionTable().erase(m_vin);
            //            std::cout << "TcpSession::parseDataUnit logout erase " << m_vinStr << ": " << n << ' '<< Util::nowTimeStr() << std::endl;
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
        m_stream.str("");
        m_stream << "invalid cmd id: " << cmdId;
        throw std::runtime_error(m_stream.str());
    }
}
