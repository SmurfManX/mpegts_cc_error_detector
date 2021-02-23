//
// C++ Implementation: genericsync
//
// Description:
//
//
// Author: nlm <nlm@hsc.ge>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "qstdinc.h"
#include "qsync.h"
#include "qlogger.h"

QLOG_REGISTER_SOURCE

QMutexSync::QMutexSync()
{
	pthread_mutexattr_t 	m_attr;
	if (pthread_mutexattr_init(&m_attr) ||
		pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE) ||
		pthread_mutex_init(&m_mutex, &m_attr) ||
		pthread_mutexattr_destroy(&m_attr))
	{
		QLogError(0, "Error initializing mutex: %d", errno);
		exit(0);
	}
}

QMutexSync::~QMutexSync()
{
	if (pthread_mutex_destroy(&m_mutex))
	{
		QLogError(0, "Error destroying mutex: %d", errno);
		exit(0);
	}
}

void QMutexSync::SyncLock()
{
	if (pthread_mutex_lock(&m_mutex))
	{
		QLogError(0, "Error locking mutex: %d", errno);
		exit(0);
	}
}

void QMutexSync::SyncUnlock()
{
	if (pthread_mutex_unlock(&m_mutex))
	{
		QLogError(0, "Error unlocking mutex: %d", errno);
		exit(0);
	}
}

QBar::QBar()
{
	m_bBar = m_bAck = false;
}

QBar::~QBar()
{
}
	
void QBar::Bar()
{
	QLocker lock(this);
	m_bBar = true;
	m_bAck = false;
}

void QBar::ReleaseBar()
{
	QLocker lock(this);
	m_bBar = m_bAck = false;
}

bool QBar::WaitBarAck()
{
	bool bRetVal = false;
	
	for (int i=0; i<200; i++)
	{
		{
			QLocker lock(this);
			if (m_bAck)
			{
				bRetVal = true;
				break;
			}
		}
		usleep(10000);
	}
	
	return bRetVal;
}
	
bool QBar::IsBarred()
{
	QLocker lock(this);
	return m_bBar;
}

bool QBar::IsBarredNL()
{
	return m_bBar;
}

void QBar::AckBar()
{
	QLocker lock(this);
	m_bAck = true;
}
