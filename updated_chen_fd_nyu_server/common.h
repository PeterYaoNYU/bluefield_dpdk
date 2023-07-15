#ifndef __RING_COMMON_H_
#define __RING_COMMON_H_

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_cycles.h>
#include <rte_timer.h>
#include <rte_debug.h>
#include <rte_log.h>

#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include "util.h"

// self defined log type
#define RTE_LOGTYPE_FD_OUTPUT       RTE_LOGTYPE_USER1
#define RTE_LOGTYPE_DEFAULT_DEBUG   RTE_LOGTYPE_USER2
#define RTE_LOGTYPE_SYS_INFO        RTE_LOGTYPE_USER3


#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS ((64*1024)-1)
#define MBUF_CACHE_SIZE 128
#define SCHED_RX_RING_SZ 8192
#define SCHED_TX_RING_SZ 65536

#define NUM_RX_QUEUE 1
#define NUM_TX_QUEUE 1 

#define BURST_SIZE 32
#define BURST_SIZE_TX 1

// how many past packets you want to keep track of
#define HEARTBEAT_N 50

// uncomment the following two lines based on which node this code is running on
#define NODE_1_IP "10.10.1.1"
#define NODE_2_IP "10.10.1.2"

// the port that I am expecting to receive the other node's heartbeat from
#define HB_SRC_PORT 6666

// the heartbeat interval defined here, in millisecond. 
#define DELTA_I 100

#define SAFETY_MARGIN 300

struct lcore_params {
    uint16_t rx_queue_id;
    uint16_t tx_queue_id;
    struct rte_mempool *mem_pool;
};

// this is the structure in the packet sent to the other side 
struct payload{
    uint64_t heartbeat_id;
};


struct hb_timestamp {
    uint64_t heartbeat_id;
    uint64_t hb_timestamp;
};

#define ARR_SIZE HEARTBEAT_N

struct fd_info {
    // heartbeat interval in melli-second
    int delta_i;
    // estimated arrival time for the next heartbeat
    uint64_t ea;
    // // keep track of the last n packets
    // uint16_t n;

    // keep track of the last n timestamps
    // +2 to make the update procedure smoother
    struct hb_timestamp arr_timestamp[ARR_SIZE];

    // the timestamp that got evicted
    uint64_t evicted_time;

    int next_evicted;
    int next_avail;
};



// the structure that is passed to the recv loop function as a parameter
// it should be a collection of fd_info, lcore_params, and a timer, all three are pointers
// to the corresponding structure
struct recv_arg {
    struct lcore_params * p;
    struct fd_info * fdinfo;
    struct rte_timer * t;
};

#endif