import socket
import struct
import sys
from datetime import datetime
import queue


def time_to_string(hours, minutes):
    # ES IST
    message = "ES IST "
    
    # show minutes
    if minutes >= 5 and minutes < 10:
        message += "FUNF NACH "
    elif minutes >= 10 and minutes < 15:
        message += "ZEHN NACH "
    elif minutes >= 15 and minutes < 20:
        message += "VIERTEL NACH "
    elif minutes >= 20 and minutes < 25:
        message += "ZEHN VOR HALB "
    elif minutes >= 25 and minutes < 30:
        message += "FUNF VOR HALB "
    elif minutes >= 30 and minutes < 35:
        message += "HALB "
    elif minutes >= 35 and minutes < 40:
        message += "FUNF NACH HALB "
    elif minutes >= 40 and minutes < 45:
        message += "ZEHN NACH HALB "
    elif minutes >= 45 and minutes < 50:
        message += "VIERTEL VOR "
    elif minutes >= 50 and minutes < 55:
        message += "ZEHN VOR "
    elif minutes >= 55 and minutes < 60:
        message += "FUNF VOR "
    
    # convert hours to 12h format
    if hours >= 12:
        hours -= 12
    if minutes >= 20:
        hours += 1
    if hours == 12:
        hours = 0
    
    # show hours
    if hours == 0:
        message += "ZWOLF "
    elif hours == 1:
        message += "EIN"
        # EIN(S)
        if minutes > 4:
            message += "S"
        message += " "
    elif hours == 2:
        message += "ZWEI "
    elif hours == 3:
        message += "DREI "
    elif hours == 4:
        message += "VIER "
    elif hours == 5:
        message += "FUNF "
    elif hours == 6:
        message += "SECHS "
    elif hours == 7:
        message += "SIEBEN "
    elif hours == 8:
        message += "ACHT "
    elif hours == 9:
        message += "NEUN "
    elif hours == 10:
        message += "ZEHN "
    elif hours == 11:
        message += "ELF "
    
    if minutes < 5:
        message += "UHR "
    
    return message.strip()


# ip address of network interface
MCAST_IF_IP = '192.168.0.7'

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
saveCounter = 0

buffer = queue.Queue(20)

# Receive/respond loop
while True:
    data, address = sock.recvfrom(1024)
    data_str = data.decode("utf-8").strip()
    # get current time as hours and minutes
    now = datetime.now()
    hours = now.hour
    minutes = now.minute
    # convert time to string
    reference_str = time_to_string(hours, minutes)

    data_str = now.strftime('%b-%d-%Y_%H%M%S') + ": " + data_str
    print(address, ":" , data_str)
    buffer.put(data_str)
    if buffer.full():
        buffer.get()

    trigger_log = ""

    if "time as String: " in data_str:
        if reference_str in data_str:
            print("Time is correct")
        else:
            print("Time is not correct")
            print("Reference: ", reference_str)
            print("Received: ", data_str)
            trigger_log = "wrong time string"

    if "NTP-Update not successful" in data_str:
        trigger_log = "NTP not successful"

    if "Start program" in data_str:
        trigger_log = "Start program"

    if trigger_log:
        f = open("log_extended.txt",'a')
        f.write("LOGGING TRIGGERED: " + trigger_log + "\n")
        while not buffer.empty():    
            f.write(buffer.get())
            f.write("\n")
        f.write("(buffer end)\n")
        f.close()
        saveCounter = 20
        
    if saveCounter > 0:
        f = open("log_extended.txt",'a')
        f.write(data_str)
        f.write("\n")
        if saveCounter == 1:
            f.write("\n")
        f.close()
        saveCounter -= 1

