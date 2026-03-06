from flask import Flask, jsonify, send_from_directory
import datetime

app = Flask(__name__)

# Trả JSON lệnh update
@app.route("/cmd")
def cmd():
    return jsonify({
        "Code": 2101,
        "Message": "http://192.168.0.104:80/firmware.bin",
        "Status": 1,
        "DeviceType": 0,
        "CreatedAt": datetime.datetime.utcnow().isoformat() + "Z",
        "IndexInPacket": 0
    })

# Cho phép tải file firmware.bin
@app.route("/mainapp.bin")
def firmware():
    return send_from_directory(".", "mainapp.bin")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=80)
