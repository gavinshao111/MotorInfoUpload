
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
    BOOST_LOG_SEV((gavinlog::s_slg),(boost::log::trivial::info)) << "[" << (id) << "] "
#define GDEBUG(id)\
    BOOST_LOG_SEV((gavinlog::s_slg),(boost::log::trivial::debug)) << "[" << (id) << "] "
#define GWARNING(id)\
    BOOST_LOG_SEV((gavinlog::s_slg),(boost::log::trivial::warning)) << "[" << (id) << "] "

namespace gavinlog {

    void init(std::string&& directory);
    extern boost::log::sources::severity_logger<boost::log::trivial::severity_level > s_slg;
}
#endif /* LOGGER_H */

