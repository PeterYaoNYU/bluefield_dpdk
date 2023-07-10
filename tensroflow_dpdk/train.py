# this is a process that train the machine learning model
# after training, it sends the parameters of the model to the inference process via IPC
# which later make the 

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

import posix_ipc
import ctypes
import struct
# import pickle
import multiprocessing
import queue

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
        dataX.append(a)
        dataY.append(dataset[i + look_back, 0])
    return np.array(dataX), np.array(dataY)

def inference(param_queue, infer_mq):
    # this is the message queue for sending control messages
    # eg: I am ready to do inference, send me the task
    # or others that I cannot think of right now
    control_mq_name = "/ctrl_msg"
    queue_size = 10
    message_size = ctypes.sizeof(ctypes.c_int) * queue_size
    
    look_back = 50
    
    print("!!!!!!!!!!!!!!")
    print("waiting for the parameter of the model to come!!!")
    print("!!!!!!!!!!!!!!")
    # retrieve the parameters of the model from the message queue
    # the first time the child proces starts, have to sync to wait for the params to be available
    model_params = param_queue.get(block = True)
    
    # create a LSTM model with the same structure
    infer_model = Sequential()
    infer_model.add(LSTM(4, input_shape=(1, look_back)))
    infer_model.add(Dense(1))
    
    # apply the weights trained   
    infer_model.set_weights(model_params)
    print("!!!!!!!!!!!!!!")
    print("inference proc ready!!!")
    print("!!!!!!!!!!!!!!")
    
    # tell the DPDK process that I am ready for doing inference...
    # TO-DO
    ctrl_mq = posix_ipc.MessageQueue(control_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    # send 1 to DPDK to indicate that I am ready to do inference
    ctrl_mq.send(bytes(str(1), encoding="utf-8"))
    
    while(True):
        model_params = None
        try:
            model_params = param_queue.get(block = False)
            if (model_params):
                infer_model.set_weights(model_params)
        except queue.Empty:
            print("Queue is empty. No model parameters available.")
        # here i used a bare except clause because I cannot find the Empty exception in this package
        
        
    # get the lookback information from DPDK and do inference
    # then send the result back to DPDK
    
def train(param_queue):
    look_back = 50
    
    ARR_SIZE = 200

    # Message queue parameters
    mq_name = '/ml_train'
    queue_size = 200
    message_size = ctypes.sizeof(ctypes.c_uint64) * ARR_SIZE

    print("message size: ", message_size)
    
    # create a new posix message queue
    # this queue is for DPDK to transmit training data to the model
    mq = posix_ipc.MessageQueue(mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    # Print the message queue's descriptor
    print("Message queue created with descriptor:", mq.mqd)
    
    while(True):
        # Receive message from the queue
        message, _ = mq.receive()
        
        # Interpret the received message as an array of uint64_t
        received_array = struct.unpack(f'{ARR_SIZE}Q', message)

        
        dataset = np.array(received_array)
        dataset = dataset * 0.00000001
        print(dataset)
        
        start = dataset[0]
        end = dataset[-1]
        interval = end - start
        
        dataset = dataset - start
        
        scaler = MinMaxScaler(feature_range=(0, 1))
        dataset = dataset.reshape(-1,1)
        dataset = scaler.fit_transform(dataset)
        
        trainX, trainY = create_dataset(dataset, look_back=look_back)
        
        trainX = np.reshape(trainX, (trainX.shape[0], 1, trainX.shape[1]))
        
        print("****************")
        print(dataset)
    
        # create and fit the LSTM network
        model = Sequential()
        model.add(LSTM(4, input_shape=(1, look_back)))
        model.add(Dense(1))
        model.compile(loss=custom_loss, optimizer='adam', metrics=['accuracy'])
        model.fit(trainX, trainY, epochs=10, batch_size=1, verbose=2)
        
        # make predictions
        trainPredict = model.predict(trainX)
        
        # invert predictions
        trainPredict = scaler.inverse_transform(trainPredict)
        trainY = scaler.inverse_transform([trainY])
        
        # calculate root mean squared error
        trainScore = math.sqrt(mean_squared_error(trainY[0], trainPredict[:,0]))
        print('Train Score: %.2f RMSE' % (trainScore))
        
        # because python does not support parallel execution on multicores for threads
        # we have to resort to multi processing
        # one process to keep training, and anther process to do the inference
        # and report back to DPDK
        
        # here I used the multiprocessing package for forking and msg queue among python processes
        
        # Get the model parameters
        model_params = model.get_weights()    
        param_queue.put(model_params)
    
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
    message_size = look_back * ctypes.sizeof(ctypes.c_uint64)
    
    infer_mq = posix_ipc.MessageQueue(infer_mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    
    # Create a child process and pass the queue as an argument
    child = multiprocessing.Process(target=inference, args=(param_queue,infer_mq,))
    
    child.start()
    train(param_queue)
    
    child.join()