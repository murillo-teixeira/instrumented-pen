import socket
import time

# The target IP address and port number should match your UDP server settings
udp_ip = "127.0.0.1"  # Use localhost to communicate within the same machine
udp_port = 4210  # Ensure this matches the port number of your UDP server

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Optionally, you can set a timeout for the socket operations
# sock.settimeout(2.0)  # 2 seconds timeout

def send_data(message):
    print("Sending message:", message)
    sock.sendto(message.encode(), (udp_ip, udp_port))

try:
    while True:
        # Send data to the server at 100Hz (every 10ms)
        send_data("Hello, World!")
        time.sleep(0.01)  # 10ms delay to simulate 100Hz data rate
except KeyboardInterrupt:
    print("UDP client stopped.")
finally:
    sock.close()
