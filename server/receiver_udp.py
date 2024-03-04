import socket
import struct
import time
import sqlite3
import shutil

# Set up a UDP server
udp_ip = "0.0.0.0"  # Listen on all available IPs
udp_port = 4210  # The port number should match the one used in the ESP32 code

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP
sock.bind((udp_ip, udp_port))

# SQLite database setup
db_name = 'data/sensor_data.db'

# Connect to the SQLite database
conn = sqlite3.connect(db_name)
cursor = conn.cursor()

# clear the database
try:
    cursor.execute('DELETE FROM orientation')
    cursor.execute('DELETE FROM battery_levels')
    cursor.execute('DELETE FROM reaction_times')
    conn.commit()
except:
    print("Empty database")

# Create tables if they don't exist
cursor.execute('''CREATE TABLE IF NOT EXISTS orientation (
                  id INTEGER PRIMARY KEY,
                  received_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                  timestamp REAL, yaw REAL, pitch REAL, roll REAL, acc_x REAL,
                  acc_y REAL, acc_z REAL, gyro_x REAL, gyro_y REAL, gyro_z REAL)''')

cursor.execute('''CREATE TABLE IF NOT EXISTS battery_levels (
                  id INTEGER PRIMARY KEY,
                  received_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                  level REAL)''')

cursor.execute('''CREATE TABLE IF NOT EXISTS reaction_times (
                  id INTEGER PRIMARY KEY,
                  received_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
                  time REAL)''')

print("Listening on port", udp_port)

try:
    while True:
        data, addr = sock.recvfrom(1024)  # Buffer size of 1024 bytes
        current_time = time.time()  # Get current time

        messageType = data[0]

        if messageType == 1:  # Float array
            floats = struct.unpack('10f', data[1:])
            print("Received float array:", floats, "from", addr)
            # Insert into the database
            cursor.execute('INSERT INTO orientation (timestamp, yaw, pitch, roll, acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)', floats)
        
        elif messageType == 2:  # Battery level
            batteryLevel = struct.unpack('f', data[1:5])[0]
            # print("Received battery level:", batteryLevel, "from", addr)
            # Insert into the database
            cursor.execute('INSERT INTO battery_levels (level) VALUES (?)', (batteryLevel,))
        
        elif messageType == 3:  # Reaction time
            reactionTime = struct.unpack('f', data[1:5])[0]
            # print("Received reaction time:", reactionTime, "from", addr)
            # Insert into the database
            cursor.execute('INSERT INTO reaction_times (time) VALUES (?)', (reactionTime,))

        conn.commit()  # Commit the transaction

except KeyboardInterrupt:
    print("UDP server stopped.")
finally:
    sock.close()
    conn.close()  # Close the database connection
    shutil.copy2(db_name, db_name.split('.')[0] + '_' + time.strftime('%Y_%m_%d_%H_%M_%S') + '.db')
    
