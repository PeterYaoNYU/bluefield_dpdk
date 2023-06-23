#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_timer.h>
#include <netinet/in.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define IP_ADDR "192.168.0.1"
#define PORT 1234
#define TIMER_INTERVAL_MS 1000

static volatile int keep_running = 1;
static volatile int timer_started = 0;

struct rte_timer timer;

// Timer callback function
static void timer_callback(__rte_unused struct rte_timer *tim, __rte_unused void *arg) {
    printf("Timer fired!\n");
    // Perform your desired action here
}

// Packet processing function
static void process_packet(struct rte_mbuf *pkt) {
    struct rte_ether_hdr *eth_hdr;
    struct rte_ipv4_hdr *ipv4_hdr;
    struct rte_udp_hdr *udp_hdr;

    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr *);

    if (eth_hdr->ether_type == rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
        ipv4_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);

        if (ipv4_hdr->next_proto_id == IPPROTO_UDP) {
            udp_hdr = (struct rte_udp_hdr *)((char *)ipv4_hdr + sizeof(struct rte_ipv4_hdr));

            if (ipv4_hdr->dst_addr == rte_cpu_to_be_32(inet_addr(IP_ADDR)) && udp_hdr->dst_port == rte_cpu_to_be_16(PORT)) {
                // Packet matches the specified IP address and port
                printf("Packet received with matching IP and port!\n");

                // Start the timer if not already running
                if (!timer_started){
                    rte_timer_reset(&timer, TIMER_INTERVAL_MS, PERIODICAL, rte_lcore_id(), timer_callback, NULL);
		    timer_started = 1;
		}
            }
        }
    }

    rte_pktmbuf_free(pkt);
}

// Packet receiving loop
static int packet_receive_loop(__rte_unused void *arg) {
    uint16_t port_id;
    struct rte_mbuf *pkts[BURST_SIZE];
    struct rte_mbuf *pkt;
    unsigned rx_burst;

    port_id = 0;  // Port ID to receive packets from (modify if needed)

    while (keep_running) {
        rx_burst = rte_eth_rx_burst(port_id, 0, pkts, BURST_SIZE);

        for (unsigned i = 0; i < rx_burst; i++) {
            pkt = pkts[i];
            process_packet(pkt);
        }
    }

    return 0;
}

// Initialization function
static int app_init(void) {
    unsigned nb_ports;
    int ret;

    ret = rte_eal_init(0, NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize EAL\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No available ports found\n");

    ret = rte_eth_dev_configure(0, 1, 1, NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure port 0\n");

    ret = rte_eth_rx_queue_setup(0, 0, RX_RING_SIZE, rte_eth_dev_socket_id(0), NULL, rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()));
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize RX queue\n");

    ret = rte_eth_dev_start(0);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot start port 0\n");

    return 0;
}

int main(int argc, char *argv[]) {
    int ret;

    ret = app_init();
    if (ret != 0)
        rte_exit(EXIT_FAILURE, "Initialization failed\n");

    rte_timer_init(&timer);

    ret = rte_eal_remote_launch(packet_receive_loop, NULL, 1);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot launch packet receive loop\n");

    getchar();  // Wait for a key press

    keep_running = 0;

    rte_eal_remote_launch(NULL, NULL, 1);

    rte_eth_dev_stop(0);

    rte_eal_cleanup();

    return 0;
}

