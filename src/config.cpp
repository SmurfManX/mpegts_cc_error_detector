#include "qstdinc.h"
#include "config.h"

void init_config(TSRecvConfig *pCfg) 
{
    pCfg->output = NULL;
    pCfg->mcast = NULL;
    pCfg->port = 0;
    pCfg->rtp = false;
}

void usage()
{
    printf("Usage: ccdetect [-m <mcast>] [-p <port>] [-o <file>] [-r] [-h]\n");
    printf("    -m <mcast>     # multicast address of channel\n");
    printf("    -p <port>      # port of channel\n");
    printf("    -o <file>      # output file path\n");
    printf("    -r             # if stream is RTP\n");
    printf("    -h             # prints help (this message)\n");
}

void parse_arguments(int argc, char *argv[], TSRecvConfig *pCfg)
{
    int c;
    while ( (c = getopt(argc, argv, "r1:m:p:o:h1")) != -1) {
        switch (c) {
        case 'r':
            pCfg->rtp = true;
            break;
        case 'o':
            pCfg->output = optarg;
            break;
        case 'm':
            pCfg->mcast = optarg;
            break;
        case 'p':
            pCfg->port = atoi(optarg);
            break;
        case 'h':
        default:
            usage();
            exit(0);
        }
    }

    if (!pCfg->mcast || !pCfg->port)
    {
        usage();
        exit(0);
    }
}
