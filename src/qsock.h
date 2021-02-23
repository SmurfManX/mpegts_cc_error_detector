#ifndef __Q_SOCK2_H__
#define __Q_SOCK2_H__

#include "qstdinc.h"

/* Address class */
class QNetAddr
{
public:
	/* Constructors */
	QNetAddr();										// Assign later
	QNetAddr(const char*);							// Use address:port
	QNetAddr(const char*, uint16_t);				// Use address, port
	QNetAddr(uint32_t, uint16_t);					// Use address, port
	QNetAddr(int);									// Use bound socket
	virtual ~QNetAddr();

	/* Same as constructors */
	virtual bool SetAddr(const char*);				// Use address:port
	virtual bool SetAddr(const char*, uint16_t);	// Use address,port
	virtual bool SetAddr(uint32_t, uint16_t);		// Use address,port
	virtual bool SetAddr(int, bool bPeer = false);	// Use bound socket, true - local, false - remote

	/* Is address valid? */
	virtual bool IsValid();

	/* Access members */
	virtual uint32_t GetAddr();
	virtual uint16_t GetPort();
	virtual const char* GetAddrFull();				// In form adress:port
	virtual const char* GetAddrIP();				// IP address only

	/* Make copy */
	virtual QNetAddr* Duplicate();
	virtual void CopyTo(QNetAddr*);

protected:
	/* Reset all info */
	virtual void Reset();

	/* Set string variants */
	virtual void Expand();

	bool		m_bValid;
	uint32_t	m_nAddr;					// IP address
	uint16_t	m_nPort;					// Port value
	std::string m_sFullAddr;				// Full address
	std::string m_sIPOnly;					// IP only address
};


class QTCPSock
{
public:
	QTCPSock(QNetAddr*);
	QTCPSock(int);
	virtual ~QTCPSock();

	/* Return bounding address of socket */
	virtual QNetAddr* GetBounds();

	/* Return remote peer information */
	virtual bool GetPeer(QNetAddr*);

	/* Open/close socket (connect/disconnect) */
	virtual bool Open();
	virtual void Close();

	/* Get/Set socket, return previous one */
	virtual int SetSock(int nSock);
	virtual int GetSock();

	/* Is socket open? */
	virtual bool IsOpen();

	/* Generic send/receive */
	virtual bool GenRecv(void* pData, int nLen);
	virtual bool GenRecvTimed(void* pData, int nLen, int nTimeOut);
	virtual bool GenSend(void* pData, int nLen);
	virtual bool GenSendTimed(void* pData, int nLen, int nTimeOut);

	/* Timeout operations */
	virtual void SetTimeOut(int nTimeOut);
	virtual int GetTimeOut();
	virtual void SetTimed(bool bTimed);
	virtual bool GetTimed();

	/* Socket option setup */
	virtual void SetNodelay();
	virtual void SetCork(bool);
	virtual void SetHWKA();
	virtual bool SetOTTBuffers();

	/* Some ping to check socket is alive */
	virtual bool CheckConnect();

	/* Get available bytes in input buffer */
	virtual bool GetInputSize(int*);

	/* Send/receive typed functions */
	virtual bool SendByte(uint8_t);
	virtual bool SendWord(uint16_t);
	virtual bool SendDWord(uint32_t);
	virtual bool SendQWord(uint64_t);
	virtual bool SendString(const char*);
	virtual bool SendBool(bool);
	virtual bool SendFloat(float);
	virtual bool SendStr(std::string&);

	virtual bool RecvByte(uint8_t&);
	virtual bool RecvWord(uint16_t&);
	virtual bool RecvDWord(uint32_t&);
	virtual bool RecvQWord(uint64_t&);
	virtual bool RecvString(char*);
	virtual bool RecvBool(bool&);
	virtual bool RecvFloat(float&);
	virtual bool RecvStr(std::string&);

	/* Last error happened with socket */
	virtual int GetError();
protected:
	int				m_sock;
	int				m_nError;
	bool			m_bTimed;
	int				m_nTimeOut;
	QNetAddr		m_bind;
};

#endif
