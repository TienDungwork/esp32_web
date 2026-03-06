import socket

SERVER_IP = '192.168.0.105'
SERVER_PORT = 35000


def build_message(device_type):
    return (
        '{"Code":1,'
        '"Message":"{\\"actived\\":false,'
        '\\"data_updated_at\\":\\"0001-01-01T00:00:00\\",'
        '\\"working_updated_at\\":\\"0001-01-01T00:00:00\\",'
        '\\"is_online\\":true,'
        '\\"no_pong_consecutive_num\\":0,'
        '\\"connection_percentage\\":100,'
        '\\"type\\":' + str(device_type) + '}",'
        '"Status":0,'
        '"DeviceType":' + str(device_type) + ','
        '"CreatedAt":"0001-01-01T00:00:00",'
        '"IndexInPacket":0}<EOF>'
    )


def build_ping():
    return (
        '{"Code":2,'
        '"Message":"",'
        '"Status":0,'
        '"DeviceType":0,'
        '"CreatedAt":"0001-01-01T00:00:00",'
        '"IndexInPacket":0}<EOF>'
    )


device_list = [8, 9, 51, 53, 54, 55, 56, 57, 60, 58, 59, 101, 102, 103]

# Tạo payload gồm tất cả device connect và 1 ping
payload = ""
for dev in device_list:
    payload += build_message(dev)

# Thêm gói ping
payload += build_ping()

try:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as client:
        client.connect((SERVER_IP, SERVER_PORT))
        print(f"Connected to {SERVER_IP}:{SERVER_PORT}")

        client.sendall(payload.encode())
        print(f"Sent:\n{payload}")

        client.settimeout(3)
        try:
            response = client.recv(8192)
            print("Received:", response.decode())
        except socket.timeout:
            print("No response received.")

except Exception as e:
    print("Connection error:", e)
