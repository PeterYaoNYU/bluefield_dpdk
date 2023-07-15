import socket
import time
import struct
import random
import binascii

# Create a raw socket using AF_PACKET address family
sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)

# Set the network interface name or index
network_interface = 'ens1f1'  

# Bind the socket to the network interface
sock.bind((network_interface, 0))

# Set the destination MAC address
destination_mac = '08:c0:eb:d1:fc:56'  # Replace with the desired destination MAC address

# Set the destination IP address and port number
ip_address = '192.168.2.113'  # Replace with the desired destination IP address
port = 6666  # Replace with the desired destination port number

sequence_number = 1

while True:
    print(sequence_number)
    
    # Create the Ethernet frame
    source_mac = ':'.join(['%02x' % random.randint(0, 255) for _ in range(6)])  # Generate random source MAC address
    ethertype = b'\x08\x00'  # EtherType for IP packets

    # Create the IP packet
    ip_version = 4
    ip_header_length = 5
    ip_tos = 0
    ip_total_length = 20 + 8  # IP header length + UDP length
    ip_identifier = 1234  # Replace with a unique identifier
    ip_flags = 0
    ip_ttl = 64
    ip_protocol = socket.IPPROTO_UDP
    ip_source_address = '192.168.2.109'  # Replace with the source IP address
    ip_destination_address = ip_address

    # Create the IP header
    ip_header = struct.pack('!BBHHHBBH4s4s', (ip_version << 4) + ip_header_length, ip_tos, ip_total_length, ip_identifier, (ip_flags << 13), ip_ttl, ip_protocol, 0, socket.inet_aton(ip_source_address), socket.inet_aton(ip_destination_address))

    # Create the UDP packet
    udp_source_port = 6666 
    udp_destination_port = 6666
    udp_length = 8  # Length of UDP header + payload
    udp_checksum = 0  # You can calculate and set the correct checksum if needed

    # Create the UDP header
    udp_header = struct.pack('!HHHH', udp_source_port, udp_destination_port, udp_length, udp_checksum)

    payload = struct.pack('!L', sequence_number)
    sequence_number += 1

    # Construct the UDP packet
    udp_packet = udp_header + payload

    # Construct the IP packet
    ip_packet = ip_header + udp_packet

    # Construct the Ethernet frame
    eth_frame = binascii.unhexlify(destination_mac.replace(':', '')) + binascii.unhexlify(source_mac.replace(':', '')) + ethertype + ip_packet

    # Send the Ethernet frame
    sock.send(eth_frame)

    # Delay before sending the next packet
    time.sleep(0.1)

# Close the socket
sock.close()
