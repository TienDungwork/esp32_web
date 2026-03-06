/*
 * @Description: Ethernet scan
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:48:33
 * @LastEditTime: 2025-02-05 13:50:49
 * @License: GPL 3.0
 */

#include <SPI.h>
#include <Ethernet.h>
#include "pin_config.h"
#include "utility/w5100.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
// The MAC address must be an even number first !
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);



void Ethernet_Reset(const uint8_t resetPin)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(250);
    digitalWrite(resetPin, LOW);
    delay(50);
    digitalWrite(resetPin, HIGH);
    delay(350);
}

bool Ethernet_Initialization_Assertion(String *assertion)
{
    switch (Ethernet.hardwareStatus())
    {
    case EthernetNoHardware:
        // no point in carrying on, so do nothing forevermore:

        Serial.println("Ethernet No Hardware");
        *assertion = "Ethernet No Hardware";
        return false;

        break;
    case EthernetW5100:
        Serial.println("Ethernet W5100 Discovery !");
        break;
    case EthernetW5200:
        Serial.println("Ethernet W5200 Discovery !");
        break;
    case EthernetW5500:
        Serial.println("Ethernet W5500 Discovery !");
        break;
    }

    switch (Ethernet.linkStatus())
    {
    case Unknown:
        // no point in carrying on, so do nothing forevermore:

        Serial.print("Link status: ");
        Serial.println("Unknown");
        Serial.println("Hardware error !");

        *assertion = "Hardware error";
        return false;

        break;
    case LinkON:
        Serial.print("Link status: ");
        Serial.println("ON");
        break;
    case LinkOFF:
        Serial.print("Link status: ");
        Serial.println("OFF");
        Serial.println("The network cable is not connected !");

        *assertion = "Please insert the network cable";
        return false;

        break;
    }

    // start the Ethernet connection:
    Serial.println("Trying to get an IP address using DHCP...");
    if (Ethernet.begin(mac, 10000, 10000) == 0)
    {
        // initialize the Ethernet device not using DHCP:
        // Ethernet.begin(mac, ip, myDns, gateway, subnet);

        Serial.println("-------------------------");
        Serial.println("[INFO] Configuring random DHCP failed !");
        Serial.println("");
        // Serial.println("[INFO] Configuring static IP...");
        // Serial.print("[Static] IP Address: ");
        // Serial.println(Ethernet.localIP());
        // Serial.print("[Static] Subnet Mask: ");
        // Serial.println(Ethernet.subnetMask());
        // Serial.print("[Static] Gateway: ");
        // Serial.println(Ethernet.gatewayIP());
        // Serial.print("[Static] DNS: ");
        // Serial.println(Ethernet.dnsServerIP());
        // Serial.println("-------------------------");
        // Serial.println("");

        *assertion = "DHCP configuration failed";
        return false;
    }
    else
    {
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
        Serial.println("");
    }
    return true;
}

void Ethernet_Initialization(void)
{
    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS);
    Ethernet_Reset(W5500_RST);
    Ethernet.init(W5500_CS);

    W5100.init();
}

void setup()
{
    Serial.begin(115200);

    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS); // SPI boots
    Ethernet_Reset(W5500_RST);
    Ethernet.init(W5500_CS);

    W5100.init();
    delay(1000); // give the Ethernet shield a second to initialize
}

void loop()
{
    switch (Ethernet.hardwareStatus())
    {
    case EthernetNoHardware:
        Serial.println("Ethernet No Hardware");
        break;
    case EthernetW5100:
        Serial.println("Ethernet W5100");
        break;
    case EthernetW5200:
        Serial.println("Ethernet W5200");
        break;
    case EthernetW5500:
        Serial.println("Ethernet W5500");
        break;
    }

    switch (Ethernet.linkStatus())
    {
    case Unknown:
        Serial.print("Link status: ");
        Serial.println("Unknown");
        break;
    case LinkON:
        Serial.print("Link status: ");
        Serial.println("ON");
        break;
    case LinkOFF:
        Serial.print("Link status: ");
        Serial.println("OFF");
        break;
    }

    Serial.println("");
    delay(1000);
}

/* void setup()
{
    Serial.begin(115200);
    
    // Khởi tạo Ethernet bằng hàm đã viết
    Ethernet_Initialization();
    
    // Kiểm tra và in cấu hình mạng
    String assertion;
    if (Ethernet_Initialization_Assertion(&assertion))
    {
        // Khởi động server nếu cấu hình thành công
        server.begin();
        Serial.print("Server is at ");
        Serial.println(Ethernet.localIP());
    }
    else
    {
        Serial.println("Ethernet initialization failed: " + assertion);
        // Có thể thêm hành động khi thất bại, ví dụ: dừng chương trình
        while (true); // Vòng lặp vô hạn nếu khởi tạo thất bại
    }
}

void loop()
{
    // Kiểm tra trạng thái phần cứng và liên kết mạng
    String assertion;
    if (!Ethernet_Initialization_Assertion(&assertion))
    {
        Serial.println("Error: " + assertion);
    }
    
    // Kiểm tra client kết nối đến server
    EthernetClient client = server.available();
    if (client)
    {
        Serial.println("New client connected");
        // Xử lý yêu cầu từ client (nếu cần)
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);
                // Đóng kết nối sau khi nhận dữ liệu (tùy thuộc vào yêu cầu)
                if (c == '\n')
                {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println();
                    client.println("Hello, World!");
                    break;
                }
            }
        }
        delay(1); // Đợi client ngắt kết nối
        client.stop();
        Serial.println("Client disconnected");
    }
    
    delay(1000); // Đợi 1 giây trước khi lặp lại
} */