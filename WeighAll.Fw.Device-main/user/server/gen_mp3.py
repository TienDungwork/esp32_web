from gtts import gTTS
import os

#words = ["một", "hai", "ba", "bốn", "năm", "sáu", "bảy", "tám", "chín", "mười"]
#words = ["xin chào quý khách", "xin mời qua", "xin dừng lại", "xin lỗi đã làm phiền quý khách"]
#words = ["xin chào quý khách", "xin mời qua", "xin dừng lại", "xin lỗi đã làm phiền quý khách"]
#words = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
words = [ "90"]

for word in words:
    tts = gTTS(text=word, lang='vi')
    filename = f"{word}.mp3"
    tts.save(filename)
    print(f"✅ Saved: {filename}")
