
/* 
 * File:   logger.h
 * Author: 10256
 *
 * Created on 2017年7月25日, 下午3:21
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <boost/log/trivial.hpp>

#define GINFO(id)\
    BOOST_LOG_SEV((gavinlog::s_slg),(gavinlog::severity_level::info)) << "[" << (id) << "] "
#define GDEBUG(id)\
    BOOST_LOG_SEV((gavinlog::s_slg),(gavinlog::severity_level::debug)) << "[" << (id) << "] "
#define GWARNING(id)\
    BOOST_LOG_SEV((gavinlog::s_slg),(gavinlog::severity_level::warning)) << "[" << (id) << "] "
#define GREPORT\
    BOOST_LOG_SEV((gavinlog::s_slg),(gavinlog::severity_level::report))

namespace gavinlog {
    enum severity_level {
        debug,
        info,
        report,
        warning
    };

    void init(std::string&& directory);
    extern boost::log::sources::severity_logger<severity_level> s_slg;
}
#endif /* LOGGER_H */

