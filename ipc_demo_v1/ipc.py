import posix_ipc

# Message queue parameters
mq_name = '/ml_data'

# Open the existing message queue
mq = posix_ipc.MessageQueue(mq_name)

# Receive message from the queue
message, _ = mq.receive()

# Process the received message
print("Received message:", message.decode())

# Close the message queue
mq.close()