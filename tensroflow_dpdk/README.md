because running train.py need sudo priviledges, but tensorflow and keras package is only available to the current user of the system, the fowlloing command could be used   
when running the python program with DPDK:  
```shell
sudo -E python3 train.py 
```

the ***-E*** flag keeps the current enviroment varaibles unchaged, so the root user can also locate the tensorflow and the keras package  