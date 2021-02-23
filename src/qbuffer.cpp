/*
 * sockdata.cpp
 *
 *  Created on: Oct 25, 2012
 *      Author: root
 */

#include "qstdinc.h"
#include "qbuffer.h"

//////////////////////////////////////////////////////
// Socket data
//////////////////////////////////////////////////////
QBuffer::QBuffer()
{
	Clean();
}

QBuffer::QBuffer(uint8_t* pData, int nSize)
{
	Clean();
	Add(pData, nSize);
}

QBuffer::~QBuffer()
{
}

void QBuffer::Add(uint8_t* pData, int nSize)
{
	for (int i=0; i<nSize; i++)
	{
		m_data.push_back(pData[i]);
	}
}

void QBuffer::Add(QByteArray* pData)
{
	for (int i=0; i<(int)pData->size(); i++)
	{
		m_data.push_back(pData->at(i));
	}
}

void QBuffer::Add(QBuffer* pData)
{
	for (int i=0; i<(int)pData->m_data.size(); i++)
	{
		m_data.push_back(pData->m_data.at(i));
	}
}

bool QBuffer::Remove(int nFrom, int nCount, QByteArray* pData)
{
	for (int i=0; i<nCount; i++)
	{
		if (pData)
		{
			pData->push_back(m_data[nFrom + i]);
		}
		m_data.erase(m_data.begin() + nFrom);
	}
	return true;
}

void QBuffer::Clean()
{
	m_data.clear();
}
