
#ifndef __DATETIME_H__
#define __DATETIME_H__

#include <string>
#include <inttypes.h>

// 1601��1��1�դ���1970��1��1�դޤǤ�ͨ��100�ʥ���
#define UNIXTIME_BASE	((_int64)0x019db1ded53e8000)

class DateTime
{
public:
    static std::string Now();
    static std::string NowMs();
    static uint64_t UnixTime();
    static uint64_t UnixTimeMs();

    static uint32_t DayOfWeek();
    static uint32_t DayOfYear();
};

#endif
