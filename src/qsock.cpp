
//
// C++ Implementation: decoder
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
#include "qsock.h"
#include "qlogger.h"

QLOG_REGISTER_SOURCE

QNetAddr::QNetAddr()
{
	Reset();								// Assign later
}

QNetAddr::~QNetAddr()
{
	/* Do nothing */
}

QNetAddr::QNetAddr(const char* pAddr)
{
	SetAddr(pAddr);
}

QNetAddr::QNetAddr(const char* pAddr, uint16_t nPort)
{
	SetAddr(pAddr, nPort);
}

QNetAddr::QNetAddr(uint32_t nAddr, uint16_t nPort)
{
	SetAddr(nAddr, nPort);
}

QNetAddr::QNetAddr(int nSock)
{
	SetAddr(nSock);
}

bool QNetAddr::SetAddr(const char* pAddr)
{
	const char* pDL = strchr(pAddr, ':');
	if (pDL == NULL)
	{
		Reset();
	}
	else
	{
		/* Populate to vars */
		m_sIPOnly.assign(pAddr, pDL-pAddr);
		SetAddr(m_sIPOnly.c_str(), atol(++pDL));
	}
	return m_bValid;
}

bool QNetAddr::SetAddr(const char* pAddr, uint16_t nPort)
{
	SetAddr(inet_addr(pAddr), nPort);
	return m_bValid;
}

bool QNetAddr::SetAddr(uint32_t nAddr, uint16_t nPort)
{
	m_nAddr = nAddr;
	m_nPort = nPort;
	m_bValid = true;
	Expand();
	return m_bValid;
}

void QNetAddr::Expand()
{
	in_addr addr;
	addr.s_addr = m_nAddr;
	m_sIPOnly.assign(inet_ntoa(addr));
	std::ostringstream stringStream;
	stringStream << m_sIPOnly << ":" << m_nPort;
	m_sFullAddr = stringStream.str();
}

bool QNetAddr::SetAddr(int nSock, bool bPeer)
{
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	int nRetVal = bPeer?getpeername(nSock, (struct sockaddr *)&sin, &len):getsockname(nSock, (struct sockaddr *)&sin, &len);
	if (nRetVal == -1)
	{
		Reset();
	}
	else
	{
		SetAddr(sin.sin_addr.s_addr, htons(sin.sin_port));
	}

	return m_bValid;
}

bool QNetAddr::IsValid()
{
	return m_bValid;
}

uint32_t QNetAddr::GetAddr()
{
	return m_nAddr;
}

uint16_t QNetAddr::GetPort()
{
	return m_nPort;
}

const char* QNetAddr::GetAddrFull()
{
	return m_sFullAddr.c_str();
}

const char* QNetAddr::GetAddrIP()
{
	return m_sIPOnly.c_str();
}

void QNetAddr::Reset()
{
	m_bValid = false;
	m_nAddr = INADDR_ANY;
	m_nPort = 0;
	m_sFullAddr.assign("0.0.0.0:0");
	m_sIPOnly.assign("0.0.0.0");
}

QNetAddr* QNetAddr::Duplicate()
{
	QNetAddr* pRetVal = new QNetAddr();

	CopyTo(pRetVal);

	return pRetVal;
}

void QNetAddr::CopyTo(QNetAddr* pDst)
{
	pDst->m_bValid = m_bValid;
	pDst->m_nAddr = m_nAddr;
	pDst->m_nPort = m_nPort;
	pDst->m_sFullAddr = m_sFullAddr;
	pDst->m_sIPOnly = m_sIPOnly;
}

QTCPSock::QTCPSock(QNetAddr* pAddr)
{
	pAddr->CopyTo(&m_bind);

	m_nError = 0;
	m_sock = -1;
	m_bTimed = false;
	m_nTimeOut = 60000;
}

QTCPSock::QTCPSock(int sock)
{
	m_nError = 0;
	m_sock = sock;
	m_bTimed = false;
	m_nTimeOut = 60000;
	m_bind.SetAddr(sock);
}

QTCPSock::~QTCPSock()
{
	Close();
}

bool QTCPSock::IsOpen()
{
	return m_sock >= 0;
}

void QTCPSock::Close()
{
	if (m_sock >= 0)
	{
		shutdown(m_sock, SHUT_RDWR);
		close(m_sock);
	}
	m_sock = -1;
}

bool QTCPSock::SendByte(uint8_t nData)
{
	return GenSend(&nData, sizeof(nData));
}

bool QTCPSock::SendWord(uint16_t nData)
{
	return GenSend(&nData, sizeof(nData));
}

bool QTCPSock::SendDWord(uint32_t nData)
{
	return GenSend(&nData, sizeof(nData));
}

bool QTCPSock::SendQWord(uint64_t nData)
{
	return GenSend(&nData, sizeof(nData));
}

bool QTCPSock::SendString(const char* pData)
{
	uint16_t nLen = strlen(pData) + 1;
	if (!SendWord(nLen)) return false;
	return GenSend(( void*) pData, nLen);
}

bool QTCPSock::SendBool(bool bData)
{
	uint16_t nData = bData?(uint8_t)1:(uint8_t)0;
	return SendWord(nData);
}

bool QTCPSock::SendFloat(float fData)
{
	char buff[32];
	sprintf(buff, "%f", fData);
	return SendString(buff);
}

bool QTCPSock::SendStr(std::string& str)
{
	uint16_t nSize = str.length();
	if (!SendWord(nSize)) return false;
	if (nSize == 0) return true;
	return GenSend((void*)str.data(), nSize);
}

bool QTCPSock::RecvByte(uint8_t& nData)
{
	return GenRecv(&nData, sizeof(nData));
}

bool QTCPSock::RecvWord(uint16_t& nData)
{
	return GenRecv(&nData, sizeof(nData));
}

bool QTCPSock::RecvDWord(uint32_t& nData)
{
	return GenRecv(&nData, sizeof(nData));
}

bool QTCPSock::RecvQWord(uint64_t& nData)
{
	return GenRecv(&nData, sizeof(nData));
}

bool QTCPSock::RecvString(char* pData)
{
	uint16_t nLen = 0;
	if (!RecvWord(nLen)) return false;
	return GenRecv(pData, nLen);
}

bool QTCPSock::RecvBool(bool& bData)
{
	uint16_t nData = bData?(uint16_t)1:(uint16_t)0;
	if (!RecvWord(nData)) return false;
	bData = nData?true:false;
	return true;
}

bool QTCPSock::RecvFloat(float& fData)
{
	char buff[32];
	float fTmp;
	if (!RecvString(buff))
	{
		return false;
	}
	sscanf(buff, "%f", &fTmp);
	fData = fTmp;
	return true;
}

bool QTCPSock::RecvStr(std::string& str)
{
	uint16_t nSize;
	if (!RecvWord(nSize)) return false;
	if (nSize == 0)
	{
		str.clear();
		return true;		
	}
	str.resize(nSize, 0);
	if (!GenRecv((void*)str.data(), nSize)) return false;
	str = str.c_str();
	return true;
}

bool QTCPSock::GenRecv(void* pData, int nLen)
{
	if (m_bTimed)
	{
		return GenRecvTimed(pData, nLen, m_nTimeOut);
	}

	int nDone = 0;
	char* buf =(char*) pData;
	if (nLen == 0) return true;

	while(nDone < nLen)
	{
		int nChunk = std::min(nLen - nDone, 32 * 1024);

		int nRecv = recv(m_sock, &buf[nDone], nChunk, MSG_NOSIGNAL);
		if (nRecv <= 0)
		{
			if (nRecv == 0)
			{
				m_nError = -1;
			}
			else
			{
				m_nError = errno;
			}
			Close();
			return false;
		}
		nDone += nRecv;
	}
	return true;
}

bool QTCPSock::GenRecvTimed(void* pData, int nLen, int nTimeOut)
{
	int nDone = 0;
	char* buf =(char*) pData;
	if (nLen == 0) return true;

	while(nDone < nLen)
	{
		int nChunk = std::min(nLen - nDone, 32 * 1024);

		pollfd fd;
		fd.fd = m_sock;
		fd.events = POLLIN | POLLERR;
		fd.revents = 0;
	
		int nRetVal = poll(&fd, 1, nTimeOut);
	
		if (nRetVal < 0)
		{
			m_nError = -2;
			Close();
			return false;
		}
		else if (nRetVal == 0)
		{
			m_nError = -3;
			Close();
			return false;
		}

		int nRecv = recv(m_sock, &buf[nDone], nChunk, MSG_NOSIGNAL);
		if (nRecv <= 0)
		{
			if (nRecv == 0)
			{
				m_nError = -4;
			}
			else
			{
				m_nError = errno;
			}
			Close();
			return false;
		}
		nDone += nRecv;
	}
	return true;
}

bool QTCPSock::GenSend(void* pData, int nLen)
{
	if (m_bTimed)
	{
		return GenSendTimed(pData, nLen, m_nTimeOut);
	}

	int nDone = 0;
	char* buf =(char*) pData;
	if (nLen == 0) return true;

	while(nDone < nLen)
	{
		/* Send */
		int nChunk = std::min(nLen - nDone, 32 * 1024);
		int nSent = send(m_sock, &buf[nDone], nChunk, MSG_NOSIGNAL);
		if (nSent <= 0)
		{
			if (nSent == 0)
			{
				m_nError = -5;
			}
			else
			{
				m_nError = errno;
			}
			Close();
			return false;
		}
		nDone += nSent;
	}
	return true;
}

bool QTCPSock::GenSendTimed(void* pData, int nLen, int nTimeOut)
{
	int nDone = 0;
	char* buf =(char*) pData;
	if (nLen == 0) return true;

	while(nDone < nLen)
	{
		int nChunk = std::min(nLen - nDone, 32 * 1024);
		pollfd fd;
		fd.fd = m_sock;
		fd.events = POLLOUT | POLLERR;
		fd.revents = 0;
	
		int nRetVal = poll(&fd, 1, nTimeOut);
	
		if (nRetVal < 0)
		{
			m_nError = -6;
			Close();
			return false;
		}
		else if (nRetVal == 0)
		{
			m_nError = -7;
			Close();
			return false;
		}

		int nSent = send(m_sock, &buf[nDone], nChunk, MSG_NOSIGNAL);
		if (nSent <= 0)
		{
			if (nSent == 0)
			{
				m_nError = -7;
			}
			else
			{
				m_nError = errno;
			}
			Close();
			return false;
		}
		nDone += nSent;
	}
	return true;
}

bool QTCPSock::Open()
{
	// Set socket buffer size
	if (m_sock >= 0)
	{
		m_nError = 0;
		QLogWarning(0, "Trying to open already opened socket, ignored");
		return true;
	}

	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)
	{
		m_nError = errno;
		QLogError(0, "Can not open socket: %d", m_nError);
		return false;
	}

	int opt = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		m_nError = errno;
		QLogError(0, "Can not set SO_REUSEADDR on socket: %d", m_nError);

		Close();
		return false;
	}
	linger lopt;
	lopt.l_onoff = 1;
	lopt.l_linger = 0;
	if (setsockopt(m_sock, SOL_SOCKET, SO_LINGER, &lopt, sizeof(lopt)))
	{
		m_nError = errno;
		QLogError(0, "Can not set SO_LINGER on socket: %d", m_nError);

		Close();
		return false;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0;
	addr.sin_port = 0;
	if (bind(m_sock,(sockaddr *) &addr, sizeof(addr)))
	{
		m_nError = errno;
		QLogError(0, "Can not set bind socket: %d", m_nError);

		Close();
		return false;
	}

	opt = 1;
	if (setsockopt(m_sock, IPPROTO_TCP, TCP_SYNCNT, (char*)&opt, sizeof(opt)))
	{
		m_nError = errno;
		QLogError(0, "Can not set TCP_SYNCNT on socket: %d", m_nError);

		Close();
		return false;
	}

	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = m_bind.GetAddr();
	saddr.sin_port = htons(m_bind.GetPort());

	if (connect(m_sock,(sockaddr*)&saddr, sizeof(saddr)))
	{
		m_nError = errno;
		QLogWarning(0, "Can not connect to remote socket: %d", m_nError);

		Close();
		return false;
	}

	m_nError = 0;
	return true;
}

bool QTCPSock::GetInputSize(int* pAvail)
{
	if (!IsOpen()) return false;
	if (ioctl(m_sock, FIONREAD, pAvail))
	{
		m_nError = errno;
		QLogError(0, "Can not FIONREAD on socket: %d", m_nError);

		Close();
		return false;
	}
	return true;
}

void QTCPSock::SetNodelay()
{
	int nFlags = 1;
	if (setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&nFlags, sizeof(nFlags)) == -1)
	{
		m_nError = errno;
		QLogError(0, "Can not set TCP_NODELAY on socket: %d", m_nError);
		Close();
	}
}

void QTCPSock::SetCork(bool bSet)
{
	int nFlags = bSet?1:0;
	if (setsockopt(m_sock, IPPROTO_TCP, TCP_CORK, (char*)&nFlags, sizeof(nFlags)) == -1)
	{
		m_nError = errno;
		QLogError(0, "Can not set TCP_CORK on socket: %d", m_nError);
		Close();
	}
}
bool QTCPSock::SetOTTBuffers()
{
	int nNewSize = 1024 * 16; /* 72 Kb buffer size */
	int size = nNewSize;
	socklen_t len = sizeof(size);

	if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &size, len) != 0)
	{
		QLogError(0, "Error setting receive buffer %d", errno);
		Close();
		return false;
	}

	if (setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, &size, len) != 0)
	{
		QLogError(0, "Error setting send buffer %d", errno);
		Close();
		return false;
	}

	if (getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &size, &len) != 0)
	{
		QLogError(0, "Error setting receive buffer %d", errno);
		Close();
		return false;
	}
	else
	{
		QLogError(0, "[DBG] Recv buffer size: %d", size);
	}

	if (getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, &size, &len) != 0)
	{
		QLogError(0, "Error setting send buffer %d", errno);
		Close();
		return false;
	}
	else
	{
		QLogError(0, "[DBG] Send buffer size: %d", size);
	}

	return true;
}

QNetAddr* QTCPSock::GetBounds()
{
	return &m_bind;
}

bool QTCPSock::GetPeer(QNetAddr* pAddr)
{
	return pAddr->SetAddr(m_sock, true);
}

int QTCPSock::GetTimeOut()
{
	return m_nTimeOut;
}

int QTCPSock::GetError()
{
	return m_nError;
}

int QTCPSock::GetSock()
{
	return m_sock;
}

int QTCPSock::SetSock(int nSock)
{
	int nRetVal = m_sock;
	m_sock = nSock;
	m_bind.SetAddr(m_sock);
	return nRetVal;
}

void QTCPSock::SetTimed(bool bTimed)
{
	m_bTimed = bTimed;
}

void QTCPSock::SetTimeOut(int nTimeout)
{
	m_nTimeOut = nTimeout;
}

void QTCPSock::SetHWKA()
{
	int nKeepAlive = 1;
	int nKeepCount = 5;
	int nKeepIdle = 30;
	int nKeepInt = 5;

	if (!IsOpen())
	{
		return;
	}

	if (setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, &nKeepAlive, sizeof (nKeepAlive)))
	{
		m_nError = errno;
		QLogError(0, "Could not set SO_KEEPALIVE on incoming socket: %d", m_nError);
		Close();
		return;
	}

	if (setsockopt(m_sock, IPPROTO_TCP, TCP_KEEPCNT, &nKeepCount, sizeof (nKeepCount)))
	{
		m_nError = errno;
		QLogError(0, "Could not set TCP_KEEPCNT on incoming socket: %d", errno);
		Close();
		return;
	}

	if (setsockopt(m_sock, IPPROTO_TCP, TCP_KEEPIDLE, &nKeepIdle, sizeof (nKeepIdle)))
	{
		m_nError = errno;
		QLogError(0, "Could not set TCP_KEEPIDLE on incoming socket: %d", errno);
		Close();
		return;
	}

	if (setsockopt(m_sock, IPPROTO_TCP, TCP_KEEPINTVL, &nKeepInt, sizeof (nKeepInt)))
	{
		m_nError = errno;
		QLogError(0, "Could not set TCP_KEEPINTVL on incoming socket: %d", errno);
		Close();
		return;
	}
}

bool QTCPSock::CheckConnect()
{
	if (!IsOpen()) return false;

	// Now do POLL
	pollfd fd;
	fd.fd = m_sock;
	fd.events = POLLERR | POLLRDHUP | POLLHUP | POLLNVAL;
	fd.revents = 0;

	int nRetVal = poll(&fd, 1, 0);

	if (nRetVal < 0)
	{
		m_nError = errno;
		Close();
		return false;
	}
	else if (nRetVal > 0)
	{
		m_nError = 0;
		Close();
		return false;
	}
	return true;
}

bool QTCPSock::GetTimed()
{
	return m_bTimed;
}

