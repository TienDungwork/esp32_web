#include "ethernet_ota.h"


// Đọc một dòng HTTP (kết thúc bằng \r\n)
static bool httpReadLine(Stream& s, String& out, uint32_t timeout_ms = 15000) {
  uint32_t start = millis();
  out = "";
  while (millis() - start < timeout_ms) {
    if (s.available()) {
      char c = s.read();
      if (c == '\r') continue;
      if (c == '\n') return true;
      out += c;
    } else {
      delay(1);
    }
  }
  return false;
}

// Đọc hết header, trả về:
//  - statusCode
//  - contentLength (-1 nếu không có)
//  - isChunked
//  - md5Sum (nếu server gửi ETag hoặc x-checksum nào đó bạn quy ước; ở đây không ép)
static bool httpReadHeaders(Stream& s, int& statusCode, long& contentLength, bool& isChunked) {
  statusCode = 0;
  contentLength = -1;
  isChunked = false;

  String line;
  // Status line: HTTP/1.1 200 OK
  if (!httpReadLine(s, line)) return false;
  // Tìm mã trạng thái
  {
    int sp1 = line.indexOf(' ');
    int sp2 = (sp1 >= 0) ? line.indexOf(' ', sp1 + 1) : -1;
    if (sp1 >= 0 && sp2 > sp1) {
      statusCode = line.substring(sp1 + 1, sp2).toInt();
    }
  }

  // Header lines
  while (true) {
    if (!httpReadLine(s, line)) return false;
    if (line.length() == 0) break; // hết header
    String key = line;
    key.toLowerCase();

    if (key.startsWith("content-length:")) {
      String v = line.substring(strlen("Content-Length:"));
      v.trim();
      contentLength = v.toInt();
    } else if (key.startsWith("transfer-encoding:")) {
      if (key.indexOf("chunked") >= 0) isChunked = true;
    }
  }
  return true;
}

// Đọc dữ liệu kiểu chunked và ghi Update.write
static bool httpStreamChunkedToUpdate(Stream& s, size_t& totalWritten, uint32_t reportEveryMs = 500) {
  String line;
  totalWritten = 0;
  uint32_t lastReport = 0;

  while (true) {
    // mỗi chunk bắt đầu bằng "size\r\n" (hex)
    if (!httpReadLine(s, line)) {
      PrintDebugLn("[OTA] Failed to read chunk size line");
      return false;
    }
    line.trim();
    long chunkSize = strtol(line.c_str(), nullptr, 16);
    if (chunkSize <= 0) {
      // chunk cuối cùng (0) -> đọc qua CRLF kế tiếp (trailer bỏ qua)
      // đọc thêm một dòng trống nếu có
      httpReadLine(s, line); // thường là trống
      break;
    }

    size_t remaining = (size_t)chunkSize;
    const size_t BUFSZ = 1024;
    uint8_t buf[BUFSZ];

    while (remaining > 0) {
      size_t toRead = remaining > BUFSZ ? BUFSZ : remaining;
      size_t n = s.readBytes(buf, toRead);
      if (n == 0) {
        PrintDebugLn("[OTA] Socket stalled while reading chunk body");
        return false;
      }
      size_t w = Update.write(buf, n);
      if (w != n) {
        PrintDebugLn("[OTA] Update.write failed (chunked)");
        return false;
      }
      totalWritten += n;
      remaining -= n;

      if (millis() - lastReport > reportEveryMs) {
        PrintDebugf("[OTA] Written: %u bytes (chunked)\n", (unsigned)totalWritten);
        lastReport = millis();
      }
    }

    // kết thúc chunk có "\r\n"
    char cr = s.read();
    char nl = s.read();
    if (cr != '\r' || nl != '\n') {
      PrintDebugLn("[OTA] Invalid chunk terminator");
      return false;
    }
  }
  return true;
}

// Đọc dữ liệu có Content-Length và ghi Update.write (show %)
static bool httpStreamLenToUpdate(Stream& s, long contentLength, size_t& totalWritten, uint32_t reportEveryMs = 500) {
  const size_t BUFSZ = 1024;
  uint8_t buf[BUFSZ];
  totalWritten = 0;
  uint32_t lastReport = 0;

  while (totalWritten < (size_t)contentLength) {
    int toRead = (contentLength - totalWritten) > BUFSZ ? BUFSZ : (contentLength - totalWritten);
    int n = s.readBytes(buf, toRead);
    if (n <= 0) {
      PrintDebugLn("[OTA] Socket stalled while reading body");
      return false;
    }
    size_t w = Update.write(buf, n);
    if (w != (size_t)n) {
      PrintDebugLn("[OTA] Update.write failed");
      return false;
    }
    totalWritten += n;

    if (millis() - lastReport > reportEveryMs && contentLength > 0) {
      int percent = (int)((totalWritten * 100UL) / contentLength);
      PrintDebugf("[OTA] %d%% (%u/%ld bytes)\n", percent, (unsigned)totalWritten, contentLength);
      lastReport = millis();
    }
  }
  return true;
}

/**
 * HTTP OTA 1 file .bin
 * - host: có thể là IP dạng chuỗi "192.168.1.10" hoặc domain
 * - port: ví dụ 80
 * - path: ví dụ "/firmware.bin"
 * - expectedMD5: null nếu không kiểm MD5; nếu có, server & file phải trùng MD5
 */
bool http_ota_update_1file(const char* host, uint16_t port, const char* path, const char* expectedMD5) {
  EthernetClient client;
  client.setTimeout(15000); // Stream timeout

  PrintDebugf("[OTA] Connecting to %s:%u\n", host, port);
  if (!client.connect(host, port)) {
    PrintDebugLn("[OTA] Connection failed");
    return false;
  }

  // Gửi HTTP GET
  String req = String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n";
  client.print(req);

  int status = 0;
  long contentLength = -1;
  bool isChunked = false;
  if (!httpReadHeaders(client, status, contentLength, isChunked)) {
    PrintDebugLn("[OTA] Failed to read HTTP headers");
    client.stop();
    return false;
  }

  if (status != 200) {
    PrintDebugf("[OTA] HTTP status %d (expect 200)\n", status);
    client.stop();
    return false;
  }

  // Chuẩn bị Update
  if (expectedMD5 && strlen(expectedMD5) == 32) {
    Update.setMD5(expectedMD5);
  }
  bool beginOk = false;
  if (!isChunked && contentLength > 0) {
    beginOk = Update.begin(contentLength);
  } else {
    // không có Content-Length hoặc dạng chunked
    beginOk = Update.begin(UPDATE_SIZE_UNKNOWN);
  }

  if (!beginOk) {
    PrintDebugf("[OTA] Update.begin failed: %s\n", Update.errorString());
    client.stop();
    return false;
  }

  // Stream body -> flash
  size_t written = 0;
  bool ok = false;
  if (isChunked) {
    PrintDebugLn("[OTA] Transfer-Encoding: chunked");
    ok = httpStreamChunkedToUpdate(client, written);
  } else {
    PrintDebugf("[OTA] Content-Length: %ld\n", contentLength);
    if (contentLength > 0) {
      ok = httpStreamLenToUpdate(client, contentLength, written);
    } else {
      // Một số server không báo length: cứ đọc đến hết socket
      const size_t BUFSZ = 1024;
      uint8_t buf[BUFSZ];
      uint32_t lastReport = 0;
      ok = true;
      while (client.connected() || client.available()) {
        int n = client.readBytes(buf, BUFSZ);
        if (n > 0) {
          size_t w = Update.write(buf, n);
          if (w != (size_t)n) { ok = false; break; }
          written += n;
          if (millis() - lastReport > 500) {
            PrintDebugf("[OTA] Written: %u bytes (unknown length)\n", (unsigned)written);
            lastReport = millis();
          }
        } else {
          delay(1);
        }
      }
    }
  }

  client.stop();

  if (!ok) {
    PrintDebugLn("[OTA] Streaming failed");
    Update.abort();
    return false;
  }

  // Kết thúc Update
  if (!Update.end()) {
    PrintDebugf("[OTA] Update.end error: %s\n", Update.errorString());
    return false;
  }

  if (!Update.isFinished()) {
    PrintDebugLn("[OTA] Update not finished");
    return false;
  }

  PrintDebugLn("[OTA] Update successful. Rebooting...");
  delay(200);
  ESP.restart();
  return true; // (thực tế sẽ restart)
}
