### make rule:

sudo g++ -g -O3 -include rte_config.h -march=native -I/usr/local/include -I/usr/include/libnl3
 -DALLOW_EXPERIMENTAL_API main.c port_init.c send_heartbeat.c recv_heartbeat.cpp util.c lstm.cpp -o build/run-shared  -L/usr/local/lib/x86_64-
linux-gnu -Wl,--as-needed -lrte_node -lrte_graph -lrte_pipeline -lrte_table -lrte_pdump -lrte_port -lrte_fib -lrte_ipsec -lrte_vhost -lrte_sta
ck -lrte_security -lrte_sched -lrte_reorder -lrte_rib -lrte_dmadev -lrte_regexdev -lrte_rawdev -lrte_power -lrte_pcapng -lrte_member -lrte_lpm
 -lrte_latencystats -lrte_jobstats -lrte_ip_frag -lrte_gso -lrte_gro -lrte_gpudev -lrte_eventdev -lrte_efd -lrte_distributor -lrte_cryptodev -
lrte_compressdev -lrte_cfgfile -lrte_bpf -lrte_bitratestats -lrte_bbdev -lrte_acl -lrte_timer -lrte_hash -lrte_metrics -lrte_cmdline -lrte_pci
 -lrte_ethdev -lrte_meter -lrte_net -lrte_mbuf -lrte_mempool -lrte_rcu -lrte_ring -lrte_eal -lrte_telemetry -lrte_kvargs -lbsd -I${install_roo
t}/include -I${install_root}/include/torch/csrc/api/include -D_GLIBCXX_USE_CXX11_ABI=0 -std=gnu++17 -L${install_root}/lib -Wl,-R${install_root
}/lib -ltorch -ltorch_cpu -lc10 -ltorch_python
