from gtts import gTTS
import os

# Thư mục lưu file
output_dir = r"user\data_sd_card"
os.makedirs(output_dir, exist_ok=True)

# Danh sách mã và câu nói tương ứng
speech_map = {
    0: "0",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7",
    8: "8",
    9: "9",

    10: "A", 11: "B", 12: "C", 13: "D", 14: "E", 15: "F",
    16: "G", 17: "H", 18: "I", 19: "J", 20: "K", 21: "L",
    22: "M", 23: "N", 24: "O", 25: "P", 26: "Q", 27: "R",
    28: "S", 29: "T", 30: "U", 31: "V", 32: "W", 33: "X",
    34: "Y", 35: "Z",

    36: "Khoảng trắng",  # khoảng trắng
    37: "Xóa bỏ",

    100: "Mã QR không hợp lệ",
    101: "Trạm cân đang bận, vui lòng thử lại sau",
    102: "Mời xe qua trạm",
    103: "Đã hoàn thành, mời di chuyển",
    104: "Xin mời xe tiến về phía trước",
    105: "Hệ thống chưa cân xong, mời lùi lại",
    106: "Xong",
    107: "Lỗi"
}

# Sinh file âm thanh
for code, text in speech_map.items():
    tts = gTTS(text=text, lang='vi')
    filename = os.path.join(output_dir, f"{code}.mp3")
    tts.save(filename)
    print(f"✅ Saved: {filename}")

print("🎯 Hoàn tất tạo file âm thanh.")
