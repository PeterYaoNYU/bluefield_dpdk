#include <stdio.h>
#include <rte_ether.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#define MAX_PORTS 16

int main(int argc, char *argv[]) {
    int ret;
    uint16_t port_id;
    uint16_t nb_ports;
    struct rte_eth_dev_info dev_info;
    struct rte_ether_addr eth_addr;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize EAL\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No available ports found\n");

    printf("Number of ports: %u\n", nb_ports);

    for (port_id = 0; port_id < nb_ports; port_id++) {
        rte_eth_dev_info_get(port_id, &dev_info);
		ret = rte_eth_macaddr_get(port_id, &eth_addr);
		if (ret<0){
				rte_exit(EXIT_FAILURE, "Cannot get MAC address: err=%d, port=%d\n", ret, port_id);
		}

	char mac[18];
	rte_ether_format_addr(&mac[0], 18, &eth_addr);

        printf("Port %u: %s ---> MAC: %s\n", port_id, dev_info.driver_name, mac);
    }

    rte_eal_cleanup();

    return 0;
}

