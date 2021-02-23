//
// C++ Interface: genericsync
//
// Description: 
//	Generic (base) syncronization classes
//
// Author: nlm <nlm@hsc.ge>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef __Q_GENSYNC_H__
#define __Q_GENSYNC_H__

#include "qstdinc.h"

/*
	Base class with sync objects
*/
class QBaseSync
{
public:
	QBaseSync() {}
	virtual ~QBaseSync() {}

	virtual void SyncLock() = 0;
	virtual void SyncUnlock() = 0;
};

/*
	Class with mutex object
*/
class QMutexSync : public QBaseSync
{
public:
	QMutexSync();
	virtual ~QMutexSync();

	virtual void SyncLock();
	virtual void SyncUnlock();
	
protected:
	pthread_mutex_t		m_mutex;
};

/*
	Class emulating bar
*/
class QBar : public QMutexSync
{
public:
	QBar();
	virtual ~QBar();
	
	virtual void Bar();
	virtual void ReleaseBar();
	virtual bool WaitBarAck();
	
	virtual bool IsBarred();
	virtual bool IsBarredNL();
	virtual void AckBar();
	
protected:
	bool	m_bBar;
	bool	m_bAck;
};

/*
	Class used to lock and unlock objects
*/
class QLocker
{
public:
	QLocker(QBaseSync* pSync)
	{ 
		m_pSync = pSync; 
		m_pSync->SyncLock();
	}

	virtual ~QLocker()
	{ 
		m_pSync->SyncUnlock();
	}
protected:

	QBaseSync*	m_pSync;
};

#endif
