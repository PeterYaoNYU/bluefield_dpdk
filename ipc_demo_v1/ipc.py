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
received_data = ctypes.cast(message, ctypes.POINTER(ctypes.c_uint64*ARR_SIZE)).contents

for element in received_data:
    print(element)
# Close the message queue
mq.close()