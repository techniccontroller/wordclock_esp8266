import socket
import struct
import sys
from datetime import datetime
import queue

# ip address of network interface
MCAST_IF_IP = '192.168.178.38'

multicast_group = '230.120.10.2'
server_address = ('', 8123)


def start(filters=None):
    if filters is None:
        filters = []

    # Create the socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # Bind to the server address
    sock.bind(server_address)

    print("Start")

    # Tell the operating system to add the socket to the multicast group on the specified interface
    group = socket.inet_aton(multicast_group)
    mreq = struct.pack('4s4s', group, socket.inet_aton(MCAST_IF_IP))
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    print("Ready")

    # Initialize buffers and save counters for each filter
    buffers = {filter_val: queue.Queue(20) for filter_val in filters}
    save_counters = {filter_val: 0 for filter_val in filters}

    # Receive/respond loop
    while True:
        data, address = sock.recvfrom(1024)
        data_str = data.decode("utf-8").strip()
        timestamped_data = f"[{address[0]} - {datetime.now().strftime('%b-%d-%Y_%H:%M:%S')}] {data_str}"

        # Check each filter and process data accordingly
        for filter_val in filters:
            if filter_val in data_str:
                print(timestamped_data)
                buffers[filter_val].put(timestamped_data)
                if buffers[filter_val].full():
                    buffers[filter_val].get()

                # Save data if specific keywords are found or if save counter is active
                if "NTP-Update not successful" in data_str or "Start program" in data_str:
                    with open(f"log_{filter_val}.txt", 'a') as f:
                        while not buffers[filter_val].empty():
                            f.write(buffers[filter_val].get() + "\n")
                    save_counters[filter_val] = 20  # Start the save counter

                if save_counters[filter_val] > 0:
                    with open(f"log_{filter_val}.txt", 'a') as f:
                        f.write(timestamped_data + "\n")
                        if save_counters[filter_val] == 1:
                            f.write("\n")
                    save_counters[filter_val] -= 1


# Main
if __name__ == '__main__':
    # Check if filters are given
    # Use filters as arguments: python3 multicastUDP_receiver.py "filter1" "filter2" ...
    if len(sys.argv) > 1:
        start(sys.argv[1:])
    else:
        start()
