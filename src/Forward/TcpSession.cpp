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
#include <vector>

#include "../Util.h"
#include "resource.h"
#include "Constant.h"
#include "Uploader.h"
#include "logger.h"

using namespace boost::asio;

TcpSession::TcpSession(io_service& ioservice) :
m_heartBeatCycle(resource::getResource()->getHeartBeatCycle()),
m_socket(ioservice),
m_timer(ioservice),
m_quit(false),
m_vin(Constant::vinInital) {
}

TcpSession::~TcpSession() {
    // deadline_timer的析构函数什么也不做，因此不会导致发出的async_wait被cancel。
    m_timer.cancel();
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
                resource::getResource()->getVechicleSessionTable().erase(m_vin);
            }

            if (error == error::operation_aborted) {
            } else if (error == error::eof) {
                GINFO("TcpSession") << "session closed by peer when readHeaderHandler";
            } else {
                GWARNING("TcpSession") << "readHeaderHandler error, message: "
                        << error.message() << ", code: " << error.value();
            }
            return;
        }
        assert(bytes_transferred == sizeof (DataPacketHeader));
        m_packetRef->position(bytes_transferred);

        m_hdr = (DataPacketHeader_t*) m_packetRef->array();
        if (m_vin.compare(Constant::vinInital) == 0) {
            m_vin.assign((char*) m_hdr->vin, sizeof (m_hdr->vin));
        }

        // 若在符合性检测下，只上传指定车机的数据
        if (resource::getResource()->getMode() != EnumRunMode::release) {
            const std::vector<std::string>& vins = resource::getResource()->getVinAllowedArray();
            for (int i = 0;; i++) {
                if (i >= vins.size())
                    return;
                if (0 == m_vin.compare(vins[i]))
                    break;
            }
        }

        readDataUnit();
    } catch (std::runtime_error& e) {
        GWARNING(m_vin) << "readHeaderHandler exception: " << e.what();
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
                resource::getResource()->getVechicleSessionTable().erase(m_vin);
            }

            if (error == error::operation_aborted) {
            } else if (error == error::eof) {
                GINFO(m_vin) << "session closed by peer";
            } else {
                GWARNING(m_vin) << "TcpSession::readDataUnitHandler error, message: "
                        << error.message() << ", code: " << error.value();
            }
            return;
        }
        assert(bytes_transferred == m_dataUnitLen + 1);
        m_packetRef->limit(m_packetRef->position() + bytes_transferred);
        boost_bytebuf_sptr packet = parseDataUnit();
        if (packet) {
            auto& realtime_queue = resource::getResource()->get_realtime_queue();
            auto& reissue_queue = resource::getResource()->get_reissue_queue();
            // 离线期间受到的数据全部发放入补发队列，Uploader与上级平台重连后先发补发队列的数据
            if (!resource::getResource()->is_connected_with_public_server()) {
                for (; !realtime_queue.isEmpty(); reissue_queue.put(realtime_queue.take()));
                reissue_queue.put(packet);
                GINFO(m_vin) << "reissue queue size: "
                        << reissue_queue.remaining();
            } else
                realtime_queue.put(packet);
        }

        if (!m_quit)
            readHeader();
    } catch (std::runtime_error& e) {
        GWARNING(m_vin) << "readDataUnitHandler exception: " << e.what();
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
        GWARNING(m_vin) << "TcpSession::writeHandler error, message: "
                << error.message() << ", code: " << error.value();
        boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
        if (m_vin.compare(Constant::vinInital) != 0) {
            resource::getResource()->getVechicleSessionTable().erase(m_vin);
        }
        return;
    }
}

void TcpSession::readTimeoutHandler(const boost::system::error_code& error) {
    if (!error) {
        m_quit = true;
        //        GWARNING(m_vin) << "read vehicle session timeout";
        m_socket.close();
    }
}

/**
 * m_packetRef 包含了头部和数据单元和bcc，此时pos指向数据单元
 */
boost_bytebuf_sptr TcpSession::parseDataUnit() {
    assert(m_packetRef->position() == sizeof (DataPacketHeader));
    //    GDEBUG(__func__) << "data unit: " << m_packetRef->to_hex();
    int cmdId = m_hdr->cmdId;

    if (cmdId == enumCmdCode::reissueUpload || cmdId == enumCmdCode::realtimeUpload) {
        auto rtData = boost::make_shared<bytebuf::ByteBuffer>(m_packetRef->capacity());
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
                        if (n == 0xff || n == 0xfe) n = 0;
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 驱动电机故障总数
                        if (n == 0xff || n == 0xfe) n = 0;
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 发动机故障总数
                        if (n == 0xff || n == 0xfe) n = 0;
                        rtData->put(*m_packetRef, 1 + n * 4);
                        n = m_packetRef->get(m_packetRef->position()); // 其他故障总数
                        if (n == 0xff || n == 0xfe) n = 0;
                        rtData->put(*m_packetRef, 1 + n * 4);
                        break;
                    }
                    case VehicleDataStructInfo::RESV_typeCode:
                    {
                        m_packetRef->movePosition(1);
                        size_t sysn = m_packetRef->get();
                        if (sysn == 0xff || sysn == 0xfe)
                            sysn = 0;
                        else if (sysn < 1 || sysn > 250)
                            throw std::runtime_error("invalid 可充电储能电压子系统个数: " + std::to_string(sysn));
                        for (size_t i = 0; i < sysn; i++) {
                            m_packetRef->movePosition(9);
                            size_t m = m_packetRef->get(); // 本帧单体电池总数
                            if (m < 1 || m > 200)
                                throw std::runtime_error("invalid 本帧单体电池总数: " + std::to_string(m));
                            m_packetRef->movePosition(2 * m);
                        }
                        break;
                    }
                    case VehicleDataStructInfo::REST_typeCode:
                    {
                        m_packetRef->movePosition(1);
                        size_t sysn = m_packetRef->get();
                        if (sysn == 0xff || sysn == 0xfe)
                            sysn = 0;
                        else if (sysn < 1 || sysn > 250)
                            throw std::runtime_error("invalid 可充电储能温度子系统个数: " + std::to_string(sysn));
                        for (size_t i = 0; i < sysn; i++) {
                            m_packetRef->movePosition(1);
                            size_t m = ntohs(m_packetRef->getShort()); // 可充电储能温度探针个数
                            if (m == 0xffff || m == 0xfffe)
                                sysn = 0;
                            else if (m < 1 || m > 65531)
                                throw std::runtime_error("invalid 可充电储能温度探针个数: " + std::to_string(m));
                            m_packetRef->movePosition(m);
                        }
                        break;
                    }
                    default:
                    {
                        throw std::runtime_error("invalid info type code: " + std::to_string((int) typ));
                    }
                }
            }
        } catch (bytebuf::ByteBufferException& e) {
            throw std::runtime_error(std::string("invalid packet format: ") + e.what());
        }
        if (m_packetRef->remaining() != 1)
            throw std::runtime_error(
                "invalid packet format: m_packetRef->remaining expect to be 1 after parse data unit");

        uint16_t newDataUnitLength = rtData->position() - sizeof (DataPacketHeader_t);
        ((DataPacketHeader_t*) rtData->array())->dataUnitLength =
                boost::asio::detail::socket_ops::host_to_network_short(newDataUnitLength);
        rtData->put(Util::generateBlockCheckCharacter(rtData->array() + 2, rtData->position() - 2));
        rtData->flip();
        return rtData;
    }

    if (cmdId == enumCmdCode::vehicleLogin) {
        m_packetRef->position(0);
        boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
        resource::getResource()->getVechicleSessionTable().insert(
                std::pair<std::string, SessionRef_t>(m_vin, shared_from_this()));
        return m_packetRef;
    }

    if (cmdId == enumCmdCode::vehicleLogout) {
        m_packetRef->position(0);
        boost::unique_lock<boost::mutex> lk(resource::getResource()->getTableMutex());
        resource::getResource()->getVechicleSessionTable().erase(m_vin);
        // 车机登出，结束会话
        m_quit = true;
        return m_packetRef;
    }

    if (cmdId == enumCmdCode::heartBeat) {
        // write ok response to car
        if (m_packetRef->remaining() == 1 && m_dataUnitLen == 0) {
            m_hdr->responseFlag = responseflag::enumResponseFlag::success;
            // regenerate bcc
            uint8_t bcc = Util::generateBlockCheckCharacter((uint8_t*) m_hdr + 2, sizeof (DataPacketHeader_t) - 2);
            m_packetRef->put(bcc);
            m_packetRef->flip();

            write(*m_packetRef);
            return nullptr;
        }
        throw std::runtime_error("invalid packet format");
    }

    throw std::runtime_error("invalid cmd id: " + std::to_string(cmdId));
}
