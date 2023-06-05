import socket

def receive_heartbeat(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((ip, port))

    while True:
        try:
            data, addr = sock.recvfrom(1024)
            print("Received heartbeat from {}:{}".format(addr[0], addr[1]))
        except socket.error as e:
            print("Error receiving heartbeat:", e)
            break

    sock.close()

# Specify the IP address and port to receive the heartbeat from
ip_address = "10.214.96.107"
port =8848 

receive_heartbeat(ip_address, port)

