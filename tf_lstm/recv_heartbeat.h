#include "common.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>

#define QUEUE_NAME "/ml_data"

mqd_t mq;

int lcore_recv_heartbeat_pkt(struct recv_arg *);

