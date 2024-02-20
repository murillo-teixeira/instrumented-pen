import socket
import struct
import time
import select

# Set up a TCP server
tcp_ip = "0.0.0.0"
tcp_port = 4210
buffer_size = 1024

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((tcp_ip, tcp_port))
sock.listen(1)
print("Listening on port", tcp_port)

last_time_received = None

try:
    while True:
        print("Waiting for a connection...")
        conn, addr = sock.accept()
        print(f"Connection established with {addr}")

        conn.setblocking(0)  # Optional: Set the socket to non-blocking mode

        while True:
            ready_to_read, _, _ = select.select([conn], [], [], 2)  # 5 seconds timeout
            if ready_to_read:
                data = conn.recv(buffer_size)
                if not data:  # Check for disconnection
                    print("Connection closed by the client.")
                    break

                current_time = time.time()
                if last_time_received is not None:
                    time_since_last_package = current_time - last_time_received
                    print(f"Time since last package: {time_since_last_package:.2f} seconds")

                floats = struct.unpack('9f', data[:36])
                print("Received floats:", floats, "from", addr)
                last_time_received = current_time
            else:
                print("No data received. Checking for client connection...")
                conn.close()
                break
                
        conn.close()

except KeyboardInterrupt:
    print("TCP server stopped.")
finally:
    sock.close()
