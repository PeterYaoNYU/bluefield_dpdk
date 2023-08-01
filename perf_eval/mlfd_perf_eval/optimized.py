# this is a process that train the machine learning model
# after training, it sends the parameters of the model to the inference process via IPC
# which later make the 

import numpy as np
import tensorflow as tf
from tensorflow import keras
from keras.models import Sequential
from keras.layers import Dense
from keras.layers import LSTM
# from sklearn.preprocessing import MinMaxScaler
# from sklearn.metrics import mean_squared_error
import math
# import matplotlib.pyplot as plt
import keras.backend as K

import posix_ipc
import ctypes
import struct
# import pickle
import multiprocessing
import queue

def tf_custom_loss(y_true, y_pred):
    # Define a penalty factor for negative predictions
    penalty_factor = 1000000

    # Calculate the squared error between true and predicted values
    squared_error = tf.square(y_true - y_pred)

    # Apply the penalty factor to negative predictions using tf.where
    penalized_error = tf.where(y_pred < 0, squared_error * penalty_factor, squared_error)

    # Calculate the mean of the penalized errors
    loss = tf.reduce_mean(penalized_error)
    
    return loss 


def custom_loss(y_true, y_pred):
    # Calculate the squared error between true and predicted values
    squared_error = K.square(y_true - y_pred)  
    # Define a penalty factor for negative predictions
    penalty_factor = 1000000  
    # Apply the penalty factor to negative predictions
    penalized_error = K.switch(y_pred < 0, squared_error * penalty_factor, squared_error)  
    # Calculate the mean of the penalized errors
    loss = K.mean(penalized_error)
    return loss

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

def inference(param_queue, infer_mq):
    # this is the message queue for sending control messages
    # eg: I am ready to do inference, send me the task
    # or others that I cannot think of right now
    control_mq_name = "/ctrl_msg"
    queue_size = 10
    message_size = ctypes.sizeof(ctypes.c_int) * queue_size
    ctrl_mq = posix_ipc.MessageQueue(control_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    look_back = 50
    
    all_data = []
    
    # this mqueue is for transmitting the already predicted result
    result_mq_name = "/result_mq"
    queue_size = 100
    message_size = ctypes.sizeof(ctypes.c_uint64) * 2
    result_mq = posix_ipc.MessageQueue(result_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    
    print("!!!!!!!!!!!!!!")
    print("waiting for the parameter of the model to come!!!")
    print("!!!!!!!!!!!!!!")
    # retrieve the parameters of the model from the message queue
    # the first time the child proces starts, have to sync to wait for the params to be available
    # model_params = param_queue.get(block = True)
    
    model_params_ready = False
    
    # create a LSTM model with the same structure
    infer_model = Sequential()
    infer_model.add(LSTM(4, input_shape=(1, look_back)))
    infer_model.add(Dense(1))
    
    # apply the weights trained   
    # infer_model.set_weights(model_params)
    
    # tell the DPDK process that I am ready for doing inference...
    # TO-DO
    # send 1 to DPDK to indicate that I am ready to do inference
    # number = 1
    # ctrl_mq.send(number.to_bytes(4, byteorder='little'))
    
    while(True):
        model_params = None
        try:
            model_params = param_queue.get(block = False)
            if (model_params):
                infer_model.set_weights(model_params)
                if not model_params_ready:
                    print("!!!!!!!!!!!!!!")
                    print("inference proc ready!!!")
                    print("!!!!!!!!!!!!!!")
                    number = 1
                    ctrl_mq.send(number.to_bytes(4, byteorder='little'))
                    model_params_ready = True
        except queue.Empty:
            pass
            # print("Queue is empty. No model parameters available.")
        # Receive message from the queue    
        message, _ = infer_mq.receive()
        
        # Interpret the received message as an array of uint64_t
        # plus one for matching request with response
        received_array = struct.unpack(f'{2}Q', message)
        print("&&&&&&&&&&&&&&&&&&&&&&&&&&&")
        print(received_array)
        print("&&&&&&&&&&&&&&&&&&&&&&&&&&&")
        
        # now making inference: exclude the first element, because it is for matching request with response
        pkt_cnt_id = received_array[0]
        
        all_data.append(received_array[1])
        if len(all_data) > look_back:
            all_data.pop(0)
            dataset = np.array(all_data)
            # dataset = dataset * 0.000001
            # print(dataset)
            
            start = dataset[0]
            end = dataset[-1]
            interval = end - start
            
            dataset = (dataset - start)/(end - start)
            
            dataset = np.reshape(dataset, (1, 1, dataset.shape[0]))
            # print("after reshaping: ", dataset.shape)
        
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
        
    # get the lookback information from DPDK and do inference
    # then send the result back to DPDK
    
    
def data_preprocess(all_data):
    dataset = np.array(all_data)
    # dataset = dataset * 0.000001
    trainX, trainY = create_dataset(dataset, look_back=look_back)
    trainX = np.reshape(trainX, (trainX.shape[0], 1, trainX.shape[1]))
    return trainX, trainY

    
def train(param_queue):
    look_back = 50
    
    ARR_SIZE = 1
    
    # for debugging the message queue
    # test_mq_name = "/test_msg"
    # queue_size = 10
    # message_size = ctypes.sizeof(ctypes.c_int) * queue_size
    # test_mq = posix_ipc.MessageQueue(test_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    # print("test_ok")
    
    
    train_data_count = 0
    first_train = True
    fine_tune_ready = False
    all_data = []
    fine_tune_count = 0

    train_mq_name = "/train_data"
    queue_size = 300
    message_size = ctypes.sizeof(ctypes.c_uint64)
    train_mq = posix_ipc.MessageQueue(train_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    print("ok")
     
    while(True):
        if (first_train):
            while (train_data_count < 200):
                message, _ = train_mq.receive()
                received_number = struct.unpack(f'{ARR_SIZE}Q', message)
                all_data.append(received_number[0])
                train_data_count = train_data_count + ARR_SIZE
                print("length: ", len(all_data))
                print(all_data)
            # first_train = False
        else:
            # Receive message from the queue
            while (fine_tune_count < 200):
                message, _ = train_mq.receive()
                print("get train data")
                received_number = struct.unpack(f'{ARR_SIZE}Q', message)
                all_data.append(received_number[0])
                fine_tune_count = fine_tune_count + 1
                if (fine_tune_count == 200):
                    fine_tune_ready = True
                
        # # troubleshoot
        # print(len(message))   # Print the length of the message buffer
        # print(ARR_SIZE)       # Print the value of ARR_SIZE
        # print(struct.calcsize(f'{ARR_SIZE}Q'))  # Print the expected size of the unpacked data
        
        # Interpret the received message as an array of uint64_t
        # received_array = struct.unpack(f'{ARR_SIZE}Q', message) 
        
        if (first_train):   
            all_data = np.array(all_data)
            all_data = all_data.reshape(-1, 1)
            trainX, trainY= data_preprocess(all_data)
            
            # create and fit the LSTM network
            model = Sequential()
            model.add(LSTM(4, input_shape=(1, look_back)))
            model.add(Dense(1))
            model.compile(loss=custom_loss, optimizer='adam', metrics=['accuracy'])
            model.fit(trainX, trainY, epochs=10, batch_size=1, verbose=2)
        
        # because python does not support parallel execution on multicores for threads
        # we have to resort to multi processing
        # one process to keep training, and anther process to do the inference
        # and report back to DPDK
        
        # here I used the multiprocessing package for forking and msg queue among python processes
        
        # Get the model parameters
            model_params = model.get_weights()    
            param_queue.put(model_params)
            first_train = False  
            all_data = []  
        elif (fine_tune_ready):
            dataset = np.array(all_data)
            dataset = all_data.reshape(-1, 1)
            trainX, trainY= data_preprocess(dataset)
            model.fit(trainX, trainY, epochs=5, batch_size=1, verbose=2)
            model_params = model.get_weights()    
            param_queue.put(model_params)
            fine_tune_ready = False
            fine_tune_count = 0
            all_data = []
    
    # do the clean up of the resources that we have opened
    mq.close()
    param_queue.close()
    # Unlink (remove) the message queue
    posix_ipc.unlink_message_queue(mq_name)
    
if __name__ == "__main__":
    look_back = 50
    # create a queue that is used to transmit model parameters  
    param_queue = multiprocessing.Queue()
    
    # create the inference queue that is used for the dpdk to transmit data for you to infer
    infer_mq_name = "/infer_data"
    queue_size = look_back
    # plus one for matching infer request with response!
    message_size = 2 * ctypes.sizeof(ctypes.c_uint64)
    
    infer_mq = posix_ipc.MessageQueue(infer_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    
    # Create a child process and pass the queue as an argument
    child = multiprocessing.Process(target=inference, args=(param_queue,infer_mq,))
    
    child.start()
    train(param_queue)
    
    child.join()