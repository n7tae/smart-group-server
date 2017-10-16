#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <ctime>
#include <string>
#include <iterator>
#include <cstring>

#include "Utilities.h"


void lprint(const char *fmt,...)
{
	time_t ltime;
	struct tm tm;
	const short BFSZ = 512;
	char buf[BFSZ];

	time(&ltime);
	localtime_r(&ltime, &tm);

	snprintf(buf,BFSZ - 1,"%02d/%02d/%02d %d:%02d:%02d:",
	         tm.tm_mon+1,tm.tm_mday,tm.tm_year % 100,
	         tm.tm_hour,tm.tm_min,tm.tm_sec);

	va_list args;
	va_start(args,fmt);
	vsnprintf(buf + strlen(buf), BFSZ - strlen(buf) -1, fmt, args);
	va_end(args);

	printf("%s", buf);
	return;
}

void toupper(std::string &str)
{
	for (auto it=str.begin(); it!=str.end(); it++) {
		if (islower(*it))
			*it = toupper(*it);
	}
}
