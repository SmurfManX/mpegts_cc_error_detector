#include "qstdinc.h"
#include "qlogger.h"
#include "qsock.h"
#include "qbuffer.h"
#include "qdvbts.h"
#include "config.h"

QLOG_REGISTER_SOURCE

using namespace std;

#define RTP_FRAME_SIZE (188 * 7 + 12)
#define ITG_FRAME_SIZE (188 * 7)

#define BUF_SIZE (32 * 1024)
#define DEFAULT_AUDIT_INTERVAL 3
#define BYTES_TO_COLLECT 100000

typedef std::map<uint16_t, uint64_t> PIDMap;

static int QAXReceiveStatic(int nSock, struct timespec* pTime, uint8_t* pBase, int nSize)
{
    struct msghdr msg;
    struct cmsghdr *cmsg;
    unsigned char ctlbuf[4096];
    struct iovec iov;

    /* Read clock, we could end up without clock eventually */
    clock_gettime(CLOCK_REALTIME, pTime);

    /* Init msg */
    memset(&msg, 0, sizeof(msg));

    /* Setup structs */
    iov.iov_base = pBase;
    iov.iov_len = nSize;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctlbuf;
    msg.msg_controllen = sizeof(ctlbuf);
    msg.msg_flags = 0 ;

    /* Receive */
    int nRetVal = recvmsg ( nSock, &msg, 0 );

    /* Check result */
    if ( nRetVal <= 0 )
    {
        return 0;
    }

    /* Analyse data */
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg!=NULL; cmsg = CMSG_NXTHDR(&msg,cmsg))
    {
        if ((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SO_TIMESTAMPNS))
        {
            struct timespec *stamp = (struct timespec *)CMSG_DATA(cmsg);
            if (stamp)
            {
                *pTime = *stamp;
            }
        }
        else if ((cmsg->cmsg_level == SOL_SOCKET) && (cmsg->cmsg_type == SO_RXQ_OVFL))
        {
        }
        else if ((cmsg->cmsg_level == IPPROTO_IP) && (cmsg->cmsg_type == IP_RECVERR))
        {
        }
    }

    return nRetVal;
}

bool STBReceiveData ( uint8_t* buf, size_t& nRecv, int sock, bool bRTP, struct timespec* pTime )
{
    uint8_t buf_rtp[RTP_FRAME_SIZE];

    nRecv = 0;

    pollfd fd;
    fd.fd = sock;
    fd.events = POLLIN;
    fd.revents = 0;

    int nRetVal = poll(&fd, 1, 2000);

    if (nRetVal <= 0)
    {
        return false;
    }

    if (bRTP)
    {
        nRecv = QAXReceiveStatic(sock, pTime, buf_rtp, RTP_FRAME_SIZE);

        if ( nRecv !=  RTP_FRAME_SIZE)
        {
            //return false;
        }

        memcpy(buf, &buf_rtp[12], ITG_FRAME_SIZE);
        nRecv = ITG_FRAME_SIZE;
    }
    else
    {
        nRecv = QAXReceiveStatic(sock, pTime, buf, ITG_FRAME_SIZE);
    }

    return true;
}

int STBCreateSocket(uint32_t nAddr, uint16_t nPort)
{
    int size = (1 * 1024 * 1024);
    socklen_t len = sizeof(size);

    int m_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_sock < 0)
    {
        QLogError(0, "BAD SOCKET (%d): %d", m_sock, errno);
        return -1;
    }

    int opt = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        QLogError(0, "Can not set REUSE parameters");
        return -1;
    }

    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &size, len) != 0)
    {
        QLogError(0, "Can not set UDP socket buffer size");
        return -1;
    }

    size = 0;
    if (getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, &size, &len) != 0)
    {
        QLogError(0, "Can not get UDP socket buffer size");
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) != 0)
    {
        QLogError(0, "Can not set SO_RCVTIMEO");
        return -1;	
    }

    int	oval = 1;
    int	olen = sizeof( oval );
    if (setsockopt(m_sock, SOL_SOCKET, SO_TIMESTAMPNS, &oval, olen ) != 0)
    {
        QLogError(0, "Can not set SO_TIMESTAMP");
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = nAddr;
    addr.sin_port = htons(nPort);
    if (bind(m_sock,(sockaddr *) &addr, sizeof(addr)))
    {
        QLogError(0, "Can not bind UDP socket");
        return -1;
    }

    ip_mreq m_req;
    m_req.imr_multiaddr.s_addr = nAddr;
    m_req.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,(void *) &m_req, sizeof(m_req)) != 0)
    {
        QLogError(0, "Can not join multicast");
        close(m_sock);
        return -1;
    }

    return m_sock;
}

void append_file(const char *pFile, const char *pData)
{
    if (pFile == NULL) return;
    FILE *fp = fopen(pFile, "a");
    if (fp == NULL) return;
    fprintf(fp, "%s\n", pData);
    fclose(fp);
}

int QReceiver (TSRecvConfig *pCfg)
{
    timespec tRecvTime;
    int nSID = -1;
    uint64_t nVCC = 0;
    bool bReceived = false;

    int nInSock = STBCreateSocket(inet_addr(pCfg->mcast), pCfg->port);
    if (nInSock < 0) return -1;

    QTCPSock sockAD(nInSock);

    QLogInfo ( 0, "Starting reception..." );
    PIDMap pidMap;

    while ( true )
    {
        uint8_t tsbuffer[RTP_FRAME_SIZE];
        size_t nRecv = 0;
        memset(tsbuffer, -1, sizeof(tsbuffer));

        if ( !STBReceiveData ( tsbuffer, nRecv, nInSock, pCfg->rtp, &tRecvTime ) )
        {
            QLogInfo(0, "Channel timed out for 2 seconds...");
            usleep ( 10000 );

            bReceived = false;
            continue;
        }

        if (!bReceived)
        {
            QLogInfo(0, "Started multicast stream");
            bReceived = true;
        }

        int nCount = nRecv / 188;
        for (int i=0; i < nCount; i++)
        {
            /* Check for TS */
            if (tsbuffer[i * 188] != 'G')
            {
                QLogInfo(0, "Not a TS stream (%d / %d)", nRecv, nCount);
                exit(-1);
            }

            /* Parse TS */
            QDVBTSPacket ts;
            if (QDVBParseTS(&tsbuffer[i * 188], &ts))
            {
                auto it = pidMap.find(ts.PID);
                if (it != pidMap.end())
                {
                    if (it->second != ts.continuity_counter)
                    {
                        uint8_t nNewVal =  (it->second + 1) % 16;

                        if (nNewVal != ts.continuity_counter)
                        {
                            char sOutput[1024];
                            snprintf(sOutput, sizeof(sOutput), 
                                "CC ERROR: PID(%d), MUSTBE(%d), RECEIVED(%d)", 
                                ts.PID, nNewVal, ts.continuity_counter);

                            QLogInfo(0, "%s", sOutput);                                
                            append_file(pCfg->output, sOutput);
                        }
                    }
                }

                pidMap[ts.PID] = ts.continuity_counter;
            }
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{
    /* Log works */
    QLogEnableScreen(true);
    QLogEnableFile(false);

    /* Init config variables */
    TSRecvConfig cfg;
    init_config(&cfg);
 
    /* Parse Commandline arguments */
    parse_arguments(argc, argv, &cfg);

    /* Start receiver */
    QReceiver(&cfg);

    return 0;
}
