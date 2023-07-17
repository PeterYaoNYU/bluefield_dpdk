#include "send_heartbeat.h"


// add an random delay following the exponential distribution
double exponential_random(double lambda) {
    double u;
    u = (double)rand() / RAND_MAX;  // Generate a random number between 0 and 1
    return -log(1 - u) / lambda;    // Calculate the exponential random number
}

// mainloop should be done either through sleeping or through a timer interrupt 
// use __rte_noreturn macro should lead to more optimized code  
// __rte_noreturn int
int lcore_mainloop_send_heartbeat(struct lcore_params *p)
{
	// uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
	unsigned lcore_id;

    // this variable is used to determine if this message is considered lost or not
    double probability;

	lcore_id = rte_lcore_id();
	printf("Starting mainloop of sending heartbeat on core %u\n", lcore_id);

    uint64_t hb_id = 1;

    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *udp_hdr;

    //init mac
    struct rte_ether_addr s_addr = {{0x08, 0xc0, 0xeb, 0xd1, 0xfc, 0x5e}};
    struct rte_ether_addr d_addr = {{0x08, 0xc0, 0xeb, 0xd1, 0xfc, 0x56}};

    //init IP header
    rte_be32_t s_ip_addr = string_to_ip("192.168.0.16");
    rte_be32_t d_ip_addr = string_to_ip("192.168.0.11");
    uint16_t ether_type = rte_cpu_to_be_16(0x0800);

    struct rte_mbuf *pkts[BURST_SIZE_TX];

    rte_pktmbuf_alloc_bulk(p->mem_pool, pkts, BURST_SIZE_TX);

    uint16_t i ;
    for (i = 0; i < BURST_SIZE_TX; i++)
    {
        eth_hdr = rte_pktmbuf_mtod(pkts[i], struct rte_ether_hdr *);
        eth_hdr->dst_addr = d_addr;
        eth_hdr->src_addr = s_addr;
        eth_hdr->ether_type = ether_type;

        ipv4_hdr = rte_pktmbuf_mtod_offset(pkts[i], struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
        ipv4_hdr->version_ihl = 0x45;
        ipv4_hdr->next_proto_id = 0x11;
        ipv4_hdr->src_addr = s_ip_addr;
        ipv4_hdr->dst_addr = d_ip_addr;
        ipv4_hdr->time_to_live = 0x40;

        udp_hdr = rte_pktmbuf_mtod_offset(pkts[i], struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
        udp_hdr->dgram_len = rte_cpu_to_be_16(sizeof(struct payload) + sizeof(struct rte_udp_hdr));
        // using port 6666 for the hearbeat between the two experiment ports 
        udp_hdr->src_port = rte_cpu_to_be_16(6666);
        udp_hdr->dst_port = rte_cpu_to_be_16(6666);
        ipv4_hdr->total_length = rte_cpu_to_be_16(sizeof(struct payload) + sizeof(struct rte_udp_hdr) + sizeof(struct rte_ipv4_hdr));

        int pkt_size = sizeof(struct payload) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr);

        pkts[i]->l2_len = sizeof(struct rte_ether_hdr);
        pkts[i]->l3_len = sizeof(struct rte_ipv4_hdr);
        pkts[i]->l4_len = sizeof(struct rte_udp_hdr);
        pkts[i]->ol_flags |= RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_UDP_CKSUM;
        ipv4_hdr->hdr_checksum = 0;

        pkts[i]->data_len = pkt_size;
        pkts[i]->pkt_len = pkt_size;   
    }

    double lambda = 1.0 / 30.0;
    // Seed the random number generator
    srand(time(NULL));

	/* Main loop. 8< */
	while (1) {

        // Generate random value between 0 and 1
        probability = (double)rand() / RAND_MAX;

        hb_id++;

        if (probability > PROBABILITY_NOT_SEND){
            lcore_send_heartbeat_pkt(p, hb_id, pkts);
            // sleep in a non-busy manner, can still schedule other tasks on that core 
            // the sleep is set to 100 ms!!!
            double rv = exponential_random(lambda);
            
            printf("Exponential Random Value %ld: %f\n", hb_id, rv);

            int exp_delay = (int)rv*1000;
            rte_delay_us_sleep(DELTA_I * 1000 + exp_delay);

            // begin eval
            if (hb_id == CRASH_TIME){
                FILE * result_file = fopen("delay.txt", "a");
                if (result_file == NULL){
		            printf("Failed to open the file.\n");
	            }
                fprintf(result_file, "the last delay is %f\n", rv);
                fclose(result_file);
            }
            // end eval
        } else {
            printf("MESSAGE LOSS\n");
            rte_delay_us_sleep(DELTA_I * 1000);

            // begin eval
            if (hb_id == CRASH_TIME){
                FILE * result_file = fopen("delay.txt", "a");
                if (result_file == NULL){
		            printf("Failed to open the file.\n");
	            }
                fprintf(result_file, "the last message is lost\n");
                fclose(result_file);
            }
            // end eval
        }
	}
	/* >8 End of main loop. */
    return 0;
}

int lcore_send_heartbeat_pkt(struct lcore_params *p, uint64_t hb_id, struct rte_mbuf **pkts)
{
    // const int socket_id = rte_socket_id();
    // printf("Core %u sending heartbeat packet id: %lu.\n", rte_lcore_id(), hb_id);

    uint16_t i ;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *udp_hdr;


    for (i = 0; i < BURST_SIZE_TX; i++)
    {          
        //init udp payload
        struct payload obj = {
            .heartbeat_id = hb_id,
        };
        struct payload *msg;

        msg = (struct payload *)(rte_pktmbuf_mtod(pkts[i], char *) + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
        *msg = obj;

        ipv4_hdr = rte_pktmbuf_mtod_offset(pkts[i], struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
        udp_hdr = rte_pktmbuf_mtod_offset(pkts[i], struct rte_udp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
        udp_hdr->dgram_cksum = rte_ipv4_phdr_cksum(ipv4_hdr, pkts[i]->ol_flags);
    }

    uint16_t sent = rte_eth_tx_burst(0, p->tx_queue_id, pkts, BURST_SIZE_TX);   
    if (unlikely(sent < BURST_SIZE_TX))
    {
        while (sent < BURST_SIZE_TX)
        {
            rte_pktmbuf_free(pkts[sent++]);
        }
    }

    printf("Sender: %u packets were sent in this burst, id: %lu\n", sent, hb_id);

    return 0;
}