import socket
import struct
import sys

# ip address of network interface
MCAST_IF_IP = '192.168.0.4'

multicast_group = '230.120.10.2'
server_address = ('', 8123)

# Create the socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind to the server address
sock.bind(server_address)

print("Start")

# Tell the operating system to add the socket to the multicast group
# on all interfaces.
group = socket.inet_aton(multicast_group)
mreq = struct.pack('4s4s', group, socket.inet_aton(MCAST_IF_IP))
sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

print("Ready")

# Receive/respond loop
while True:
    data, address = sock.recvfrom(1024)
    print(address, ": ", data.decode("utf-8").strip())

