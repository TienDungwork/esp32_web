# server_mp3_tcp.py
import socket

HOST = '0.0.0.0'     # Lắng nghe mọi IP
PORT = 8888          # Cổng bạn chọn, ví dụ: 8888

mp3_path = 'sad.mp3'  # Đường dẫn đến file MP3

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(1)
    print(f"[+] Listening on {HOST}:{PORT}")
    
    conn, addr = s.accept()
    with conn:
        print(f"[+] Connected by {addr}")
        with open(mp3_path, 'rb') as f:
            while True:
                data = f.read(1024)
                if not data:
                    break
                conn.sendall(data)
        print("[+] File sent completely.")
