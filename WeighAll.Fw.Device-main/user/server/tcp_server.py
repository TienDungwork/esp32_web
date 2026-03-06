import socket
import time

# Cấu hình server
HOST = '0.0.0.0'    # Lắng nghe mọi IP
PORT = 35000         # Phải trùng với port trong ESP32

# Tạo socket TCP
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(1)

print(f"🔌 Server đang chờ kết nối trên {HOST}:{PORT}...")

# Chấp nhận kết nối từ ESP32
client_socket, client_address = server_socket.accept()
print(f"✅ ESP32 đã kết nối từ: {client_address}")

# Gửi chuỗi lệnh phát nhạc
commands = [
    "sad\n"
    #"hello-1-2-3-go-4-5-sorry-6-7-8-9-stop\n",
    #"sad-troll-sad2\n",
    #"2-1-4\n"
]

for cmd in commands:
    print(f"📤 Gửi lệnh: {cmd.strip()}")
    client_socket.sendall(cmd.encode())
    time.sleep(10)  # Đợi 10 giây cho ESP32 phát xong
                   # Bạn có thể chỉnh lại cho phù hợp

# Đóng kết nối
client_socket.close()
server_socket.close()
print("🔒 Đã đóng kết nối.")
