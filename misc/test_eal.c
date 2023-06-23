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
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define IP_ADDR "192.168.0.1"
#define PORT 1234
#define TIMER_INTERVAL_MS 1000

static volatile int keep_running = 1;
static volatile int timer_started = 0;


static int app_init(void) {
    unsigned nb_ports;
    int ret;

    ret = rte_eal_init(0, NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize EAL\n");
}

int main(int argc, char *argv[]) {
    int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0){
		rte_exit(EXIT_FAILURE, "cannot init eal\n");
	}	

    rte_eal_cleanup();
    return 0;
}
