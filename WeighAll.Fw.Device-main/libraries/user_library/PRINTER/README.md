# Hướng Dẫn Sử Dụng Thư Viện Printer

Thư viện `printer.cpp` được thiết kế để giao tiếp với máy in nhiệt, cung cấp các hàm để gửi lệnh và dữ liệu cho nhiều tác vụ in ấn khác nhau. Thư viện hỗ trợ định dạng văn bản cơ bản, in mã vạch và mã QR, in ảnh bitmap và hỗ trợ giới hạn cho ký tự tiếng Việt.

## Yêu Cầu
- Bao gồm tệp tiêu đề `printer.h` trong dự án của bạn.
- Đảm bảo hàm `printer_send_data` được triển khai để xử lý giao tiếp cấp thấp với máy in.
- Thư viện sử dụng các thư viện C tiêu chuẩn (`string.h`).

## Phân Loại Hàm
Các hàm trong thư viện được chia thành các nhóm sau:
1. **Lệnh Cơ Bản**: Khởi tạo, đặt lại và cấu hình cài đặt máy in.
2. **In Văn Bản**: In văn bản, dòng mới và ký tự tiếng Việt.
3. **In Mã Vạch**: In mã vạch 1D với các cài đặt tùy chỉnh.
4. **In Mã QR**: In mã QR với các tùy chọn phiên bản, mức sửa lỗi và phóng to.
5. **In Ảnh Bitmap**: In ảnh bitmap cao 8 pixel.

## Chi Tiết Hàm

### Lệnh Cơ Bản
Các hàm này cấu hình trạng thái và tùy chọn định dạng của máy in.

#### `printer_init(void)`
Khởi tạo máy in bằng cách gọi `printer_reset()`.
- **Cách dùng**: Gọi hàm này khi bắt đầu chương trình để đảm bảo máy in ở trạng thái đã biết.
- **Ví dụ**:
  ```c
  printer_init();
  ```

#### `printer_reset(void)`
Đặt lại máy in về cài đặt mặc định bằng lệnh ESC @.
- **Cách dùng**: Sử dụng để xóa các cài đặt trước đó.
- **Ví dụ**:
  ```c
  printer_reset();
  ```

#### `printer_set_line_spacing(uint8_t n)`
Cài đặt khoảng cách dòng với giá trị `n`.
- **Cách dùng**: Điều chỉnh khoảng cách giữa các dòng văn bản.
- **Ví dụ**:
  ```c
  printer_set_line_spacing(10); // Đặt khoảng cách dòng là 10
  ```

#### `printer_set_char_spacing(uint8_t n)`
Cài đặt khoảng cách giữa các ký tự với giá trị `n`.
- **Cách dùng**: Điều chỉnh khoảng cách giữa các ký tự trong văn bản.
- **Ví dụ**:
  ```c
  printer_set_char_spacing(2); // Đặt khoảng cách ký tự là 2
  ```

#### `printer_set_alignment(uint8_t align)`
Cài đặt căn lề văn bản (`0`: trái, `1`: giữa, `2`: phải).
- **Cách dùng**: Căn chỉnh văn bản trên giấy in.
- **Ví dụ**:
  ```c
  printer_set_alignment(1); // Căn giữa
  ```

#### `printer_set_bold(bool enable)`
Bật/tắt chế độ in đậm.
- **Cách dùng**: Sử dụng để làm nổi bật văn bản.
- **Ví dụ**:
  ```c
  printer_set_bold(true); // Bật in đậm
  ```

#### `printer_set_underline(bool enable)`
Bật/tắt chế độ gạch chân.
- **Cách dùng**: Thêm gạch chân cho văn bản.
- **Ví dụ**:
  ```c
  printer_set_underline(true); // Bật gạch chân
  ```

#### `printer_set_reverse(bool enable)`
Bật/tắt chế độ in ngược (nền đen, chữ trắng).
- **Cách dùng**: Tạo hiệu ứng đảo ngược màu.
- **Ví dụ**:
  ```c
  printer_set_reverse(true); // Bật chế độ in ngược
  ```

#### `printer_set_text_size(uint8_t width, uint8_t height)`
Cài đặt kích thước văn bản (chiều rộng và chiều cao từ 1 đến 8).
- **Lưu ý**: Giá trị ngoài khoảng 1-8 sẽ được đặt về 1.
- **Cách dùng**: Phóng to hoặc thu nhỏ văn bản.
- **Ví dụ**:
  ```c
  printer_set_text_size(2, 2); // Phóng to văn bản gấp 2 lần
  ```

### In Văn Bản
Các hàm này xử lý việc in văn bản và tạo dòng mới.

#### `printer_print_text(const char *text)`
In chuỗi văn bản được cung cấp.
- **Cách dùng**: In văn bản mà không xuống dòng.
- **Ví dụ**:
  ```c
  printer_print_text("Xin chao");
  ```

#### `printer_println(const char *text)`
In văn bản và thêm dòng mới (`\n`).
- **Cách dùng**: In văn bản và tự động xuống dòng.
- **Ví dụ**:
  ```c
  printer_println("Dong thu nhat"); // In và xuống dòng
  ```

#### `printer_new_line(uint8_t lines)`
Tạo số dòng mới được chỉ định.
- **Cách dùng**: Di chuyển xuống nhiều dòng mà không in văn bản.
- **Ví dụ**:
  ```c
  printer_new_line(2); // Xuống 2 dòng
  ```

### In Mã Vạch
Các hàm này hỗ trợ in mã vạch 1D với các tùy chọn cấu hình.

#### `printer_set_barcode_height(uint8_t height)`
Cài đặt chiều cao mã vạch.
- **Cách dùng**: Điều chỉnh kích thước chiều cao của mã vạch.
- **Ví dụ**:
  ```c
  printer_set_barcode_height(50); // Đặt chiều cao 50
  ```

#### `printer_set_barcode_width(uint8_t width)`
Cài đặt chiều rộng mã vạch.
- **Cách dùng**: Điều chỉnh độ dày của các vạch.
- **Ví dụ**:
  ```c
  printer_set_barcode_width(3); // Đặt chiều rộng 3
  ```

#### `printer_set_barcode_text_position(uint8_t pos)`
Cài đặt vị trí văn bản của mã vạch (`0`: không hiển thị, `1`: trên, `2`: dưới, `3`: cả trên và dưới).
- **Cách dùng**: Quy định vị trí văn bản mô tả mã vạch.
- **Ví dụ**:
  ```c
  printer_set_barcode_text_position(2); // Hiển thị văn bản dưới mã vạch
  ```

#### `printer_print_barcode(const char *data, uint8_t type)`
In mã vạch với dữ liệu và loại mã vạch được chỉ định.
- **Lưu ý**: Độ dài dữ liệu tối đa là 255 ký tự và cần kết thúc bằng null.
- **Cách dùng**: In mã vạch (ví dụ: CODE39, EAN13).
- **Ví dụ**:
  ```c
  printer_print_barcode("1234567890", 0x41); // In mã CODE39
  ```

### In Mã QR
Hàm này in mã QR với các tham số tùy chỉnh.

#### `printer_print_qrcode(const char *data, uint8_t version, uint8_t error_level, uint8_t zoom)`
In mã QR với dữ liệu, phiên bản, mức độ sửa lỗi và độ phóng đại.
- **Tham số**:
  - `data`: Chuỗi ký tự chứa nội dung mã QR (tối đa 255 ký tự).
  - `version`: Phiên bản mã QR (thường từ 1 đến 40).
  - `error_level`: Mức sửa lỗi (0: thấp, 1: cao).
  - `zoom`: Độ phóng to mã QR.
- **Cách dùng**: In mã QR cho các ứng dụng như liên kết hoặc thông tin thanh toán.
- **Ví dụ**:
  ```c
  printer_print_qrcode("https://example.com", 1, 2, 2); // In mã QR với phiên bản 1, mức sửa lỗi 2, phóng to gấp 2
  ```

### In Ảnh Bitmap
Hàm này hỗ trợ in ảnh bitmap với chiều cao hàng là 8 pixel.

#### `printer_print_bitmap(const uint8_t *bitmap_data, uint16_t width, uint16_t height)`
In ảnh bitmap với dữ liệu, chiều rộng và chiều cao được chỉ định.
- **Tham số**:
  - `bitmap_data`: Mảng chứa dữ liệu bitmap (mỗi byte đại diện cho 8 pixel).
  - `width`: Chiều rộng ảnh (bội số của 8).
  - `height`: Chiều cao ảnh.
- **Cách dùng**: In logo hoặc hình ảnh đơn giản.
- **Ví dụ**:
  ```c
  uint8_t bitmap[] = {0xFF, 0x00}; // Dữ liệu mẫu
  printer_print_bitmap(bitmap, 8, 8); // In ảnh 8x8 pixel
  ```

### In Tiếng Việt
Hàm này xử lý văn bản tiếng Việt với hỗ trợ ánh xạ Unicode đơn giản.

#### `printer_print_vietnamese(const char *utf8_text)`
In văn bản UTF-8 tiếng Việt bằng cách ánh xạ các ký tự Unicode sang ASCII gần đúng.
- **Lưu ý**: Hiện chỉ hỗ trợ ánh xạ giới hạn (ví dụ: `á` → `a`).
- **Cách dùng**: In văn bản tiếng Việt trên máy in không hỗ trợ Unicode đầy đủ.
- **Ví dụ**:
  ```c
  printer_print_vietnamese("Tiếng Việt"); // In "Tieng Viet"
  ```

## Ví Dụ Minh Họa
Dưới đây là một chương trình mẫu sử dụng thư viện để in biên lai:

```c
#include "printer.h"

int main() {
    printer_init(); // Khởi tạo máy in
    printer_set_alignment(1); // Căn giữa
    printer_set_bold(true); // In đậm
    printer_println("CUA HANG ABC");
    printer_set_bold(false);
    printer_new_line(1);

    printer_set_alignment(0); // Căn trái
    printer_set_text_size(1, 1); // Kích thước chữ bình thường
    printer_println("San pham: Banh Mi");
    printer_println("Gia: 20.000 VND");
    printer_new_line(1);

    // In mã QR
    printer_print_qrcode("https://abc.com", 1, 2, 2);

    // In mã vạch
    printer_set_barcode_height(50);
    printer_set_barcode_text_position(2);
    printer_print_barcode("1234567890", 0x41);

    printer_new_line(2);
    printer_print_vietnamese("Cảm ơn quý khách!");
    printer_new_line(3);

    return 0;
}
```

## Lưu Ý
- Hàm `printer_send_data` phải được triển khai để gửi dữ liệu đến máy in (thường qua UART).
- Hỗ trợ tiếng Việt hiện tại rất hạn chế. Để hỗ trợ đầy đủ, cần mở rộng hàm `convert_utf8_to_ascii`.
- Kiểm tra tài liệu của máy in để xác định các giá trị hợp lệ cho các tham số như `type` trong mã vạch hoặc `version` trong mã QR.
- Đảm bảo dữ liệu bitmap được định dạng đúng (mỗi byte đại diện cho 8 pixel theo chiều dọc).
