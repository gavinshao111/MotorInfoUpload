#include "DataFormatForward.h"
#include <cstring>

DataPacketHeader::DataPacketHeader() {
    startCode[0] = '#';
    startCode[1] = '#';
    responseFlag = 0xfe;
    ::memset(vin, 0, VINLEN);
}

LoginDataForward::LoginDataForward() {
    ::memset(username, 0, sizeof(username));
    ::memset(password, 0, sizeof(password));
}
