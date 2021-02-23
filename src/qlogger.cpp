/*
 * qlogger.cpp
 *
 *  Created on: May 18, 2013
 *      Author: root
 */

#include "qstdinc.h"
#include "qsync.h"
#include "qlogger.h"
#include "qtime.h"

/* Nothing will work until this variable is set to 0 */
static volatile long g_LoggerInit = 0;
static volatile long g_LoggerEnabled = 1;
static volatile long g_ScreenEnabled = 1;
static volatile long g_DebugEnabled = 0;
static volatile long g_FileEnabled = 1;
static std::string g_sBaseName;
static volatile uint64_t g_nSourceID = 1;

static char g_buffer[8196];
static char g_buffer_fin[8196];
static char g_ident[1280];
static char g_logfilename[512];
static char g_logfilebase[512];
static char g_fname[128];
static FILE* g_pLogFile = NULL;

/* Class that will initialize lock and this variable */
class QLogSync : public QMutexSync
{
public:
	QLogSync()
	{
		/* Set flag that we have finished initializing */
		__sync_add_and_fetch(&g_LoggerInit, 1);
	}
};

/* Log access guardian */
static QLogSync* g_pLogSync = NULL;

/* Class that will acquire locks */
class QLogLocker
{
public:
	QLogLocker()
	{
		/* Test for finish */
		m_bLock = false;
		int nLock = __sync_add_and_fetch(&g_LoggerInit, 1);
		if (nLock < 1)
		{
			/* Transit from uninitialized state, fail */
			__sync_sub_and_fetch(&g_LoggerInit, 1);
			return;
		}

		/* Acquire lock */
		g_pLogSync->SyncLock();
		m_bLock = true;
	}

	virtual ~QLogLocker()
	{
		/* Release if locked */
		if (m_bLock)
		{
			g_pLogSync->SyncUnlock();
			__sync_sub_and_fetch(&g_LoggerInit, 1);
		}
	}

	bool IsLocked()
	{
		return m_bLock;
	}

protected:
	bool m_bLock;
};

/* Maps for SC */
typedef std::map<std::string, QLESC*> QLESCMap;
typedef std::map<std::string, QLESC*>::iterator QLESCMapKey;
typedef std::map<uint64_t, QLESC*> QLESCMapID;
typedef std::map<uint64_t, QLESC*>::iterator QLESCMapIDKey;
typedef std::vector<QLESC*> QLESCArray;

/* Source file access manager class */
class QLogManager
{
public:
	QLogManager()
	{
		/* After all initializations create sync */
		g_pLogSync = new QLogSync();
	}

	virtual ~QLogManager()
	{
		/* Announce that we have done with logging */
		__sync_lock_test_and_set(&g_LoggerInit, LONG_MIN);

		/* Wait for g_LoggerInit become 0 */

		/* Make sure that no one access sync, use just  QLocker */
		{
			QLocker lock(g_pLogSync);
		}

		/* Delete sync */
		delete g_pLogSync;
		g_pLogSync = NULL;
	}

	bool LogSourceEnable(uint64_t nID, bool bEnable)
	{
		QLocker lock(&m_sync);
		QLESCMapIDKey it = m_mapid.find(nID);
		if (it == m_mapid.end()) return false;
		it->second->bEnabled = bEnable;
		return true;
	}

	bool LogSourceMessages(uint64_t nID, uint32_t nMessages)
	{
		QLocker lock(&m_sync);
		QLESCMapIDKey it = m_mapid.find(nID);
		if (it == m_mapid.end()) return false;
		it->second->nTypeMask = nMessages;
		return true;
	}

	bool LogSourceLevel(uint64_t nID, uint32_t nLevel)
	{
		QLocker lock(&m_sync);
		QLESCMapIDKey it = m_mapid.find(nID);
		if (it == m_mapid.end()) return false;
		it->second->nLevel = nLevel;
		return true;
	}

	void LogGetSetup(bool& bEnabled, bool& bScreenEnabled, bool& bFileEnabled, std::string& sBaseName)
	{
		bEnabled = __sync_sub_and_fetch(&g_LoggerEnabled, 0) != 0;
		bScreenEnabled = __sync_sub_and_fetch(&g_ScreenEnabled, 0) != 0;
		bFileEnabled = __sync_sub_and_fetch(&g_FileEnabled, 0) != 0;
		sBaseName = g_sBaseName;
	}

	void LogGetSourceArray(QQWordArray& ids)
	{
		QLocker lock(&m_sync);
		for (int i=0; i<(int)m_array.size(); i++)
		{
			ids.push_back(m_array[i]->nSourceID);
		}
	}

	bool LogGetIDDescr(uint64_t nID, std::string& sFile, bool& bEnabled, uint32_t& nMask, uint32_t& nLevel)
	{
		QLESCMapIDKey it = m_mapid.find(nID);
		if (it == m_mapid.end()) return false;
		sFile = it->second->sSourceFile;
		bEnabled = it->second->bEnabled;
		nMask = it->second->nTypeMask;
		nLevel = it->second->nLevel;
		return true;
	}

	/* Manage SC trees */
	QLESC* QLogRegisterSource(const char* pSource)
	{
		QLocker lock(&m_sync);
		QLESC* pRetVal = NULL;

		/* Find SC in tree if exists, or create a new */
		QLESCMapKey it = m_map.find(std::string(pSource));
		if (it != m_map.end())
		{
			pRetVal = it->second;
		}
		else
		{
			pRetVal = new QLESC();
			pRetVal->bEnabled = true;
			pRetVal->nLevel = 0;
			pRetVal->nSourceID = __sync_add_and_fetch(&g_nSourceID, 1);
			pRetVal->nTypeMask = QLT_VERBOSE | QLT_DEBUG | QLT_INFO | QLT_WARNING | QLT_ERROR;
			pRetVal->sSourceFile.assign(pSource);

			m_map[pRetVal->sSourceFile] = pRetVal;
			m_mapid[pRetVal->nSourceID] = pRetVal;
			m_array.push_back(pRetVal);
		}

		return pRetVal;
	}

protected:
	QMutexSync		m_sync;
	QLESCMap		m_map;
	QLESCMapID		m_mapid;
	QLESCArray		m_array;
};

/* Log manager */
static QLogManager g_logmgr;

/* Register source in source list */
static QLESC* QLogRegisterSource(const char* pSource)
{
	return g_logmgr.QLogRegisterSource(pSource);
}

/* Disable/Enable logging 					*/
void QLogEnable(bool bEnable)
{
	__sync_lock_test_and_set(&g_LoggerEnabled, bEnable?1:0);
}

/* Disable/Enable screen logging			*/
void QLogEnableScreen(bool bEnable)
{
	__sync_lock_test_and_set(&g_ScreenEnabled, bEnable?1:0);
}

/* Disable/Enable file logging				*/
void QLogEnableFile(bool bEnable)
{
	__sync_lock_test_and_set(&g_FileEnabled, bEnable?1:0);
}

void QLogSetDebug(bool bEnable)
{
	__sync_lock_test_and_set(&g_DebugEnabled, bEnable?1:0);
}

bool QLogGetDebug()
{
	return __sync_add_and_fetch(&g_DebugEnabled, 0) > 0;
}

bool QLogSourceEnable(uint64_t nID, bool bEnable)
{
	return g_logmgr.LogSourceEnable(nID, bEnable);
}

bool QLogSourceMessages(uint64_t nID, uint32_t nMessages)
{
	return g_logmgr.LogSourceMessages(nID, nMessages);
}

bool QLogSourceLevel(uint64_t nID, uint32_t nLevel)
{
	return g_logmgr.LogSourceLevel(nID, nLevel);
}

void QLogGetSetup(bool& bEnabled, bool& bScreenEnabled, bool& bFileEnabled, std::string& sBaseFile)
{
	return g_logmgr.LogGetSetup(bEnabled, bScreenEnabled, bFileEnabled, sBaseFile);
}

void QLogGetSourceArray(QQWordArray& ids)
{
	g_logmgr.LogGetSourceArray(ids);
}

/* Retrieve all current data for given ID	*/
bool QLogGetIDDescr(uint64_t nID, std::string& sSource, bool& bEnabled, uint32_t& nTypes, uint32_t& nLevel)
{
	return g_logmgr.LogGetIDDescr(nID, sSource, bEnabled, nTypes, nLevel);
}

/* Initialize logging */
bool QLogInit(bool bScreenLog, bool bFileLog, const char* pBase)
{
	g_ScreenEnabled = bScreenLog?1:0;
	g_FileEnabled = bFileLog?1:0;

	char sStampFile[512];
	char sTimeStr[32];
	QTimeToStrR(sTimeStr, QTimeNow());

	/* Make version stamp */
	sprintf(sStampFile, ".%s.ver", pBase);
	FILE* pStamp = fopen(sStampFile, "w+t");
	if (pStamp)
	{
		fprintf(pStamp, "%s - 0.0.1\n", sTimeStr);
		fclose(pStamp);
	}

	strcpy(g_logfilebase, pBase);
	int nLen = strlen(g_logfilebase);
	if (nLen == 0)
	{
		g_FileEnabled = 0;
	}

	return true;
}

static inline char QLogGetType(int nType)
{
	switch (nType)
	{
		default:
			return 'U';
		case QLT_VERBOSE:
			return 'V';
		case QLT_DEBUG:
			return 'D';
		case QLT_INFO:
			return 'I';
		case QLT_WARNING:
			return 'W';
		case QLT_ERROR:
			return 'E';
	}
	return 'U';
}

static void QLogCheckOpenFile()
{
	time_t rawtime;
	tm* timeinfo;

	if (g_FileEnabled == 0)
	{
		return;
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	sprintf(g_fname, "%s%04d-%02d-%02d.log", g_logfilebase, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
	if (strcmp(g_fname, g_logfilename) != 0)
	{
		if (g_pLogFile)
		{
			fclose(g_pLogFile);
			g_pLogFile = NULL;
		}

		strcpy(g_logfilename, g_fname);
		g_pLogFile = fopen(g_logfilename, "a+t");
	}
}

/* Write string to file */
static void QLogToFile(const char* pMessage)
{
	QLogCheckOpenFile();
    if (g_pLogFile) fprintf(g_pLogFile, pMessage);
}

void QLogReal(volatile QLESC** ppSC, const char* pFile, int nLine, int nType, int nLevel, const char* pString, ...)
{
	/* Initialization check */
	if (__sync_add_and_fetch(&g_LoggerInit, 0) < 1) return;
	if (__sync_add_and_fetch(&g_LoggerEnabled, 0) == 0) return;

	/* Check / create SC */
	if (*ppSC == NULL) *ppSC = QLogRegisterSource(pFile);

	/* Check rights */
	if (*ppSC == NULL) return;
	if (!(*ppSC)->bEnabled) return;
	if (((*ppSC)->nTypeMask & nType) == 0) return;
	if ((int)(*ppSC)->nLevel < nLevel) return;
	char cType = QLogGetType(nType);

	{
		/* Acquire lock and continue */
		QLogLocker lock;
		char sTimeStr[32];
		QTimeToStrR(sTimeStr, QTimeNow());
		if (lock.IsLocked())
		{
			/* Stack voodoo */
	        va_list args;
	        va_start (args, pString);
	        vsprintf (g_buffer, pString, args);
	        va_end (args);

	        /* Make ident */
	        g_ident[0] = 0;
	        for (int i=0; i<nLevel; i++)
	        {
	            strcat(g_ident, "  ");
	        }

        	if (__sync_add_and_fetch(&g_DebugEnabled, 0))
        	{
        		sprintf(g_buffer_fin, "%s - [%s][%05d][%c][%03d][%s]\n", sTimeStr, pFile, nLine, cType, nLevel, g_buffer);
        	}
        	else
        	{
        		sprintf(g_buffer_fin, "%s - [%c] %s%s\n", sTimeStr, cType, g_ident, g_buffer);
        	}

       		if (__sync_add_and_fetch(&g_ScreenEnabled, 0)) printf(g_buffer_fin);
	        if (__sync_add_and_fetch(&g_FileEnabled, 0)) QLogToFile(g_buffer_fin);
		}
	}
}
