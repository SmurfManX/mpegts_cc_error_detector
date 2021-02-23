/*
 * qbuffer.h
 *
 *  Created on: Oct 25, 2012
 *      Author: root
 */

#ifndef QBUFFER_H_
#define QBUFFER_H_

#include "qstdinc.h"
#include "qstdvectors.h"

class QBuffer
{
public:
	/* Construction */
	QBuffer();
	QBuffer(uint8_t*, int);
	virtual ~QBuffer();

	/* Control data indirect */
	void Add(uint8_t* pData, int nSize);
	void Add(QByteArray* pData);
	void Add(QBuffer* pData);
	bool Remove(int nFrom, int nCount, QByteArray* pData);
	void Clean();

	/* Members */
	QByteArray	m_data;
};

#endif /* SOCKDATA_H_ */
