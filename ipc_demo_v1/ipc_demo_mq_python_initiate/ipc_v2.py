import posix_ipc
import ctypes
import struct

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
queue_size = 10
message_size = ctypes.sizeof(ctypes.c_uint64) * ARR_SIZE

# create a new posix message queue
mq = posix_ipc.MessageQueue(mq_name, flags = posix_ipc.O_CREAT, mode = 0o666, max_messages = queue_size, max_message_size = message_size)

# Print the message queue's descriptor
print("Message queue created with descriptor:", mq.mqd)

# Receive message from the queue
message, _ = mq.receive()

# !!!
# we need to convert the binary data to the python data structure, using Struct package 

# Calculate the expected size of the received array
expected_size = ARR_SIZE * struct.calcsize('Q')

# Ensure that the received message has the expected size
if len(message) != expected_size:
    print(f"Received message size ({len(message)}) doesn't match the expected size ({expected_size}).")
    exit(1)
else:
    print("size matches")

# Interpret the received message as an array of uint64_t
received_array = struct.unpack(f'{ARR_SIZE}Q', message)

# Access the received array elements
for element in received_array:
    print(element, type(element))
# Close the message queue
mq.close()