import socket
import json

broadcast_ip = "255.255.255.255"
broadcast_port = 12345
message = {"ip": "192.168.0.105", "port": 35000}

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.sendto(json.dumps(message).encode(), (broadcast_ip, broadcast_port))
print("Sent UDP broadcast:", message)