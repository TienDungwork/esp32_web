import socket
import threading
import json
import random
import time
from datetime import datetime

HOST = "0.0.0.0"
PORT = 35000

clients = []  # list: (conn, addr)

def handle_client(conn, addr):
    print(f"[NEW CONNECTION] {addr} connected.")
    buffer = ""

    # Thread gửi gói test ngược lại
    threading.Thread(target=send_random_packets, args=(conn, addr), daemon=True).start()

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            buffer += data.decode(errors="ignore")

            while "<EOF>" in buffer:
                packet, buffer = buffer.split("<EOF>", 1)
                if packet.strip():
                    process_packet(conn, addr, packet.strip())

    except ConnectionResetError:
        print(f"[DISCONNECTED] {addr} - connection reset.")
    finally:
        conn.close()
        clients.remove((conn, addr))
        print(f"[CLOSED] {addr} removed from client list.")

def process_packet(conn, addr, packet):
    try:
        data_json = json.loads(packet)
        code = data_json.get("Code")
        dev_type = data_json.get("DeviceType")
        message = data_json.get("Message")
        ts = datetime.now().strftime("%H:%M:%S")

        # ESP1
        if code == 1:
            msg_json = json.loads(message)
            print(f"[{ts}] ESP1({addr}) Báo danh - DeviceType={msg_json.get('DeviceType')}")
        elif code == 2:
            print(f"[{ts}] ESP1({addr}) Ping")
            pong_packet = json.dumps({
                "Code": 2,
                "Message": "pong",
                "Status": 0
            }) + "<EOF>"
            conn.sendall(pong_packet.encode())
        elif code in [201, 401, 801, 501]:
            print(f"[{ts}] ESP1({addr}) Code={code} -> {message}")

        # ESP2
        elif code == 101:
            print(f"[{ts}] ESP2({addr}) Code=101 -> {message}")

        # Khác
        else:
            print(f"[{ts}] {addr} Code={code} -> {message}")

    except json.JSONDecodeError:
        print(f"[INVALID JSON] from {addr} -> {packet}")

def send_random_packets(conn, addr):
    """Gửi gói random mỗi 5 giây cho client."""
    while True:
        time.sleep(5)
        if (conn, addr) not in clients:
            break

        # Random gửi cho ESP1 hoặc ESP2
        random_code = random.choice([201, 401, 801, 501, 101])
        if random_code == 101:
            # Gửi cho ESP2
            payload = {
                "Code": 101,
                "Message": "tiktok,1,2,3,4,5",
                "Status": 0,
                "DeviceType": 0,
                "CreatedAt": datetime.utcnow().isoformat() + "Z",
                "IndexInPacket": 0
            }
        else:
            # Gửi cho ESP1
            payload = {
                "Code": random_code,
                "Message": f"Test message code {random_code}",
                "Status": 0,
                "DeviceType": random.randint(1, 10),
                "CreatedAt": datetime.utcnow().isoformat() + "Z",
                "IndexInPacket": 0
            }

        try:
            conn.sendall((json.dumps(payload) + "<EOF>").encode())
            print(f"[SEND TEST] {addr} -> Code={payload['Code']}")
        except:
            break

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    print(f"[LISTENING] TCP server on {HOST}:{PORT}")

    while True:
        conn, addr = server.accept()
        clients.append((conn, addr))
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()

if __name__ == "__main__":
    start_server()
