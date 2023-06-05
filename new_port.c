#include <stdio.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#define MAX_PORTS 16

int main(int argc, char *argv[]) {
    int ret;
    uint16_t port_id;
    uint16_t nb_ports;
    struct rte_eth_dev_info dev_info;

    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize EAL\n");

    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No available ports found\n");

    printf("Number of ports: %u\n", nb_ports);

    for (port_id = 0; port_id < nb_ports; port_id++) {
        rte_eth_dev_info_get(port_id, &dev_info);
        printf("Port %u: %s\n", port_id, dev_info.driver_name);
    }

    rte_eal_cleanup();

    return 0;
}

