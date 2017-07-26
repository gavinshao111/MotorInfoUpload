
/* 
 * File:   logger.cpp
 * Author: 10256
 * 
 * Created on 2017年7月25日, 下午3:21
 */

#include "logger.h"
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/detail/format.hpp>
#include <boost/log/detail/thread_id.hpp>

namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(my_logger, src::logger_mt)
boost::log::sources::severity_logger<boost::log::trivial::severity_level > gavinlog::s_slg;

void gavinlog::init(std::string&& directory) {
    std::string info_dir = directory + "/info";
    std::string warn_dir = directory + "/warn";
    std::string debug_dir = directory + "/debug";
    
    if (boost::filesystem::exists(info_dir) == false) {
        boost::filesystem::create_directories(info_dir);
    }
    if (boost::filesystem::exists(warn_dir) == false) {
        boost::filesystem::create_directories(warn_dir);
    }
    if (boost::filesystem::exists(debug_dir) == false) {
        boost::filesystem::create_directories(debug_dir);
    }

    auto pSinkDeb = boost::log::add_file_log
            (
            keywords::filter = expr::attr< boost::log::trivial::severity_level >("Severity") == boost::log::trivial::debug, 
            keywords::open_mode = std::ios::app,
            keywords::file_name = debug_dir + "/%Y%m%d.log",
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format =
            (
            expr::stream
            << "[" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] " << expr::smessage
            << "\n"
            )
            );
    pSinkDeb->locked_backend()->auto_flush(true);
    
    auto pSinkInfo = boost::log::add_file_log
            (
            keywords::filter = expr::attr< boost::log::trivial::severity_level >("Severity") == boost::log::trivial::info, 
            keywords::open_mode = std::ios::app,
            keywords::file_name = info_dir + "/%Y%m%d.log",
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format =
            (
            expr::stream
            << "[" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] " << expr::smessage
            << "\n"
            )
            );
    pSinkInfo->locked_backend()->auto_flush(true);
    
    auto pSinkWarn = boost::log::add_file_log
            (
            keywords::filter = expr::attr< boost::log::trivial::severity_level >("Severity") >= boost::log::trivial::warning,
            keywords::open_mode = std::ios::app,
            keywords::file_name = warn_dir + "/%Y%m%d.log",
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format =
            (
            expr::stream
            << "[" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] " << expr::smessage
            << "\n"
            )
            );
    pSinkWarn->locked_backend()->auto_flush(true);
    
    boost::log::add_common_attributes();
}

