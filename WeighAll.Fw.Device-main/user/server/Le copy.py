from pydub import AudioSegment
from pydub.effects import strip_silence
from gtts import gTTS
import os

words = ["chín mươi chín", "A", "một", "hai", "ba", "bốn", "năm"]

# Tạo file mp3 cho từng từ nếu chưa có
for word in words:
    filename = f"{word}.mp3"
    if not os.path.exists(filename):
        tts = gTTS(text=word, lang='vi')
        tts.save(filename)

# Ghép các file
final_audio = AudioSegment.empty()
for word in words:
    audio = AudioSegment.from_mp3(f"{word}.mp3")
    # Cắt khoảng lặng đầu và cuối (50ms)
    audio = strip_silence(audio, silence_len=50, silence_thresh=audio.dBFS - 16, padding=10)
    final_audio += audio + AudioSegment.silent(duration=50)  # thêm 50ms để tự nhiên

final_audio.export("99A12345.mp3", format="mp3")
print("✅ Đã ghép thành công!")
