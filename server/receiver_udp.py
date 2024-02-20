import socket
import struct
import time

# Set up a UDP server
udp_ip = "0.0.0.0"  # Listen on all available IPs
udp_port = 4210  # The port number should match the one used in the ESP32 code

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP
sock.bind((udp_ip, udp_port))

print("Listening on port", udp_port)

last_time_received = None  # Initialize variable to store the last receive time

try:
    while True:
        data, addr = sock.recvfrom(1024)  # Buffer size of 1024 bytes
        current_time = time.time()  # Get current time

        if last_time_received is not None:
            time_since_last_package = current_time - last_time_received
            print(f"Time since last package: {time_since_last_package:.2f} seconds")

        floats = struct.unpack('9f', data)  # Unpack 9 floats
        print("Received floats:", floats, "from", addr)

        last_time_received = current_time  # Update last receive time
except KeyboardInterrupt:
    print("UDP server stopped.")
finally:
    sock.close()
