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

# from sync_server import address, authkey
# from multiprocessing.managers import BaseManager

# BaseManager.register("get_queue")
# BaseManager.register("get_event")

# manager = BaseManager(authkey=authkey, address=address)
# manager.connect()

# param_queue= manager.get_queue()
# param_event = manager.get_event()

# profiler = cProfile.Profile() 

import pickle

MY_SIGNAL = signal.SIGUSR1

look_back = 50
ARR_SIZE = 200
first_train = True

train_mq_name = "/train_data"
queue_size = 200
message_size = ctypes.sizeof(ctypes.c_uint64) * queue_size
train_mq = posix_ipc.MessageQueue(train_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666)
print("ok setting up the training data msg queue")

train_mq.request_notification(MY_SIGNAL)

# manager for event and the queue

param_mq_name = "/parameters"
param_mq = posix_ipc.MessageQueue(param_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666)

def custom_loss(y_true, y_pred):
    # Calculate the squared error between true and predicted values
    squared_error = tf.square(y_true - y_pred)
    # Define a penalty factor for negative predictions
    penalty_factor = 1000
    # Apply the penalty factor to negative predictions using TensorFlow's `where` function
    penalized_error = tf.where(y_pred - y_true < 0, squared_error * penalty_factor, squared_error)
    # Calculate the mean of the penalized errors using TensorFlow's `reduce_mean` function
    loss = tf.reduce_mean(penalized_error)
    return loss

# create the LSTM network for training
train_model = Sequential()
train_model.add(LSTM(4, input_shape=(1, look_back)))
train_model.add(Dense(1))
train_model.compile(loss=custom_loss, optimizer='adam', metrics=['accuracy'])

def handle_signal(signum, frame):
    message, _ = train_mq.receive()  
    train(message)
    train_mq.request_notification(MY_SIGNAL)

# def ctrl_c_handler(signum, frame):
#     profiler.disable()
#     # profiler.print_stats(sort="cumulative")
#     profiler.dump_stats("profile_results_python_4_check_again.prof")
#     # print("train_times: ", train_times)
    
# signal.signal(signal.SIGINT, ctrl_c_handler)
signal.signal(MY_SIGNAL, handle_signal)

def train(training_data):
    all_data = struct.unpack(f'{ARR_SIZE}Q', training_data)    
    all_data = np.array(all_data)
    all_data = all_data.reshape(-1, 1)
    trainX, trainY= data_preprocess(all_data)
    train_model.fit(trainX, trainY, epochs=10, batch_size=1, verbose=2)
    model_params = train_model.get_weights()    
    serialized_model_params = pickle.dumps(model_params)
    param_mq.send(serialized_model_params)
    # param_event.set()
    print("************** put param to queue, event set")

    # # do the clean up of the resources that we have opened
    # mq.close()
    # param_queue.close()
    # # Unlink (remove) the message queue
    # posix_ipc.unlink_message_queue(mq_name)


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
    
def data_preprocess(all_data):
    dataset = np.array(all_data)
    # dataset = dataset * 0.000001
    trainX, trainY = create_dataset(dataset, look_back=look_back)
    trainX = np.reshape(trainX, (trainX.shape[0], 1, trainX.shape[1]))
    return trainX, trainY

if __name__ == "__main__":
    while True:
        time.sleep(3600)