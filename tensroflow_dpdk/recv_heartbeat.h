#include "common.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>

#define QUEUE_NAME      "/train_data"
#define CONTROL_MQ_NAME "/ctrl_msg"
#define INFER_MQ_NAME   "/infer_data"
#define RESULT_MQ_NAME  "/result_mq"

int lcore_recv_heartbeat_pkt(struct recv_arg *);
