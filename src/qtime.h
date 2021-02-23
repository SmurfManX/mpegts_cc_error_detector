#ifndef __Q_TIME_H__
#define __Q_TIME_H__

#include "qstdinc.h"

/* 
	Current time format 64 bits:
	YYYY MMDD hhmm ssms
	YYYY	- complete year
	MM	- month, January is 1
	DD	- day, 1-31
	hh	- hour, 0-23
	mm	- minute, 0-59
	ss	- second, 0-59
	ms	- 1/100 OS second, 0-99
*/

/* Time conversions */
uint64_t QTimeFromTime(time_t);
uint64_t QTimeFromTimespec(timespec&);
time_t QTimeToTime(uint64_t);
timespec QTimeToTimespec(uint64_t);

/* Current time */
uint64_t QTimeNow();

/* Create time */
uint64_t QTimeCreate(int year, int month, int day, int hour=0, int minute=0, int second=0, int sfraq=0);

/*Time components */
int QTimeYear(uint64_t);
int QTimeMonth(uint64_t);
int QTimeDay(uint64_t);
int QTimeHour(uint64_t);
int QTimeMinute(uint64_t);
int QTimeSecond(uint64_t);
int QTimeSecondFraq(uint64_t);

/* Time difference */
int QTimeDiffSecs(uint64_t, uint64_t);		/* retval = arg2 - arg1 */
uint64_t QTimeAddSecs(uint64_t, int);

/* Out time to string conversion functions */
void QTimeToStr(char* pDst, uint64_t nTime);
void QTimeToStrR(char* pDst, uint64_t nTime);
void QTimeToStrNet(char* pDst, uint64_t nTime);
uint64_t QTimeFromStr(const char*);
uint64_t QTimeFromStrNet(const char*);
void QQ2GetHTTPTime(std::string& sLine, int nShift = 0);

/* Timespec safe loader */
#define QSafeTimespec(A) if (clock_gettime(CLOCK_REALTIME, (timespec*)&A)) { QLogError(0, "clock_gettime : %d / %s / %d", errno, __FILE__, __LINE__); exit(0); }

/* Timespec difference */
uint64_t QDiffTimespec(timespec* pNext, timespec* pPrev);

#endif
