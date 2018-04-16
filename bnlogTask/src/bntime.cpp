/*
 * bntime.cpp
 *
 *  Created on: 2017年8月23日
 */

#include "bnosutils.h"

// Add gettimeofday function for windows system
#ifdef WIN32
int gettimeofday(struct timeval *tp, void *tzp){
    time_t tSecond;
    struct tm tm;
    SYSTEMTIME tSystemTime;

    GetLocalTime(&tSystemTime);
    tm.tm_year = tSystemTime.wYear - 1900;
    tm.tm_mon = tSystemTime.wMonth - 1;
    tm.tm_mday = tSystemTime.wDay;
    tm.tm_hour = tSystemTime.wHour;
    tm.tm_min = tSystemTime.wMinute;
    tm.tm_sec = tSystemTime.wSecond;
    tm. tm_isdst = -1;
    tSecond = mktime(&tm);
    tp->tv_sec = tSecond;
    tp->tv_usec = tSystemTime.wMilliseconds * 1000;

    return 0;
}
#endif



