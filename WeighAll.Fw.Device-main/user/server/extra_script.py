import numpy as np
import matplotlib.pyplot as plt

# ===== Cấu hình =====
pcm_file = 'F:/congViec/Freelance/PlatformIO/Projects/Esp32S3_Audio/server/sad.pcm'       # Đường dẫn file PCM
sample_rate = 16000         # Hz
bits_per_sample = 16        # 16-bit
duration_sec = 2            # Hiển thị 2 giây đầu

# ===== Đọc dữ liệu PCM =====
with open(pcm_file, 'rb') as f:
    # Số mẫu cần đọc = sample_rate * duration (giây)
    num_samples = sample_rate * duration_sec
    raw = f.read(num_samples * 2)  # 2 byte mỗi mẫu

# Chuyển thành mảng numpy
data = np.frombuffer(raw, dtype=np.int16)

# Tạo trục thời gian
time_axis = np.linspace(0, duration_sec, num=len(data))

# ===== Vẽ đồ thị sóng âm =====
plt.figure(figsize=(12, 4))
plt.plot(time_axis, data, color='blue')
plt.title(f'PCM Waveform - {pcm_file}')
plt.xlabel('Time (s)')
plt.ylabel('Amplitude')
plt.grid(True)
plt.tight_layout()
plt.show()
