import socket
import binascii
import time
import struct
import random
# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Set the destination IP address and port number
ip_address = '192.168.2.113'  # Replace with the desired destination IP address
port = 6666  # Replace with the desired destination port number

sequence_number = 1

while (True):
    payload = struct.pack('!L', sequence_number)
    
    sock.sendto(payload, (ip_address, port))
    
    sequence_number += 1
    
    delay = random.expovariate(1/30) 
    
    print("adding a delay of ", delay, "ms")
    
    time.sleep(0.1)

# Close the socket
sock.close()
