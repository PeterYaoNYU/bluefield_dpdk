#include <stdio.h>
#include <stdlib.h> // Include the <stdlib.h> header
#include <rte_eal.h>
#include <rte_ethdev.h>

int main(int argc, char *argv[]) {
    int ret;

    // Initialize the EAL (Environment Abstraction Layer)
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize the EAL.\n");
        exit(EXIT_FAILURE); // Use exit() instead of rte_exit()
    }

    // Get the number of available Ethernet devices
    uint16_t num_ports = rte_eth_dev_count_avail();

    if (num_ports == 0) {
        printf("No Ethernet ports found.\n");
    } else {
        printf("Found %d Ethernet port(s).\n", num_ports);

        // Get the port ID of the first available port
        uint16_t port_id = rte_eth_find_next(0);
        printf("The port ID of the first available port is %u.\n", port_id);
    }

    return 0;
}

