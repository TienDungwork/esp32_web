from PIL import Image
import sys


def convert_image_to_bitmap(image_path, output_width=384):
    # Mở ảnh và chuyển sang đen trắng
    img = Image.open(image_path).convert('L')

    # Resize giữ tỉ lệ, chiều rộng theo máy in (384px là phổ biến)
    width_percent = (output_width / float(img.size[0]))
    height_size = int((float(img.size[1]) * float(width_percent)))
    img = img.resize((output_width, height_size))

    # Chuyển sang ảnh nhị phân (đen trắng)
    threshold = 128
    img = img.point(lambda p: 0 if p < threshold else 255, '1')

    img_data = img.tobytes()

    width_bytes = (img.width + 7) // 8
    height = img.height

    bitmap = bytearray()

    for y in range(0, height):
        for x in range(0, width_bytes):
            byte = 0
            for bit in range(0, 8):
                pixel_x = x * 8 + bit
                pixel_y = y
                if pixel_x >= img.width:
                    continue
                pixel = img.getpixel((pixel_x, pixel_y))
                if pixel == 0:  # Pixel đen
                    byte |= (1 << (7 - bit))
            bitmap.append(byte)

    return bitmap, width_bytes, height


def save_bitmap_to_header(bitmap, width_bytes, height, output_file):
    with open(output_file, 'w') as f:
        f.write(f"const uint8_t image_bitmap[] = {{\n")
        for i, byte in enumerate(bitmap):
            f.write(f"0x{byte:02X}, ")
            if (i + 1) % 12 == 0:
                f.write("\n")
        f.write("\n};\n")
        f.write(f"const uint16_t image_width_bytes = {width_bytes};\n")
        f.write(f"const uint16_t image_height = {height};\n")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python convert_bitmap.py input_image output_header.h")
        sys.exit(1)

    image_path = sys.argv[1]
    output_path = sys.argv[2]

    bitmap, width_bytes, height = convert_image_to_bitmap(image_path)
    save_bitmap_to_header(bitmap, width_bytes, height, output_path)

    print(f"Bitmap generated: {output_path}")
    print(f"Width in bytes: {width_bytes}, Height: {height}")
