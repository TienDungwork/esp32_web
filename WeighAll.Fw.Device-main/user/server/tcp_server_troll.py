import socket
import time

# Danh sách lời bài hát
lyrics = [
    "Ta chẳng hợp nhau đâu là câu nói khi em bắt đầu",
    "Lo âu nhiều điều tốt xấu, nghĩ ta chẳng đi đến đâu",
    "Em đừng giận anh nhé, vì có lẽ anh đã xem nhẹ",
    "Chuyện gắn bó một đời với người con gái tuyệt vời",
    "Để rồi anh đánh mất cơ hội làm người sánh bước bên em",
    "Cầm tay em đứng trước lễ đường mọi người cùng ngước lên xem",
    "Nhìn hoàng hôn đang xuống sau đồi từ giờ chỉ thấy đêm đen",
    "Anh lùi về quá khứ, em sẽ hạnh phúc chứ?",
    "Dĩ nhiên rồi",
    "Người ta là sự lựa chọn của em mà",
    "Chẳng phải sống hối hả, chạy theo hi vọng hoá đá",
    "Những thăng trầm nay hoá phong ba trời đổ cơn mưa",
    "Đến khi mưa tạnh rồi anh mang ô tới em đi mất rồi",
    "Có đôi lần ở lại níu tay em nhận sự thương hại",
    "Người yếu đuối thật ngây ngô, chẳng dám xoá mờ kí ức",
    "Vì nỗi nhớ ở đây, dấu trong tận tim này",
    "Giá như không bắt đầu, thì ngày kết thúc chẳng đâu như vậy"
]

# Cấu hình server
HOST = '0.0.0.0'
PORT = 5000

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(1)

print(f"🔌 Server đang chờ kết nối trên {HOST}:{PORT}...")

client_socket, client_address = server_socket.accept()
print(f"✅ ESP32 đã kết nối từ: {client_address}")

# Gửi lệnh phát nhạc
client_socket.sendall(b"sad\n")
print("📤 Đã gửi lệnh: sad")

# Hiện lời bài hát như karaoke
print("🎤 Bắt đầu hiện lời bài hát:\n")
for line in lyrics:
    print(line)
    time.sleep(2.5)  # Hiện từng dòng cách nhau 2.5 giây

# Đóng kết nối
client_socket.close()
server_socket.close()
print("🔒 Đã đóng kết nối.")
