#include "common.h"

int lcore_send_heartbeat_pkt(struct lcore_params *p, uint64_t hb_id, struct rte_mbuf *pkts);

__rte_noreturn int lcore_mainloop_send_heartbeat(struct lcore_params *p);
