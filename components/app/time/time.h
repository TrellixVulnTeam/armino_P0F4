#ifndef __TIME_H__
#define __TIME_H__

#include "time/sys_time.h"


time_t mktime(struct tm * const t);

char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timep);
struct tm* localtime(const time_t* t);

char *asctime_r(const struct tm *t, char *buf);
struct tm *gmtime_r(const time_t *timep, struct tm *r);
struct tm* localtime_r(const time_t* t, struct tm* r);

#endif
