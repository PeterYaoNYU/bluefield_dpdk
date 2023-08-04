#include "recv_heartbeat.h"

FILE * general_stats_output;
uint64_t false_positive_cnt = 0;

static void
timer1_cb(struct rte_timer *tim, void *arg)
{
	unsigned lcore_id = rte_lcore_id();

	uint64_t suspected_time = rte_rdtsc();

	// rewire the timer even for the suspected node
	uint64_t rewired_amount = (uint64_t) arg;

	false_positive_cnt++;

	RTE_LOG(INFO, FD_OUTPUT, "!!%lu!! suspected\n", rewired_amount);
	RTE_LOG(INFO, FD_OUTPUT, "Suspected Time: %lu\n", suspected_time);

	// rte_timer_reset(tim, rewired_amount, SINGLE, lcore_id, timer1_cb, (void *)rewired_amount);
}


	/* for Jacobson's dynamic estimation of safety margin,
	we need the following variables:
		error
		delay_prev	: delay(k)
		delay_new	: delay(k+1)
		var_prev	: var(k)
		var_new		: var(k+1)
		alpha
		BETA		:= 1
		THETA		:= 4
		GAMMA		:= 0.1
		tau
	*/

	/* for Bertier's estimation before receiving n heartbeats.
	we need to keep track of
		u_prev	: U(k)
		u_new	: U(k+1)
		where u_new is initialized to A(0), i.e. 1st reception time
	*/


// int lcore_recv_heartbeat_pkt(struct lcore_params *p, struct fd_info * fdinfo, struct rte_timer * tim)
int lcore_recv_heartbeat_pkt(struct recv_arg * recv_arg)
{
	uint64_t tau = 0;
	uint64_t u_prev = 0;
	uint64_t u_new = 0;
	int64_t delay = 0;
	int64_t alpha = 0;
	int64_t var_prev = 0;
	int64_t var_new = 0;
	int64_t error = 0;

	int BETA = 1;
	int THETA = 4;
	int GAMMA_INVERSE = 10;

		
	general_stats_output = fopen("./output/general_stats.txt", "a");
	if (general_stats_output == NULL){
		perror("General Stats File Fail to Open");
	}
	fprintf(general_stats_output, "*******************New Run*******************\n");

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
						fdinfo.next_avail = (fdinfo.next_avail + 1) % 10;


						// the 10s here in the following lines should really be GAMMA_INVERSE, but I am too lazy to change that
						// it is working fine, so why bother?
						// change it when it is absolutely necessary
						error = receipt_time - fdinfo.ea - delay;
						delay = delay + error / 10;
						var_new = var_prev + (labs(error) - var_prev) / 10;
						RTE_LOG(DEBUG, DEFAULT_DEBUG, "abs(error): %ld, var_prev: %ld, (abs(error) - var_prev) / 10: %ld\n", labs(error), var_prev, (labs(error) - var_prev) / 10);
						var_prev = var_new;
						alpha = BETA * delay + THETA * var_new;
						safety_margin = alpha;

						RTE_LOG(DEBUG, DEFAULT_DEBUG, "error: %ld, delay: %ld, var_new: %ld, alpha: %ld, safety_margin: %ld\n", error, delay, var_new, alpha, safety_margin);

						if (pkt_cnt > HEARTBEAT_N){
							// calculate the new estimeated arrival time 
							fdinfo.ea = fdinfo.ea + ((receipt_time - (fdinfo.evicted_time)) / HEARTBEAT_N);
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "FD: %lu th HB arriving, at time %lu, esti: %lu, evicted: %lu\n", pkt_cnt, receipt_time, fdinfo.ea, fdinfo.evicted_time);

							// update the next_evicted variable
							fdinfo.next_evicted = (fdinfo.next_evicted + 1) % 10;

							// if (pkt_cnt == STOP_TIME){
							// 	// begin eval
							// 	FILE * result_file = fopen("output.txt", "a");

							// 	if (result_file == NULL){
							// 		RTE_LOG(INFO, SYS_INFO, "Failed to open the file.\n");
							// 	}
							// 	fprintf(result_file, "the time it takes to start suspecting is %lu\n", fdinfo.ea - receipt_time + safety_margin);
							// 	fclose(result_file);
							// 	break;
							// 	// end eval
							// }
							
							// rewire the timer to the next estimation of the arrival time
							rte_timer_reset(tim, fdinfo.ea - receipt_time + safety_margin, SINGLE, lcore_id, timer1_cb, (void *)(fdinfo.ea - receipt_time + safety_margin));
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "rewiring the timer to %lu\n", fdinfo.ea - receipt_time + safety_margin);
						} else {
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "too early to put an estimate, but the arrival time is %lu, pkt_cnt: %lu\n", receipt_time, pkt_cnt);
							for (int i = 0; i < ARR_SIZE; i++){
								RTE_LOG(DEBUG, DEFAULT_DEBUG, "%lu: %lu | \n", fdinfo.arr_timestamp[i].heartbeat_id, fdinfo.arr_timestamp[i].hb_timestamp);
							}
							u_new = receipt_time / pkt_cnt + (pkt_cnt - 1) / pkt_cnt * u_prev;
							fdinfo.ea = u_new + (pkt_cnt + 1) / 2 * DELTA_I;
							u_prev = u_new;
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "u_new: %ld\n", u_new);
							RTE_LOG(DEBUG, DEFAULT_DEBUG, "the estimated arrival time is %lu\n", fdinfo.ea);
						}

						if (pkt_cnt % 1000 == 0){
							fprintf(general_stats_output, "false_positives: %lu\n", false_positive_cnt);
							fflush(general_stats_output);
							false_positive_cnt = 0;
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