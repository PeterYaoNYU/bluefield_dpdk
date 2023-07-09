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

def main():
    look_back = 50
    
    ARR_SIZE = 200

    # Message queue parameters
    mq_name = '/ml_train'
    queue_size = 200
    message_size = ctypes.sizeof(ctypes.c_uint64) * ARR_SIZE

    print("message size: ", message_size)
    
    # create a new posix message queue
    mq = posix_ipc.MessageQueue(mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)
    # Print the message queue's descriptor
    print("Message queue created with descriptor:", mq.mqd)
    # Receive message from the queue
    message, _ = mq.receive()
    
    # Interpret the received message as an array of uint64_t
    received_array = struct.unpack(f'{ARR_SIZE}Q', message)
    
    print(received_array)
    mq.close()
    # Unlink (remove) the message queue
    posix_ipc.unlink_message_queue(mq_name)
    
    
    # create and fit the LSTM network
    # model = Sequential()
    # model.add(LSTM(4, input_shape=(1, look_back)))
    # model.add(Dense(1))
    # model.compile(loss=custom_loss, optimizer='adam', metrics=['accuracy'])
    # model.fit(trainX, trainY, epochs=10, batch_size=1, verbose=2)
    
if __name__ == "__main__":
    main()

