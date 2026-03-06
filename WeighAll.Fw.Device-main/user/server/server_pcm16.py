import socket
import time
import os

HOST = '0.0.0.0'   # Lắng nghe tất cả địa chỉ IP
PORT = 1234        # Cổng TCP server

PCM_FILE = 'F:/congViec/Freelance/PlatformIO/Projects/Esp32S3_Audio/server/sad.pcm'
CHUNK_SIZE = 512   # Số byte gửi mỗi lần

# Cấu hình âm thanh
SAMPLE_RATE = 16000  # Hz
BITS_PER_SAMPLE = 16  # 16bit PCM
CHANNELS = 1          # Mono

def get_delay(chunk_size, sample_rate, bits_per_sample, channels):
    bytes_per_second = sample_rate * (bits_per_sample // 8) * channels
    return chunk_size / bytes_per_second

def main():
    delay = get_delay(CHUNK_SIZE, SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS)
    print(f"Calculated delay per chunk: {delay:.6f} seconds")

    if not os.path.exists(PCM_FILE):
        print(f"Error: File not found: {PCM_FILE}")
        return

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(1)
        print(f"Server listening on {HOST}:{PORT}")

        while True:
            print("Waiting for client to connect...")
            conn, addr = server.accept()
            print(f"Client connected from {addr}")

            with conn:
                with open(PCM_FILE, 'rb') as f:
                    while True:
                        data = f.read(CHUNK_SIZE)
                        if not data:
                            print("End of file reached, rewinding...")
                            f.seek(0)
                            continue
                        try:
                            conn.sendall(data)
                            time.sleep(delay)
                        except (BrokenPipeError, ConnectionResetError):
                            print("Client disconnected")
                            break

if __name__ == '__main__':
    main()
