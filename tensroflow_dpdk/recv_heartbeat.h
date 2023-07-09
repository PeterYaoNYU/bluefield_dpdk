#include "common.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>

#define QUEUE_NAME "/ml_train"

int lcore_recv_heartbeat_pkt(struct recv_arg *);
