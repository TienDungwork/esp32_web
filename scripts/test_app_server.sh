#!/usr/bin/env bash
# Test kết nối và gửi bảng tin yêu cầu kết nối tới App Server (theo format trong web_server.cpp)
# Server: 192.168.1.11, Port: 35000
#
# --- Lệnh one-liner (copy vào terminal) ---
#
# 1) Test kết nối TCP:
#    nc -zv 192.168.1.11 35000
#
# 2) Bắn 1 bảng tin yêu cầu kết nối (Code=3, kết thúc bằng \\n):
#    python3 -c 'import json,socket; m=json.dumps({"model":"ESP32-S3","firmwareVersion":"dev","firmwareBuild":"","networkMode":"wifi_sta","ip":"192.168.1.100","mac":"AA:BB:CC:DD:EE:FF"}); p=json.dumps({"Code":3,"Message":m,"DeviceType":3})+"\n"; s=socket.socket(); s.settimeout(3); s.connect(("192.168.1.11",35000)); s.sendall(p.encode()); s.close(); print("Sent:", p.strip())'
#

HOST="${1:-192.168.1.11}"
PORT="${2:-35000}"
# Code thiết bị (vd: 3 = Barrier vào, 5 = Đèn giao thông vào)
CODE="${3:-3}"

# --- 1. Test kết nối TCP ---
echo "=== 1. Test kết nối tới $HOST:$PORT ==="
if command -v nc &>/dev/null; then
  if nc -zv "$HOST" "$PORT" 2>&1; then
    echo "OK: Cổng mở, có thể kết nối."
  else
    echo "Lỗi: Không kết nối được (cổng đóng hoặc firewall)."
  fi
else
  (echo >/dev/tcp/"$HOST"/"$PORT") 2>/dev/null && echo "OK: Cổng mở." || echo "Lỗi: Không kết nối được."
fi

# --- 2. Gửi 1 dòng JSON = bảng tin yêu cầu kết nối (giống sendConnectionRequestPacket) ---
# Format: {"Code":<code>,"Message":"<JSON string>","DeviceType":<code>}\n
# Message = JSON string của deviceInfo: model, firmwareVersion, firmwareBuild, networkMode, ip, mac
echo ""
echo "=== 2. Gửi bảng tin yêu cầu kết nối (Code=$CODE) ==="
# Dùng Python để build đúng JSON (escape Message)
PACKET=$(python3 -c "
import json
msg = {'model':'ESP32-S3','firmwareVersion':'dev','firmwareBuild':'','networkMode':'wifi_sta','ip':'192.168.1.100','mac':'AA:BB:CC:DD:EE:FF'}
payload = {'Code': $CODE, 'Message': json.dumps(msg), 'DeviceType': $CODE}
print(json.dumps(payload))
")
echo "Gửi: $PACKET"
( echo "$PACKET"; ) | nc -w 3 "$HOST" "$PORT" && echo "Đã gửi xong." || echo "Gửi thất bại."
