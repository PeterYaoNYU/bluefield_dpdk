#include "util.h"

rte_be32_t string_to_ip(char *s)
{
    unsigned char a[4];
    int rc = sscanf(s, "%hhd.%hhd.%hhd.%hhd", a + 0, a + 1, a + 2, a + 3);
    if (rc != 4)
    {
        fprintf(stderr, "bad source IP address format. Use like: 1.1.1.1\n");
        exit(1);
    }
    return (rte_be32_t)(a[3]) << 24 |
           (rte_be32_t)(a[2]) << 16 |
           (rte_be32_t)(a[1]) << 8 |
           (rte_be32_t)(a[0]);
}
