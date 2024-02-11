# bluefield_dpdk
This is a machine learning based failure detector for distributed systems. It is based on DPDK and Tensorflow. It can be offloaded easily to NVIDIA Bluefield-2 SmartNIC. 

This repo was written half a year ago, so I have a hard time recalling which part is the main code. It is also messy, because it contains all the intermediate work (trials and failures) that I have not yet cleaned up. 

To run it, you need to have DPDK compatible NICs. You also need to have configured your NIC appropriately. (Intel NICs need NIC binding, while Mellanox NIC does not).

The main code is in ***/tensroflow_dpdk*** folder  
To compile the dpdk program, use Makefile.  
To run, please first start the Python process that serves as a Machine Learning Inference backend, by running：  
```bash
sudo -E python3 train.py
```

To reproduce the experiment result, you need to have 2 servers interconnected back to back.  

For more detail, please refer to the report: https://drive.google.com/file/d/1T3i0JLjAAVSKCsDsdZXVK4rvqgLsfthK/view?usp=sharing 
      

