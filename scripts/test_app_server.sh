#!/usr/bin/env bash
# Test kết nối và gửi bảng tin yêu cầu kết nối tới App Server (theo format trong web_server.cpp)
# Server: 192.168.1.11, Port: 35000
#
# --- Lệnh one-liner (copy vào terminal) ---
#
# 1) Test kết nối TCP:
#    nc -zv 192.168.1.11 35000
#
# 2) Bắn 1 gói API_CONNECT (Code=1) tối giản, kết thúc bằng <EOF> (đúng theo web_server.cpp):
#    python3 -c 'import json,socket; info=json.dumps({"DeviceType":3}); p=json.dumps({"Code":1,"Message":info,"DeviceType":3,"CreatedAt":"0001-01-01T00:00:00","IndexInPacket":0})+"<EOF>"; s=socket.socket(); s.settimeout(3); s.connect(("192.168.1.11",35000)); s.sendall(p.encode()); s.close(); print("Sent:", p)'
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

# --- 2. Gửi 1 gói Code=1 (API_CONNECT) kết thúc bằng <EOF> ---
# Format: {"Code":1,"Message":"<escaped DeviceInfo JSON>","Status":0,"DeviceType":<int>,"CreatedAt":"0001-01-01T00:00:00","IndexInPacket":0}<EOF>
echo ""
echo "=== 2. Gửi gói Code=1 (API_CONNECT) DeviceType=$CODE ==="
# Dùng Python để build đúng JSON (escape Message) + delimiter <EOF>
PACKET=$(python3 -c "
import json
info = {'DeviceType': $CODE}
payload = {'Code': 1, 'Message': json.dumps(info), 'DeviceType': $CODE, 'CreatedAt': '0001-01-01T00:00:00', 'IndexInPacket': 0}
print(json.dumps(payload) + '<EOF>')
")
echo "Gửi: $PACKET"
( echo "$PACKET"; ) | nc -w 3 "$HOST" "$PORT" && echo "Đã gửi xong." || echo "Gửi thất bại."
