#inlcude "recv_heartbeat.h"


static void
timer1_cb(__rte_unused struct rte_timer *tim,
	  __rte_unused void *arg)
{
	unsigned lcore_id = rte_lcore_id();

	printf("the other side suspected to be dead\n");

	rte_timer_reset(tim, fdinfo->ea - receipt_time, SINGLE, lcore_id, timer1_cb, NULL);
}

int lcore_recv_heartbeat_pkt(struct lcore_params *p, struct fd_info * fdinfo, struct rte_timer * tim)
{
	const int socket_id = rte_socket_id();

	unsigned lcore_id = rte_lcore_id();
	printf("Core %u doing RX dequeue.\n", lcore_id);

	uint64_t pkt_cnt = 0;

	while (1){
		struct rte_mbuf *bufs[BURST_SIZE];
		uint16_t nb_rx = rte_eth_rx_burst(0, p->rx_queue_id, bufs, BURST_SIZE);
		// printf("received %u packets in this burst\n", nb_rx);

		if (nb_rx == 0){
			continue;
		}

		printf("received %u packets in this burst\n", nb_rx);
		uint16_t i;
		for (i = 0; i < nb_rx; i++){
            struct rte_mbud *pkt = bufs[i];

            // unwrap the ethernet layer header
            struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
            
            // if this is indeed an IP packet
            if (ether_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4) ){
				struct ipv4_hdr * ip_hdr = (struct ipv4_hdr *)(eth_hdr + 1);
				// if this is a UDP packet and is intended for me and is from someone that I am expecting...
				if ((ip_hdr->next_proto_id == IPPROTO_UDP) && (ip_hdr->src_addr == string_to_ip(NODE_2_IP)) && (ip_hdr->dst_addr == string_to_ip(NODE_1_IP)) ){
					struct udp_hdr *udp_hdr = (struct udp_hdr *)(ip_hdr + 1);
					// if the UDP port matches what I am expecting...
					if (udp_hdr->dst_port == rte_cpu_to_be_16(HB_SRC_PORT)) {
						// print the packet detail that I have unwrapped so far...
						// TO-DO

						// update the Chen's estimation based on the packet received...
						struct payload * obj= (struct payload *)(udp_hdr + 1);
						uint64_t receipt_time = rte_rdtsc();

						fdinfo->arr_timestamp[fdinfo->next_avail] = (struct hb_timestamp) { .heartbeat_id = obj->heartbeat_id, .hb_timestamp = receipt_time};
						
						// increment the next_avail variable 
						fdinfo->next_avail = (fdinfo -> next_avail + 1) % HEARTBEAT_N;

						if (unnlikely(obj->heartbeat_id == HEARTBEAT_N)) {
							uint16_t i;
							uint64_t moving_sum;
							struct hb_timestamp hb;
							for (i = next_evicted; i < next_avail; i++){
								hb = fdinfo->arr_timestamp[i];
								moving_sum += (hb.hb_timestamp - hb.heartbeat_id * fdinfo->delta_i);
							}
							fdinfo->ea = moving_sum / HEARTBEAT_N + (HEARTBEAT_N+1) * (fdinfo->delta_i);
						} else {
							// calculate the new estimeated arrival time 
							fdinfo->ea = fdinfo->ea + ((receipt_time - arr_timestamp[fdinfo->next_evicted]) / HEARTBEAT_N)
							printf("FD: %llu th HB arriving, at time %llu, esti: %llu\n", obj->heartbeat_id, receipt_time, fdinfo->ea);

							// update the next_evicted variable
							fdinfo->next_evicted = (fdinfo -> next_evicted + 1) % HEARTBEAT_N;
							
							// rewire the timer to the next estimation of the arrival time
							rte_timer_reset(tim, fdinfo->ea - receipt_time, SINGLE, lcore_id, timer1_cb, NULL);
						}

					}
				}
			}


			rte_pktmbuf_free(bufs[i]);
		}

	}
	return 0;
}
