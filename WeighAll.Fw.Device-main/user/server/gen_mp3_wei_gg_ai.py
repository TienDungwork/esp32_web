from google import genai
from google.genai import types
import os, mimetypes, struct

API_KEY = "AIzaSyAt6ozYYcO-qRXU7wfOE14nbIZHoqsDdZk"  # API key của bạn

def save_binary_file(file_name, data):
    with open(file_name, "wb") as f:
        f.write(data)
    print(f"✅ Saved: {file_name}")

def convert_to_wav(audio_data: bytes, mime_type: str) -> bytes:
    parameters = parse_audio_mime_type(mime_type)
    bits_per_sample = parameters["bits_per_sample"]
    sample_rate = parameters["rate"]
    num_channels = 1
    data_size = len(audio_data)
    bytes_per_sample = bits_per_sample // 8
    block_align = num_channels * bytes_per_sample
    byte_rate = sample_rate * block_align
    chunk_size = 36 + data_size
    header = struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", chunk_size, b"WAVE", b"fmt ", 16, 1,
        num_channels, sample_rate, byte_rate, block_align,
        bits_per_sample, b"data", data_size
    )
    return header + audio_data

def parse_audio_mime_type(mime_type: str):
    bits_per_sample, rate = 16, 24000
    for param in mime_type.split(";"):
        p = param.strip().lower()
        if p.startswith("rate="):
            try: rate = int(p.split("=")[1])
            except: pass
        elif "audio/l" in p:
            try: bits_per_sample = int(p.split("l")[1])
            except: pass
    return {"bits_per_sample": bits_per_sample, "rate": rate}

def generate():
    client = genai.Client(api_key=API_KEY)
    model = "gemini-2.5-pro-preview-tts"
    contents = [types.Content(role="user", parts=[types.Part.from_text(text="Mã QR không hợp lệ")])]
    config = types.GenerateContentConfig(
        temperature=1,
        response_modalities=["audio"],
        speech_config=types.SpeechConfig(
            voice_config=types.VoiceConfig(
                prebuilt_voice_config=types.PrebuiltVoiceConfig(voice_name="Zephyr")
            )
        ),
    )

    for i, chunk in enumerate(client.models.generate_content_stream(model=model, contents=contents, config=config)):
        if not chunk.candidates or not chunk.candidates[0].content.parts: continue
        part = chunk.candidates[0].content.parts[0]
        if part.inline_data and part.inline_data.data:
            ext = mimetypes.guess_extension(part.inline_data.mime_type) or ".wav"
            data = part.inline_data.data if ext != ".wav" else convert_to_wav(part.inline_data.data, part.inline_data.mime_type)
            save_binary_file(f"tts_output_{i}{ext}", data)

if __name__ == "__main__":
    generate()
