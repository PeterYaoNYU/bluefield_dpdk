#include "recv_heartbeat.h"


static void
timer1_cb(struct rte_timer *tim, void *arg)
{
	unsigned lcore_id = rte_lcore_id();

	uint64_t suspected_time = rte_rdtsc();

	// rewire the timer even for the suspected node
	uint64_t rewired_amount = (uint64_t) arg;

	RTE_LOG(INFO, FD_OUTPUT, "!!%lu!! suspected\n", rewired_amount);
	RTE_LOG(INFO, FD_OUTPUT, "Suspected Time: %lu\n", suspected_time);

	// rte_timer_reset(tim, rewired_amount, SINGLE, lcore_id, timer1_cb, (void *)rewired_amount);
}

// int lcore_recv_heartbeat_pkt(struct lcore_params *p, struct fd_info * fdinfo, struct rte_timer * tim)
int lcore_recv_heartbeat_pkt(struct recv_arg * recv_arg)
{
	// need to make a translation between the number of cycles per second and the number of seconds of our EA
	uint64_t hz_per_sec = rte_get_timer_hz();
	RTE_LOG(INFO, SYS_INFO, "the hz is %lu\n", hz_per_sec);
	// now real_interval is the real number of clock tick between 2 emissions
	uint64_t hz = hz_per_sec * DELTA_I / 1000;

	RTE_LOG(INFO, SYS_INFO, "the real gap of clock ticks is %lu\n", hz);

	// the variable safety margin in terms of clock ticks
	uint64_t safety_margin = hz_per_sec * SAFETY_MARGIN / 1000;

	RTE_LOG(INFO, SYS_INFO, "the safety margin in terms of ticks is %lu\n", safety_margin);

	// this is the number of ticks per second  

	// first, unpack the arguments from recv_arg
	struct lcore_params *p = recv_arg->p;
	// struct fd_info * fdinfo = recv_arg->fdinfo;
	struct rte_timer * tim = recv_arg->t;

	struct fd_info fdinfo = {
		.delta_i = DELTA_I * 1000,
		.ea = 0,
		.evicted_time = 0,
		.next_evicted = 0,
		.next_avail = 0
	};

	memset(fdinfo.arr_timestamp, 0, sizeof(fdinfo.arr_timestamp));

	const int socket_id = rte_socket_id();

	unsigned lcore_id = rte_lcore_id();
	RTE_LOG(INFO, SYS_INFO, "Core %u doing RX dequeue.\n", lcore_id);

	uint64_t pkt_cnt = 0;

	FILE * result_file = fopen("output.txt", "a");

	if (result_file == NULL){
		RTE_LOG(INFO, SYS_INFO, "Failed to open the file.\n");
	}

	while (1){
		struct rte_mbuf *bufs[BURST_SIZE];
		uint16_t nb_rx = rte_eth_rx_burst(0, p->rx_queue_id, bufs, BURST_SIZE);

		// update the states of all timers in the skip list, check for expiration
		rte_timer_manage();

		if (nb_rx == 0){
			continue;
		}

		// printf("received %u packets in this burst\n", nb_rx);
		uint16_t i;
		for (i = 0; i < nb_rx; i++){
            struct rte_mbuf *pkt = bufs[i];

            // unwrap the ethernet layer header
			// I totally forget that there is an rte_ in the name of this structure, which waste me some time finding this bug!!!
			// It keeps telling me that this structure cannot be found in the header file
			// which caused me to rethink whether I have been including header files incorrectly for my entire life...
            struct rte_ether_hdr *eth_hdr =(struct rte_ether_hdr *)rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);
            
            // if this is indeed an IP packet
            if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4) ){
				struct rte_ipv4_hdr * ip_hdr = rte_pktmbuf_mtod_offset(pkt, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
				// if this is a UDP packet and is intended for me and is from someone that I am expecting...
				// if ((ip_hdr->next_proto_id == IPPROTO_UDP) && (ip_hdr->src_addr == string_to_ip(NODE_2_IP)) && (ip_hdr->dst_addr == string_to_ip(NODE_1_IP)) ){
				if (ip_hdr->next_proto_id == IPPROTO_UDP){
					struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
					// if the UDP port matches what I am expecting...
					if (udp_hdr->dst_port == rte_cpu_to_be_16(HB_SRC_PORT)) {
						// update pkt_cnt
						pkt_cnt++;
						// print the packet detail that I have unwrapped so far...
						// TO-DO

						// update the Chen's estimation based on the packet received...
						struct payload * obj= (struct payload *)(udp_hdr + 1);
						uint64_t receipt_time = rte_rdtsc();
						fdinfo.evicted_time = fdinfo.arr_timestamp[fdinfo.next_evicted].hb_timestamp;

						// printf("storing the receipt time into index: %d\n", fdinfo.next_avail);
						RTE_LOG(INFO, SYS_INFO, "Packet reception: %lu\n", receipt_time);

						// fdinfo.arr_timestamp[fdinfo.next_avail] = (struct hb_timestamp) { .heartbeat_id = pkt_cnt, .hb_timestamp = receipt_time};
						fdinfo.arr_timestamp[fdinfo.next_avail].heartbeat_id = pkt_cnt;
						fdinfo.arr_timestamp[fdinfo.next_avail].hb_timestamp = receipt_time;
						// printf("fdinfo + 1: %d, take mod: %d, ARR_SIZE: %d\n", fdinfo.next_avail+1, (fdinfo.next_avail + 1) % 12, ARR_SIZE);

						
						// increment the next_avail variable 
						fdinfo.next_avail = (fdinfo.next_avail + 1) % 50;

						// if (unlikely(pkt_cnt == HEARTBEAT_N)) {
						if (pkt_cnt == HEARTBEAT_N) {
							for (int i = 0; i < ARR_SIZE; i++){
								RTE_LOG(DEBUG, DEFAULT_DEBUG, "%lu: %lu | \n", fdinfo.arr_timestamp[i].heartbeat_id, fdinfo.arr_timestamp[i].hb_timestamp);
							}

							int i;
							uint64_t moving_sum = 0;
							struct hb_timestamp hb;
							for (i = 0; i < HEARTBEAT_N; i++){
								hb = fdinfo.arr_timestamp[i];
								moving_sum += (hb.hb_timestamp - (hb.heartbeat_id-1) * hz);
								// printf("%d: %lu, moving sum: %lu\n", hb.heartbeat_id, (hb.hb_timestamp - hb.heartbeat_id * fdinfo.delta_i * hz / ), moving_sum);
								RTE_LOG(DEBUG, DEFAULT_DEBUG, "%lu: %lu, moving sum: %lu\n", hb.heartbeat_id, (hb.hb_timestamp - (hb.heartbeat_id - 1) * hz), moving_sum);
							}
							fdinfo.ea = moving_sum / HEARTBEAT_N + (HEARTBEAT_N+1) * hz;
							printf("putting the first estimate %lu\n", fdinfo.ea);
							rte_timer_reset(tim, fdinfo.ea - receipt_time + safety_margin, SINGLE, lcore_id, timer1_cb, (void *)(fdinfo.ea - receipt_time + safety_margin));
						} else if (pkt_cnt > HEARTBEAT_N){
							// calculate the new estimeated arrival time 
							fdinfo.ea = fdinfo.ea + ((receipt_time - (fdinfo.evicted_time)) / HEARTBEAT_N);
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "FD: %lu th HB arriving, at time %lu, esti: %lu, evicted: %lu\n", pkt_cnt, receipt_time, fdinfo.ea, fdinfo.evicted_time);

							// update the next_evicted variable
							fdinfo.next_evicted = (fdinfo.next_evicted + 1) % 50;

							if (pkt_cnt == 1500){
								fprintf(result_file, "the time it takes to start suspecting is %lu\n", fdinfo.ea - receipt_time + safety_margin);
								fclose(result_file);
								break;
							}
							
							// rewire the timer to the next estimation of the arrival time
							rte_timer_reset(tim, fdinfo.ea - receipt_time + safety_margin, SINGLE, lcore_id, timer1_cb, (void *)(fdinfo.ea - receipt_time + safety_margin));
						} else {
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "too early to put an estimate, but the arrival time is %lu\n", receipt_time);
							for (int i = 0; i < ARR_SIZE; i++){
								RTE_LOG(DEBUG, DEFAULT_DEBUG, "%lu: %lu | \n", fdinfo.arr_timestamp[i].heartbeat_id, fdinfo.arr_timestamp[i].hb_timestamp);
							}
						}
					}
				}
			}
			rte_pktmbuf_free(bufs[i]);
		}
	}
	return 0;
}


// int lcore_recv_heartbeat_pkt(struct lcore_params *p, struct fd_info * fdinfo, struct rte_timer * tim)
// int lcore_recv_heartbeat_pkt(struct recv_arg recv_arg)
// {
// 	return 0;
// }