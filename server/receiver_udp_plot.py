import socket
import struct
import threading
import matplotlib.pyplot as plt
import numpy as np
import time

# Global variable to store the latest data
data_storage = {"time": [], "yaw_pitch_roll": [], "acceleration": [], "gyro": []}
lock = threading.Lock()

# Set up a UDP server
udp_ip = "0.0.0.0"  # Listen on all available IPs
udp_port = 4210  # The port number should match the one used in the ESP32 code

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP
sock.bind((udp_ip, udp_port))
sock.settimeout(0.01)  # Non-blocking mode

print("Listening on port", udp_port)

def udp_listener():
    try:
        while True:
            try:
                data, addr = sock.recvfrom(1024)  # Buffer size of 1024 bytes
                floats = struct.unpack('10f', data)  # Unpack 10 floats
                
                with lock:
                    # Store the latest data
                    data_storage["time"].append(floats[0])
                    data_storage["yaw_pitch_roll"].append(floats[1:4])
                    data_storage["acceleration"].append(floats[4:7])
                    data_storage["gyro"].append(floats[7:10])
                    
                    # Keep the last N elements for real-time plotting
                    N = 100  # Adjust based on how much history you want to display
                    for key in data_storage:
                        data_storage[key] = data_storage[key][-N:]
                
            except socket.timeout:
                continue
    except KeyboardInterrupt:
        print("UDP listener stopped.")
    finally:
        sock.close()

# Start UDP listener in a separate thread
threading.Thread(target=udp_listener, daemon=True).start()

# Setup the plot
plt.ion()  # Interactive mode on
fig, axs = plt.subplots(3, 1, figsize=(10, 8))
axs[0].set_ylim(-180, 180)
axs[1].set_ylim(-2, 2)
axs[2].set_ylim(-250, 250)

def update_plot():
    while True:
        time.sleep(0.05)  # Update plot every 100 ms
        with lock:
            if data_storage["time"]:  # If there's data
                for i, key in enumerate(["yaw_pitch_roll", "acceleration", "gyro"]):
                    axs[i].clear()
                    axs[i].plot(data_storage["time"], np.array(data_storage[key]))
                    axs[i].set_title(key)
                axs[0].set_ylim(-180, 180)
                axs[1].set_ylim(-2, 2)
                axs[2].set_ylim(-500, 500)
                plt.pause(0.001)  # Pause needed to update the plot

update_plot()
