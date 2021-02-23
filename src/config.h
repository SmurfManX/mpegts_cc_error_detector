#ifndef __LIBCFG_H__
#define __LIBCFG_H__

#include "qstdinc.h"

typedef struct {
    char* output;
    char* mcast;
    int port;
    bool rtp;
} TSRecvConfig;

void usage();
void init_config(TSRecvConfig *pCfg);
void parse_arguments(int argc, char *argv[], TSRecvConfig *pCfg);

#endif