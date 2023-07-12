#include "recv_heartbeat.h"

// global variable, the file descriptor of the mq that send the data to the model
mqd_t send_mq;

void keyInteruptHandler(int signal) {
    if (signal == SIGINT) {
        // Close the message queue
        mq_close(send_mq);
		mq_unlink(QUEUE_NAME);
        printf("Message queue closed.\n");
        exit(0);
    }
}

int create_send_msg_queue()
{
    // create the memory queue in the system
	// if succeed, return the message queue descriptor for use of other functions
	// else, indicate that there is an error and quit the system
	struct mq_attr attr;
	mqd_t mq;

	attr.mq_flags = 0;
	attr.mq_maxmsg =10;
	attr.mq_msgsize = sizeof(uint64_t) * ARR_SIZE;
	attr.mq_curmsgs = 0;

	// create the message queue explicitly
	mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
	printf("the msg queue desc is %d\n", (int)mq);
	if (mq == (mqd_t)-1) {
        perror("fail to create the message queue successfully");
        exit(1);
    } else {
		return mq;
	}
}

int 
send_to_ml_model(uint64_t* ts, mqd_t mq_desc, size_t input_size)
{
	struct mq_attr attr;
	mq_getattr(mq_desc, &attr);

	printf("mq maxmsg is %ld\n", attr.mq_maxmsg);
	printf("mq msgsize is %ld\n", attr.mq_msgsize);
	printf("mq curmsg is %ld\n", attr.mq_curmsgs);

	printf("sending to the descriptor: %d\n", (int)mq_desc);
	int ret = mq_send(mq_desc, (const char*)ts, input_size, 0);

	for (int i = 0; i < ARR_SIZE; i++){
		printf("%ld\n",ts[i]);
	}

	if (ret == -1){
		perror("mq_send");
		exit(1);
	}
	return 0;
}

int send_to_infer(uint64_t* ts, mqd_t mq_desc, int next_idx){
	uint64_t send_buffer[LOOK_BACK];
	int i;
	int offset = LOOK_BACK - 1;

	for (i = 0; i < LOOK_BACK; i++){
		int array_index = (next_idx - i - 1 - offset + ARR_SIZE) % ARR_SIZE;
		printf("cloning the %d th element to infer send, with array idx %d\n", i, array_index);
		send_buffer[i] = ts[array_index];
	}
	mq_send(mq_desc, (const char*)send_buffer, sizeof(send_buffer), 0);
}


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

	// this variable is an indication of whether the inference model is ready
	// if it is not ready, the value is 0
	// if ready, the value is set to 1
	int ready_to_infer = 0;
	// this is the buffer that holds ready_to_infer control message if it is ever received
	char ready_to_infer_buffer[sizeof(int)];
	// set the buffer to 0
	memset(ready_to_infer_buffer, 0, sizeof(int));

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

	// TO-DO:
	// later, the send_mq should be changed to a local scope variable as well

    send_mq = mq_open(QUEUE_NAME, O_RDWR);
	// the control message queue is opened in a non-blocking manner

	// !!!!!!!!!!!!!TODO:
	// the control_mq is the problem here, the dpdk program cannot get the signal that it can now send inference data to the model for prediction
 	mqd_t control_mq = mq_open(CONTROL_MQ_NAME, O_RDWR|O_NONBLOCK);
	// the infer_data_mq is from the DPDK to the Inference model, transmitting timestamps to make an inference
	mqd_t infer_data_mq = mq_open(INFER_MQ_NAME, O_RDWR);

	struct mq_attr control_mq_attr;
	mq_getattr(control_mq, &control_mq_attr);
	printf("the max message size in bytes of the control mq is %ld\n", control_mq_attr.mq_msgsize);


	if ((send_mq == (mqd_t)-1) || (control_mq == (mqd_t)-1) || (infer_data_mq == (mqd_t)-1)){
		perror("mq_open failure");
	} else {
		printf("Message queue opened successfully!\n");
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
						fdinfo.evicted_time = fdinfo.arr_timestamp[fdinfo.next_evicted];

						printf("storing the receipt time into index: %d\n", fdinfo.next_avail);

						fdinfo.arr_timestamp[fdinfo.next_avail] = receipt_time;
						
						// increment the next_avail variable 
						fdinfo.next_avail = (fdinfo.next_avail + 1) % 200;

						if (pkt_cnt % HEARTBEAT_N == 0){
							// send the data to the model for training
							send_to_ml_model(fdinfo.arr_timestamp, send_mq, sizeof(fdinfo.arr_timestamp));
						}
						if (!ready_to_infer){
							// try to see if the model is ready for inference tasks
							ssize_t bytes_received = mq_receive(control_mq, ready_to_infer_buffer, control_mq_attr.mq_msgsize , NULL);
							if (bytes_received != -1){
								int received_data = *((int*) ready_to_infer_buffer);
								// if (received_data == 1){
								// 	ready_to_infer = 1;
								// 	// send the last look_back amount of data to the model to do inference
								// 	send_to_infer(fdinfo.arr_timestamp, infer_data_mq, fdinfo.next_avail);
								// }
								printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
								printf("ready to send inference data\n");
								printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
								send_to_infer(fdinfo.arr_timestamp, infer_data_mq, fdinfo.next_avail);
							} else if (bytes_received == -1){
								perror("ctrl_message");
							} else {
								printf("not receiving the ctrl message yet\n");
							}
						} else if (ready_to_infer){
							// the inference model has been ready for a while
							// send the last look_back amount of data to the model to do inference
							printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
							printf("ready to infer\n");
							printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
							send_to_infer(fdinfo.arr_timestamp, infer_data_mq, fdinfo.next_avail);
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