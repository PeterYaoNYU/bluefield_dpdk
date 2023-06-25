#include "recv_heartbeat.h"
#include "lstm.h"

static void
timer1_cb(struct rte_timer *tim, void *arg)
{
	unsigned lcore_id = rte_lcore_id();

	// rewire the timer even for the suspected node
	uint64_t rewired_amount = (uint64_t) arg;
	printf("!!%lu!! suspected\n", rewired_amount);

	// rte_timer_reset(tim, rewired_amount, SINGLE, lcore_id, timer1_cb, (void *)rewired_amount);
}

// int lcore_recv_heartbeat_pkt(struct lcore_params *p, struct fd_info * fdinfo, struct rte_timer * tim)
int lcore_recv_heartbeat_pkt(struct recv_arg * recv_arg)
{
	// need to make a translation between the number of cycles per second and the number of seconds of our EA
	uint64_t hz = rte_get_timer_hz();
	printf("the hz is %lu\n", hz);
	// now real_interval is the real number of clock tick between 2 emissions
	hz = hz * DELTA_I / 1000;
	printf("the real gap of clock ticks is %lu\n", hz);

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
	printf("Core %u doing RX dequeue.\n", lcore_id);

	uint64_t pkt_cnt = 0;

	// set the training device to CPU, which is availble On BlueField
    torch::Device device(torch::kCPU);
	// set the input size to 1, which is the size of the feature at each time step
	int input_size = 1;
	// set the size of the number of hidden units in LSTM
    int hidden_size = 4;
	// set the size of the number of output feature at each time step
    int output_size = 1;
	// the number of samples that are processed in parallel
    int batch_size = 1;
    // sequence length represents how far back in time the LSTM can look when processing the input sequence.
    int sequence_length = 3;
	float learning_rate = 0.001;
	int num_epochs = 100;
	int num_layers = 1;
	bool batch_first = true;

	// initialize the model that we will train on the fly
	LSTMModel model(input_size, hidden_size, output_size, num_layers, batch_first);
    torch::nn::MSELoss criterion;
    torch::optim::Adam optimizer(model.parameters(), torch::optim::AdamOptions(learning_rate));

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
						long long_receipt_time = static_cast<long>(receipt_time);
						fdinfo.evicted_time = fdinfo.arr_timestamp[fdinfo.next_evicted].hb_timestamp;

						printf("storing the receipt time into index: %d\n", fdinfo.next_avail);

						// fdinfo.arr_timestamp[fdinfo.next_avail] = (struct hb_timestamp) { .heartbeat_id = pkt_cnt, .hb_timestamp = receipt_time};
						fdinfo.arr_timestamp[fdinfo.next_avail].heartbeat_id = pkt_cnt;
						fdinfo.arr_timestamp[fdinfo.next_avail].hb_timestamp = receipt_time;
						// printf("fdinfo + 1: %d, take mod: %d, ARR_SIZE: %d\n", fdinfo.next_avail+1, (fdinfo.next_avail + 1) % 12, ARR_SIZE);

						
						// increment the next_avail variable 
						fdinfo.next_avail = (fdinfo.next_avail + 1) % 32;

						// if (unlikely(pkt_cnt == HEARTBEAT_N)) {
						if (pkt_cnt == HEARTBEAT_N) {
							int i;
							for (i = 0; i < ARR_SIZE; i++){
								printf("%lu: %lu | ", fdinfo.arr_timestamp[i].heartbeat_id, fdinfo.arr_timestamp[i].hb_timestamp);
							}

							printf("\n");

							// a lock is needed to synchronize,
							// so that the copy is cpmlete before the new data is written in

							std::vector<int64_t> input_vec;
							for (i = 0; i < ARR_SIZE - 1; i++){
								input_vec.push_back(fdinfo.arr_timestamp[i].hb_timestamp);
							}

							std::vector<int64_t> target_vec;
							target_vec.push_back(fdinfo.arr_timestamp[ARR_SIZE-1].hb_timestamp);

							torch::Tensor input = torch::tensor(input_vec);
							input = input.view({batch_size, HEARTBEAT_N-1 , input_size});
							input = input.to(torch::kFloat32);

							torch::Tensor target = torch::tensor(target_vec);
							target = target.view({batch_size, output_size});
							target = target.to(torch::kFloat32);

							std::cout << "Tensor: " << input << std::endl;
							std::cout << "Target: " << target << std::endl;

							// torch::Tensor input = torch::arange(1, HEARTBEAT_N+1, torch::kFloat64).reshape({batch_size, HEARTBEAT_N, input_size});
							// input = input.to(torch::kFloat64);
							// std::cout << "Input:" << std::endl;
    						// std::cout << input << std::endl;

							// int64_t signedValues[HEARTBEAT_N];

							// for (i = 0; i < HEARTBEAT_N; i++){
							// 	signedValues[i] = static_cast<int64_t>(fdinfo.arr_timestamp[i].hb_timestamp);
							// }
							
							// // cannot do that, arr_timestamp is a complicated structure, needs to change this! but it compiles fine
							// torch::Tensor target = torch::from_blob(signedValues, {batch_size, HEARTBEAT_N, input_size}, torch::kLong);
							// std::cout << "Target (Long64 bits):" << std::endl;
    						// std::cout << target << std::endl;

							// target = target.to(torch::kFloat64);

							// std::cout << "Target:" << std::endl;
    						// std::cout << target << std::endl;
							model.trainModel(model, device, criterion, optimizer, input, target, 1);
							std::cout << "training complete" << std::endl;



							// printf("putting the first estimate %lu\n", fdinfo.ea);
							// rte_timer_reset(tim, fdinfo.ea - receipt_time, SINGLE, lcore_id, timer1_cb, (void *)(fdinfo.ea - receipt_time));
						} else if (pkt_cnt > HEARTBEAT_N){
							// calculate the new estimeated arrival time 
							fdinfo.ea = fdinfo.ea + ((receipt_time - (fdinfo.evicted_time) / HEARTBEAT_N));
							printf("FD: %lu th HB arriving, at time %lu, esti: %lu\n", pkt_cnt, receipt_time, fdinfo.ea);

							// update the next_evicted variable
							fdinfo.next_evicted = (fdinfo.next_evicted + 1) % 32;
							
							// rewire the timer to the next estimation of the arrival time
							rte_timer_reset(tim, fdinfo.ea - receipt_time, SINGLE, lcore_id, timer1_cb, (void *)(fdinfo.ea - receipt_time));
						} else {
							printf("too early to put an estimate, but the arrival time is %lu\n", receipt_time);
							for (int i = 0; i < ARR_SIZE; i++){
								printf("%lu: %lu | ", fdinfo.arr_timestamp[i].heartbeat_id, fdinfo.arr_timestamp[i].hb_timestamp);
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