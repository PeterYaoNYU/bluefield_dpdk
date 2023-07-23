because running train.py need sudo priviledges, but tensorflow and keras package is only available to the current user of the system, the fowlloing command could be used   
when running the python program with DPDK:  
```shell
sudo -E python3 train.py 
```

the ***-E*** flag keeps the current enviroment varaibles unchaged, so the root user can also locate the tensorflow and the keras package    


if you encounter the problem where the prompt says that there is not enough file descripotrs for the train.py, here is how I debug the program:  

1. locate exactly which mqueue is causing the trouble
2. change the size of that mqueue, it should work, usually it is because that the size is set too large
3. if it doesn't work, change the name of the troubled queue as well. 