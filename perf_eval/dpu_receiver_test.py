import socket
import struct

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Set the IP address and port to listen on
ip_address = '0.0.0.0'  # Listen on all available network interfaces
port = 6666  # Choose a port number

# Bind the socket to the IP address and port
sock.bind((ip_address, port))

while True:
    # Receive a packet (payload) and the address it was sent from
    payload, address = sock.recvfrom(4)  # Assuming the payload size is 4 bytes

    # Unpack the payload to get the sequence number
    sequence_number = struct.unpack('!L', payload)[0]

    print(f"Received packet with sequence number: {sequence_number} from {address[0]}:{address[1]}")

# Close the socket
sock.close()

