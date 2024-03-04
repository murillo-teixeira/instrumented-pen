import socket

# The IP address of the ESP32-C3 or the broadcast address
# Use the broadcast address if you want to send to all devices on the network
# For example, '192.168.1.255' for a typical home network
udp_ip_address = "192.168.10.7"
udp_port = 4210  # The port number should match the one used by the ESP32-C3

# Creating a socket for UDP
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Enable broadcasting mode
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

message = "print_hi"

try:
    # Sending the message
    sock.sendto(message.encode(), (udp_ip_address, udp_port))
    print(f"Message '{message}' sent to {udp_ip_address}:{udp_port}")
finally:
    # Close the socket
    sock.close()
