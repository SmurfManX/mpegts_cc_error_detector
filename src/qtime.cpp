#include "qstdinc.h"
#include "qtime.h"

uint64_t QTimeFromTime(time_t tSrc)
{
	struct tm *pti;
	pti = localtime(&tSrc);
	return  QTimeCreate(pti->tm_year+1900, pti->tm_mon+1, pti->tm_mday, pti->tm_hour, pti->tm_min, pti->tm_sec);
}

uint64_t QTimeFromTimespec(timespec& tSrc)
{
	struct tm *pti;
	pti = localtime(&tSrc.tv_sec);
	return QTimeCreate(pti->tm_year+1900, pti->tm_mon+1, pti->tm_mday, pti->tm_hour, pti->tm_min, pti->tm_sec, tSrc.tv_nsec/10000000);
}

time_t QTimeToTime(uint64_t tSrc)
{
	struct tm tDst;

	tDst.tm_year = QTimeYear(tSrc); if (tDst.tm_year >= 1900) tDst.tm_year -= 1900; else tDst.tm_year = 0;
	tDst.tm_mon = QTimeMonth(tSrc) - 1;
	tDst.tm_mday = QTimeDay(tSrc);

	tDst.tm_hour = QTimeHour(tSrc);
	tDst.tm_min = QTimeMinute(tSrc);
	tDst.tm_sec = QTimeSecond(tSrc);
	return mktime(&tDst);
}

timespec QTimeToTimespec(uint64_t tSrc)
{
	timespec tDst;
	tDst.tv_sec = QTimeToTime(tSrc);
	tDst.tv_nsec = QTimeSecondFraq(tSrc) * 10000000;
	return tDst;
}

uint64_t QTimeNow()
{
	timespec tSrc;
	struct tm *pti;

	if (clock_gettime(CLOCK_REALTIME, &tSrc) != 0)
	{
	}

	// Getting current timeinfo
	pti = localtime(&tSrc.tv_sec);

	return QTimeCreate(pti->tm_year+1900, pti->tm_mon+1, pti->tm_mday, pti->tm_hour, pti->tm_min, pti->tm_sec, tSrc.tv_nsec/10000000);
}

uint64_t QTimeCreate(int year, int month, int day, int hour, int minute, int second, int sfraq)
{
	uint64_t mt;
	mt = year;
	mt <<= 8;
	mt += month;
	mt <<= 8;
	mt += day;
	mt <<= 8;
	mt += hour;
	mt <<= 8;
	mt += minute;
	mt <<= 8;
	mt += second;
	mt <<= 8;
	mt += sfraq;

	return mt;
}

int QTimeYear(uint64_t mt)
{
	return (int)(mt >> 48);
}

int QTimeMonth(uint64_t mt)
{
	return (int)((mt >> 40) & 0xFF);
}

int QTimeDay(uint64_t mt)
{
	return (int)((mt >> 32) & 0xFF);
}

int QTimeHour(uint64_t mt)
{
	return (int)((mt >> 24) & 0xFF);
}

int QTimeMinute(uint64_t mt)
{
	return (int)((mt >> 16) & 0xFF);
}

int QTimeSecond(uint64_t mt)
{
	return (int)((mt >> 8) & 0xFF);
}

int QTimeSecondFraq(uint64_t mt)
{
	return (int)(mt  & 0xFF);
}

int QTimeDiffSecs(uint64_t t1, uint64_t t2)
{
	return (int)QTimeToTime(t2) - (int)QTimeToTime(t1);
}

uint64_t QTimeAddSecs(uint64_t tSrc, int nSecs)
{
	timespec tSpec = QTimeToTimespec(tSrc);
	tSpec.tv_sec += nSecs;
	return QTimeFromTimespec(tSpec);
}



void QTimeToStr(char* pDst, uint64_t nTime)
{
	sprintf(pDst, "%04d%02d%02d%02d%02d%02d%02d", 
			QTimeYear(nTime), QTimeMonth(nTime),
			QTimeDay(nTime), QTimeHour(nTime),
			QTimeMinute(nTime), QTimeSecond(nTime),
			QTimeSecondFraq(nTime));
}

void QTimeToStrR(char* pDst, uint64_t nTime)
{
	sprintf(pDst, "%04d-%02d-%02d %02d:%02d:%02d.%02d", 
			QTimeYear(nTime), QTimeMonth(nTime),
			QTimeDay(nTime), QTimeHour(nTime),
			QTimeMinute(nTime), QTimeSecond(nTime),
			QTimeSecondFraq(nTime));
}

uint64_t QTimeFromStr(const char* pStr)
{
	int nYear, nMonth, nDay, nHour, nMinute, nSecond, nFraq;
	nYear = nMonth = nDay = nHour = nMinute = nSecond = nFraq = 0;

	sscanf(pStr, "%04d%02d%02d%02d%02d%02d%02d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond, &nFraq);

	return QTimeCreate(nYear, nMonth, nDay, nHour, nMinute, nSecond, nFraq);
}

uint64_t QTimeFromStrNet(const char* pStr)
{
	int nYear, nMonth, nDay, nHour, nMinute, nSecond, nFraq;
	nYear = nMonth = nDay = nHour = nMinute = nSecond = nFraq = 0;

	sscanf(pStr, "%04d-%02d-%02d-%02d-%02d-%02d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond);

	return QTimeCreate(nYear, nMonth, nDay, nHour, nMinute, nSecond, 0);
}

void QTimeToStrNet(char* pDst, uint64_t nTime)
{
	int nYear, nMonth, nDay, nHour, nMinute, nSecond, nFraq;
	nYear = nMonth = nDay = nHour = nMinute = nSecond = nFraq = 0;

	sprintf(pDst, "%04d-%02d-%02d-%02d-%02d-%02d",
			QTimeYear(nTime), QTimeMonth(nTime),
			QTimeDay(nTime), QTimeHour(nTime),
			QTimeMinute(nTime), QTimeSecond(nTime));
}


uint64_t QDiffTimespec(timespec* pNext, timespec* pPrev)
{
        uint64_t nDiff = pNext->tv_sec - pPrev->tv_sec;
        nDiff = nDiff * (uint64_t)1000000000;
        if ((uint64_t)pNext->tv_nsec >= (uint64_t)pPrev->tv_nsec)
        {
                nDiff += ((uint64_t)pNext->tv_nsec - (uint64_t)pPrev->tv_nsec);
        }
        else
        {
                nDiff += ((uint64_t)pNext->tv_nsec - (uint64_t)pPrev->tv_nsec);
                //nDiff += ((uint64_t)10000000000 - (uint64_t)pPrev->tv_nsec + (uint64_t)pNext->tv_nsec);
        }
        return nDiff;
}

void QQ2GetHTTPTime(std::string& sLine, int nShift)
{
	/* Date: Thu, 01 Aug 2013 19:42:25 GMT */
	char sBuffer[128];
	time_t rawtime;
	int32_t* pRawTime = (int32_t*)&rawtime;
    time ( &rawtime );

    *pRawTime = *pRawTime + (int32_t)nShift;

	strftime (sBuffer, 127, "%a, %d %b %G %H:%M:%S GMT", gmtime ( &rawtime ));

	sLine = std::string(sBuffer);
}

