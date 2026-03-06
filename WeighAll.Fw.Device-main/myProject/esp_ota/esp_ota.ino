/*
  ota_agent.ino
  OTA Agent for ESP32 (Arduino). Uses Ethernet (W5500) to download .bin,
  computes SHA-256, verifies optional signature (ECDSA DER, base64),
  writes to next OTA partition and makes it bootable.
*/

#include <Arduino.h>
#include <Ethernet.h>       // UIPEthernet or Ethernet library for W5x00
#include "utility/w5100.h"
#include <Update.h>
#include <SPI.h>
#include <Preferences.h>
#include <vector>
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "mbedtls/sha256.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"
#include "mbedtls/base64.h"

// --- CONFIG ---
#define W5500_SCLK 12
#define W5500_MISO 13
#define W5500_MOSI 11
#define W5500_CS 10
#define W5500_RST 48
#define W5500_INT 9
#define CONNECT_TIMEOUT_MS 15000
#define READ_TIMEOUT_MS 15000

// Public key (PEM) used to verify firmware signature (ECDSA P-256)
static const char FW_PUBKEY_PEM[] = R"KEY(
-----BEGIN PUBLIC KEY-----
<INSERT YOUR PEM PUBLIC KEY HERE>
-----END PUBLIC KEY-----
)KEY";

// --- Network Configuration ---
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// --- Helpers ---
Preferences prefs;

// Base64 decode helper (mbedtls)
bool base64_decode(const String &b64, std::vector<uint8_t> &out) {
  size_t olen = 0;
  int ret = mbedtls_base64_decode(NULL, 0, &olen, (const unsigned char*)b64.c_str(), b64.length());
  if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && ret != 0) {
    return false;
  }
  out.resize(olen);
  ret = mbedtls_base64_decode(out.data(), out.size(), &olen, (const unsigned char*)b64.c_str(), b64.length());
  if (ret != 0) return false;
  out.resize(olen);
  return true;
}

// hex string -> bytes
bool hexstr_to_bytes(const String &hex, std::vector<uint8_t> &out) {
  if (hex.length() % 2 != 0) return false;
  size_t n = hex.length()/2;
  out.resize(n);
  for (size_t i=0;i<n;i++){
    char h = hex[2*i];
    char l = hex[2*i+1];
    auto val = [](char c)->int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      return -1;
    };
    int hi = val(h), lo = val(l);
    if (hi < 0 || lo < 0) return false;
    out[i] = (uint8_t)((hi<<4) | lo);
  }
  return true;
}

// parse host:port/path
void parse_url(const String &url, String &host, uint16_t &port, String &path) {
  String u = url;
  if (u.startsWith("http://")) u = u.substring(7);
  else if (u.startsWith("https://")) u = u.substring(8); // we still treat as http here (no TLS)
  int slash = u.indexOf('/');
  if (slash >= 0) {
    host = u.substring(0, slash);
    path = u.substring(slash);
  } else {
    host = u;
    path = "/";
  }
  int colon = host.indexOf(':');
  if (colon >= 0) {
    port = host.substring(colon+1).toInt();
    host = host.substring(0, colon);
  } else {
    port = 80;
  }
}

// print mbedtls error
String mbed_err_str(int err) {
  char buf[200];
  mbedtls_strerror(err, buf, sizeof(buf));
  return String(buf);
}

// verify signature (DER ECDSA) of digest using PEM public key
bool verify_signature_ecdsa_der(const uint8_t *digest, size_t digest_len, const std::vector<uint8_t> &sig_der) {
  int ret;
  mbedtls_pk_context pk;
  mbedtls_pk_init(&pk);

  ret = mbedtls_pk_parse_public_key(&pk, (const unsigned char*)FW_PUBKEY_PEM, strlen(FW_PUBKEY_PEM)+1);
  if (ret != 0) {
    Serial.printf("[SIG] mbedtls_pk_parse_public_key failed: %s\n", mbed_err_str(ret).c_str());
    mbedtls_pk_free(&pk);
    return false;
  }

  ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, digest, digest_len, sig_der.data(), sig_der.size());
  if (ret != 0) {
    Serial.printf("[SIG] signature verify failed: %s\n", mbed_err_str(ret).c_str());
    mbedtls_pk_free(&pk);
    return false;
  }

  mbedtls_pk_free(&pk);
  return true;
}

// Compute SHA256 hex string of buffer vector
String sha256_hex_from_vector(const std::vector<uint8_t> &v) {
  uint8_t out[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, v.data(), v.size());
  mbedtls_sha256_finish_ret(&ctx, out);
  mbedtls_sha256_free(&ctx);
  char hex[65];
  for (int i=0;i<32;i++) sprintf(hex+i*2, "%02x", out[i]);
  hex[64] = 0;
  return String(hex);
}

// --- Ethernet Initialization ---
void Ethernet_Reset(const uint8_t resetPin) {
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  delay(250);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  delay(350);
}

bool Ethernet_Initialization_Assertion(String *assertion) {
  switch (Ethernet.hardwareStatus()) {
    case EthernetNoHardware:
      Serial.println("[W5500] Ethernet No Hardware");
      *assertion = "[W5500] Ethernet No Hardware";
      return false;
    case EthernetW5100:
      Serial.println("[W5500] Ethernet W5100 Discovery !");
      break;
    case EthernetW5200:
      Serial.println("[W5500] Ethernet W5200 Discovery !");
      break;
    case EthernetW5500:
      Serial.println("[W5500] Ethernet W5500 Discovery !");
      break;
  }

  switch (Ethernet.linkStatus()) {
    case Unknown:
      Serial.println("[W5500] Link status: Unknown");
      Serial.println("[W5500] Hardware error !");
      *assertion = "[W5500] Hardware error";
      return false;
    case LinkON:
      Serial.println("[W5500] Link status: ON");
      break;
    case LinkOFF:
      Serial.println("[W5500] Link status: OFF");
      Serial.println("[W5500] The network cable is not connected !");
      *assertion = "[W5500] Please insert the network cable";
      return false;
  }

  Serial.println("[W5500] Trying to get an IP address using DHCP...");
  if (Ethernet.begin(mac, 10000, 10000) == 0) {
    *assertion = "DHCP configuration failed";
    return false;
  } else {
    Serial.println("-------------------------");
    Serial.println("[INFO] Configuring random DHCP successfully !");
    Serial.println("");
    Serial.print("[DHCP] IP Address: ");
    Serial.println(Ethernet.localIP());
    Serial.print("[DHCP] Subnet Mask: ");
    Serial.println(Ethernet.subnetMask());
    Serial.print("[DHCP] Gateway: ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("[DHCP] DNS: ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.println("-------------------------");
  }
  return true;
}

void Ethernet_Initialization() {
  SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS);
  Ethernet_Reset(W5500_RST);
  Ethernet.init(W5500_CS);
  W5100.init();
}

bool initEthernetWithRetry(uint8_t retries, uint32_t delayMs, String &err) {
  for (uint8_t i = 0; i < retries; i++) {
    if (Ethernet_Initialization_Assertion(&err)) {
      uint32_t start = millis();
      while (Ethernet.linkStatus() != LinkON && millis() - start < 3000) {
        delay(100);
      }
      if (Ethernet.linkStatus() == LinkON) {
        Serial.println("[W5500] Link OK");
        return true;
      } else {
        Serial.println("[W5500] Link OFF, retry...");
        err = "[W5500] Link OFF after initialization";
      }
    } else {
      Serial.println("[W5500] Init failed, retry...");
    }
    delay(delayMs);
  }
  return false;
}

// --- OTA process ---
bool download_and_flash_http(const String &host, uint16_t port, const String &path,
                             const String &expected_sha256_hex,
                             const String &sig_b64)
{
  Serial.printf("[OTA] connect %s:%u\n", host.c_str(), port);
  EthernetClient client;
  client.setTimeout(READ_TIMEOUT_MS/1000);
  if (!client.connect(host.c_str(), port)) {
    Serial.println("[OTA] connect failed");
    return false;
  }
  // send request
  String req = String("GET ") + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  client.print(req);

  // parse headers
  unsigned long start = millis();
  int status = 0;
  long contentLength = -1;
  bool chunked = false;
  while (millis() - start < READ_TIMEOUT_MS) {
    String line = client.readStringUntil('\n');
    if (line.length()==0) continue;
    line.trim();
    if (status==0) {
      int sp1 = line.indexOf(' ');
      int sp2 = line.indexOf(' ', sp1+1);
      if (sp1 >=0 && sp2>sp1) status = line.substring(sp1+1, sp2).toInt();
    } else {
      if (line.length() == 0) break;
      String lower = line;
      lower.toLowerCase();
      if (lower.startsWith("content-length:")) {
        contentLength = line.substring(15).toInt();
      } else if (lower.startsWith("transfer-encoding:")) {
        if (lower.indexOf("chunked") >= 0) chunked = true;
      }
    }
    if (!client.connected()) break;
    if (client.available()==0) delay(1);
  }

  if (status != 200) {
    Serial.printf("[OTA] HTTP status %d\n", status);
    client.stop();
    return false;
  }

  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  if (!update_partition) {
    Serial.println("[OTA] no update partition");
    client.stop();
    return false;
  }
  Serial.printf("[OTA] writing to partition addr 0x%X\n", update_partition->address);

  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts_ret(&sha_ctx, 0);

  bool begin_ok;
  if (contentLength > 0) {
    begin_ok = Update.begin(contentLength, U_FLASH);
  } else {
    begin_ok = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
  }
  if (!begin_ok) {
    Serial.printf("[OTA] Update.begin failed: %s\n", Update.errorString());
    mbedtls_sha256_free(&sha_ctx);
    client.stop();
    return false;
  }

  const size_t BUFSZ = 1024;
  uint8_t buf[BUFSZ];
  size_t total = 0;
  unsigned long lastReport = millis();
  if (!chunked) {
    while (total < (size_t)contentLength) {
      int toRead = ((contentLength - total) > BUFSZ) ? BUFSZ : (contentLength - total);
      int r = client.readBytes((char*)buf, toRead);
      if (r <= 0) {
        if (!client.connected()) break;
        delay(1);
        continue;
      }
      size_t written = Update.write(buf, r);
      if (written != (size_t)r) {
        Serial.printf("[OTA] Update.write failed, wrote %u of %d\n", (unsigned)written, r);
        Update.abort();
        mbedtls_sha256_free(&sha_ctx);
        client.stop();
        return false;
      }
      mbedtls_sha256_update_ret(&sha_ctx, buf, r);
      total += r;
      if (millis() - lastReport > 1000) {
        Serial.printf("[OTA] downloaded %u / %ld\n", (unsigned)total, contentLength);
        lastReport = millis();
      }
    }
  } else {
    while (true) {
      String line = client.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      long chunkSize = strtol(line.c_str(), NULL, 16);
      if (chunkSize <= 0) break;
      size_t remaining = chunkSize;
      while (remaining > 0) {
        int toRead = (remaining > BUFSZ) ? BUFSZ : remaining;
        int r = client.readBytes((char*)buf, toRead);
        if (r <= 0) { delay(1); continue; }
        size_t written = Update.write(buf, r);
        if (written != (size_t)r) {
          Serial.printf("[OTA] Update.write failed (chunked)\n");
          Update.abort();
          mbedtls_sha256_free(&sha_ctx);
          client.stop();
          return false;
        }
        mbedtls_sha256_update_ret(&sha_ctx, buf, r);
        remaining -= r;
        total += r;
      }
      client.read(); client.read();
    }
  }

  client.stop();

  unsigned char digest[32];
  mbedtls_sha256_finish_ret(&sha_ctx, digest);
  mbedtls_sha256_free(&sha_ctx);

  char digest_hex[65];
  for (int i=0;i<32;i++) sprintf(digest_hex + i*2, "%02x", digest[i]);
  digest_hex[64] = 0;
  Serial.printf("[OTA] SHA256: %s\n", digest_hex);

  if (expected_sha256_hex.length() > 0) {
    String expected = expected_sha256_hex;
    expected.toLowerCase();
    if (expected != String(digest_hex)) {
      Serial.println("[OTA] SHA256 mismatch -> abort");
      Update.abort();
      return false;
    }
    Serial.println("[OTA] SHA256 matches expected");
  }

  if (sig_b64.length() > 0) {
    std::vector<uint8_t> sig_der;
    if (!base64_decode(sig_b64, sig_der)) {
      Serial.println("[OTA] base64 decode signature failed");
      Update.abort();
      return false;
    }
    if (!verify_signature_ecdsa_der(digest, sizeof(digest), sig_der)) {
      Serial.println("[OTA] signature verify FAILED");
      Update.abort();
      return false;
    }
    Serial.println("[OTA] signature verify OK");
  } else {
    Serial.println("[OTA] no signature provided, skipping verify");
  }

  if (!Update.end()) {
    Serial.printf("[OTA] Update.end failed: %s\n", Update.errorString());
    return false;
  }
  if (!Update.isFinished()) {
    Serial.println("[OTA] Update not finished?");
    return false;
  }
  Serial.println("[OTA] Update image written OK");

  const esp_partition_t* new_part = esp_ota_get_next_update_partition(NULL);
  if (!new_part) {
    Serial.println("[OTA] cannot find new partition to set boot");
    return false;
  }
  esp_err_t err = esp_ota_set_boot_partition(new_part);
  if (err != ESP_OK) {
    Serial.printf("[OTA] esp_ota_set_boot_partition failed: 0x%X\n", err);
    return false;
  }

  prefs.begin("ota_req", false);
  prefs.putBool("ota_request", false);
  prefs.end();

  Serial.println("[OTA] successful -> restarting to new firmware");
  delay(200);
  ESP.restart();
  return true;
}

void printPartitions() {
  Serial.println("=== Partition Table ===");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP,
                                                   ESP_PARTITION_SUBTYPE_ANY,
                                                   NULL);
  while (it != NULL) {
    const esp_partition_t *part = esp_partition_get(it);
    Serial.printf("Name:%s Type:%d SubType:%d Addr:0x%06X Size:%dKB\n",
                  part->label, part->type, part->subtype,
                  part->address, part->size/1024);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);

  const esp_partition_t *running = esp_ota_get_running_partition();
  Serial.printf("Currently running from: %s\n", running->label);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("=== OTA AGENT START v1.0 ===");
  printPartitions();

  // init preferences, mở ở chế độ ghi để có thể clear cờ
  prefs.begin("ota_req", false);
  bool request = prefs.getBool("ota_request", false);
  String url = prefs.getString("url", "");
  String sha256_hex = prefs.getString("sha256", "");
  String sig_b64 = prefs.getString("sig_b64", "");

  Serial.printf("[AGENT] ota_request=%d url=%s\n", request ? 1 : 0, url.c_str());

  // clear cờ ngay để tránh lặp khi reboot
  prefs.putBool("ota_request", false);
  prefs.end();

  if (!request || url.length() == 0) {
    Serial.println("[AGENT] no OTA request -> fall back to normal boot");

    // lấy phân vùng app0 và set boot về đó trước khi restart
    const esp_partition_t *app0_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_0,
        NULL);

    if (app0_partition) {
      esp_err_t err = esp_ota_set_boot_partition(app0_partition);
      if (err == ESP_OK) {
        Serial.println("Switch to app0 and reboot...");
      } else {
        Serial.printf("Set boot partition failed: %s\n", esp_err_to_name(err));
      }
    } else {
      Serial.println("app0 partition not found!");
    }

    delay(200);
    ESP.restart();
    return;
  }


  // init Ethernet
  String ethErr;
  Ethernet_Initialization(); // Initialize SPI and reset Ethernet
  if (!initEthernetWithRetry(5, 1000, ethErr)) {
    Serial.println("Ethernet init failed after retries: " + ethErr);

    // Lấy phân vùng app0
    const esp_partition_t *app0_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_0,
        NULL);

    if (app0_partition) {
      esp_err_t err = esp_ota_set_boot_partition(app0_partition);
      if (err == ESP_OK) {
        Serial.println("Switch to app0 and reboot...");
        ESP.restart();
      } else {
        Serial.printf("Set boot partition failed: %s\n", esp_err_to_name(err));
      }
    } else {
      Serial.println("app0 partition not found!");
    }
    return;
  }

  // parse url
  String host, path;
  uint16_t port;
  parse_url(url, host, port, path);

  // run download + flash + verify
  bool ok = download_and_flash_http(host, port, path, sha256_hex, sig_b64);
  if (!ok) {
    Serial.println("[AGENT] OTA failed -> reboot to previous app");
    prefs.begin("ota_req", false);
    prefs.putBool("ota_request", false);
    prefs.end();
    delay(1000);
    ESP.restart();
  }
}

void loop() {
  delay(1000);
}