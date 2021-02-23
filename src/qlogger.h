/***************************************************************************
 *   Copyright (C) 2008 by root   *
 *   root@skystar   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef __Q_LOGGER_H__
#define __Q_LOGGER_H__

#include "qstdinc.h"
#include "qstdvectors.h"

/* Per file log manager struct */
/* Qarva Logging Engine Source Control */
struct QLESC
{
	std::string			sSourceFile;		/* Source file name, __FILE__ usually 	*/
	uint64_t			nSourceID;			/* Allocated and assigned by engine		*/
	volatile bool		bEnabled;			/* Is this source enabled?	D = yes		*/
	volatile uint32_t	nTypeMask;			/* Type mask, D = all					*/
	volatile uint32_t	nLevel;				/* Max level to output, D = 0 / all		*/
};

/* Message types */
#define QLT_VERBOSE		1
#define QLT_DEBUG		2
#define QLT_INFO		4
#define QLT_WARNING		8
#define	QLT_ERROR		16

/* Initialize logger system */
bool QLogInit(bool bScreenLog, bool bFileLog, const char* pBase);

/* Start thread that listens and interprets commands on specified port */
bool QLogConsoleStart(uint16_t);

/* Logging system setup */
void QLogEnable(bool);										/* Disable/Enable logging 					*/
void QLogEnableScreen(bool);								/* Disable/Enable screen logging			*/
void QLogEnableFile(bool);									/* Disable/Enable file logging				*/
void QLogSetDebug(bool);									/* Set debug mode (print file and line)		*/
bool QLogGetDebug();										/* Get debug mode							*/
bool QLogSourceEnable(uint64_t nID, bool bEnable); 			/* Disable/Enable exact source file 		*/
bool QLogSourceMessages(uint64_t nID, uint32_t nMessages); 	/* Set message types for exact source file 	*/
bool QLogSourceLevel(uint64_t nID, uint32_t nLevel); 		/* Max level messages to output 			*/

void QLogGetSetup(bool&, bool&, bool&, std::string&);		/* Enabled, Screen, File, BaseFileName		*/
void QLogGetSourceArray(QQWordArray&);						/* Retrieve all registered IDs				*/
bool QLogGetIDDescr(uint64_t, std::string&, bool&,			/* Retrieve all current data for given ID	*/
		uint32_t&, uint32_t&);

#define QLOG_REGISTER_SOURCE static volatile QLESC* g_pLogSCEntry = NULL;

#define QLogVerbose(A, B, ...) QLogReal(&g_pLogSCEntry, __FILE__, __LINE__, QLT_VERBOSE, A, B, ##__VA_ARGS__);
#define QLogDebug(A, B, ...) QLogReal(&g_pLogSCEntry, __FILE__, __LINE__, QLT_DEBUG, A, B, ##__VA_ARGS__);
#define QLogInfo(A, B, ...) QLogReal(&g_pLogSCEntry, __FILE__, __LINE__, QLT_INFO, A, B, ##__VA_ARGS__);
#define QLogWarning(A, B, ...) QLogReal(&g_pLogSCEntry, __FILE__, __LINE__, QLT_WARNING, A, B, ##__VA_ARGS__);
#define QLogError(A, B, ...) QLogReal(&g_pLogSCEntry, __FILE__, __LINE__, QLT_ERROR, A, B, ##__VA_ARGS__);

/* By default log is enabled, all types are allowed and only level 0 is is logged */
/* Log string */
void QLogReal(volatile QLESC**, const char* pFile, int nLine, int nType, int nLevel, const char* pString, ...);

#endif
