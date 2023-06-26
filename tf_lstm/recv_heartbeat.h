#include "common.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define QUEUE_NAME "/ml_data"

int lcore_recv_heartbeat_pkt(struct recv_arg *);

