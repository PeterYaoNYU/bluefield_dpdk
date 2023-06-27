import posix_ipc
import ctypes

ARR_SIZE = 10

# Define the nested struct
class Hb_timestamp(ctypes.Structure):
    _fields_ = [
        ("heartbeat_id", ctypes.c_uint64),
        ("hb_timestamp", ctypes.c_uint64),
        # Add more fields as needed
    ]

# Define the outer struct containing the array of inner structs
class Fd_info(ctypes.Structure):
    _fields_ = [
        ("delta_i", ctypes.c_int),
        ("ea", ctypes.c_uint64),
        ("arr_timestamp", Hb_timestamp * ARR_SIZE),
        ("evicted_time", ctypes.c_uint64),
        ("next_evicted", ctypes.c_int),
        ("next_avail", ctypes.c_int)
    ]

# Message queue parameters
mq_name = '/ml_data'

# Open the existing message queue
mq = posix_ipc.MessageQueue(mq_name)

# Receive message from the queue
message, _ = mq.receive()

# Process the received message
received_data = ctypes.cast(message, ctypes.POINTER(Fd_info)).contents

delta_i = received_data.delta_i
ea = received_data.ea
evicted_time = received_data.evicted_time
next_evicted = received_data.next_evicted
next_avail = received_data.next_avail

print(delta_i)
print(ea)
print(evicted_time)

# arr_timestamp = received_data.arr_timestamp
# first_content = arr_timestamp[0]
# print(first_content.heartbeat_id)
# print(first_content.hb_timestamp)

# Close the message queue
mq.close()