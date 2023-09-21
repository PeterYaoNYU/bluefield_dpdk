# this is a process that train the machine learning model
# after training, it sends the parameters of the model to the inference process via IPC
# which later make the 

import posix_ipc
import ctypes
import struct
# import pickle
import multiprocessing
import queue
import time

import numpy as np
import tensorflow as tf
from tensorflow import keras
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import LSTM
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import mean_squared_error
import math
import matplotlib.pyplot as plt
import keras.backend as K

import cProfile
import signal

import threading

from sync_server import address, authkey
from multiprocessing.managers import BaseManager

BaseManager.register("get_queue")
BaseManager.register("get_event")
manager = BaseManager(authkey=authkey, address=address)
manager.connect()

param_queue = manager.get_queue()
param_event = manager.get_event()


MY_SIGNAL = signal.SIGUSR2

look_back = 50
ARR_SIZE = 200

# this is to tell whether the process is ready to make inferences:
# if it is ready, it will send 1 to the control message queue
inference_ready = False

control_mq_name = "/ctrl_msg"
queue_size = 10
message_size = ctypes.sizeof(ctypes.c_int) * queue_size
ctrl_mq = posix_ipc.MessageQueue(control_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
look_back = 50

result_mq_name = "/result_mq"
queue_size = 100
message_size = ctypes.sizeof(ctypes.c_uint64) * 2
result_mq = posix_ipc.MessageQueue(result_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)

# create the inference queue that is used for the dpdk to transmit data for you to infer
infer_mq_name = "/infer_data"
queue_size = look_back
# plus one for matching infer request with response!
message_size = (look_back+1) * ctypes.sizeof(ctypes.c_uint64)
print("the message size of the " + str(message_size))
infer_mq = posix_ipc.MessageQueue(infer_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)


infer_mq.request_notification(MY_SIGNAL)

# create a LSTM model with the same structure
infer_model = Sequential()
infer_model.add(LSTM(4, input_shape=(1, look_back)))
infer_model.add(Dense(1))

# this lock is to prevent concurrent access to the parameters of the model
param_lock = threading.Lock()

def handle_signal(signum, frame):
    print("signal received")
    while True: 
        try:
            message, _ = infer_mq.receive(timeout = 0)
            print("received a message from the infer queue")
            inference(message)
            infer_mq.request_notification(MY_SIGNAL)
        except posix_ipc.BusyError:
            break
    
def param_consumer():
    inference_ready=False
    signal.siginterrupt(signal.SIGUSR2, False)
    while True:
        param_event.wait()
        model_params = param_queue.get()
        param_lock.acquire()
        infer_model.set_weights(model_params)
        if not inference_ready:
            inference_ready = True
            # send 1 to DPDK to indicate that I am ready to do inference
            number = 1
            ctrl_mq.send(number.to_bytes(4, byteorder='little'))
            
        print("get param from queue, event notification get")
        param_lock.release()
        # print("************** wait param")
        # model_params = param_queue.get()
        # print("!!!!!!!!!!!!!! get param")
        # if (model_params):
        #     param_lock.acquire()
        #     infer_model.set_weights(model_params)
        #     param_lock.release()

# def ctrl_c_handler(signum, frame):
#     profiler.disable()
#     # profiler.print_stats(sort="cumulative")
#     profiler.dump_stats("profile_results_python_4_check_again.prof")
#     # print("train_times: ", train_times)
    
# signal.signal(signal.SIGINT, ctrl_c_handler)
signal.signal(MY_SIGNAL, handle_signal)


# convert an array of values into a dataset matrix
def create_dataset(dataset, look_back=1):
    dataX, dataY = [], []
    for i in range(len(dataset)-look_back-1):
        a = dataset[i:(i+look_back), 0]
        window_min = a[0]
        window_max = a[-1]
        a = (a - window_min) / (window_max - window_min)
        dataX.append(a)
        dataY.append((dataset[i + look_back, 0] - window_min) / (window_max - window_min))
    return np.array(dataX), np.array(dataY)

def inference(message):
    print("waiting to acquire the lock")
    param_lock.acquire()
    
    received_array = struct.unpack(f'{look_back+1}Q', message)
    print("&&&&&&&&&&&&&&&&&&&&&&&&&&&")
    print(received_array)
    print("&&&&&&&&&&&&&&&&&&&&&&&&&&&")
        
    # now making inference: exclude the first element, because it is for matching request with response
    pkt_cnt_id = received_array[0]
    dataset = np.array(received_array[1:])
        
    start = dataset[0]
    end = dataset[-1]
    interval = end - start
    
    dataset = (dataset - start)/(end - start)
    
    dataset = np.reshape(dataset, (1, 1, dataset.shape[0]))
    
    # make predictions
    next_arrival = infer_model.predict(dataset)
    print("next arrival (before scaling back): ", next_arrival)
        
    # invert predictions
    next_arrival = next_arrival * (end - start) + start
    
    print("next arrival (after scaling back): ", next_arrival[0][0])
    next_arrival = int(next_arrival.item())
    
    # TODO send the result back to dpdk
    packed_next_arrival_ts = struct.pack("QQ", next_arrival, pkt_cnt_id)
    result_mq.send(packed_next_arrival_ts)
    
    param_lock.release()
    print("param lock released")
    
    
def data_preprocess(all_data):
    dataset = np.array(all_data)
    # dataset = dataset * 0.000001
    trainX, trainY = create_dataset(dataset, look_back=look_back)
    trainX = np.reshape(trainX, (trainX.shape[0], 1, trainX.shape[1]))
    return trainX, trainY

if __name__ == "__main__":
    infer_mq.request_notification(MY_SIGNAL)
    model_param_thread = threading.Thread(target=param_consumer)
    model_param_thread.start()
    while True:
        time.sleep(3600)