/*
 * @Description: Ethernet relay test
 * @Author: LILYGO_L
 * @Date: 2025-02-05 13:49:27
 * @LastEditTime: 2025-02-05 13:49:27
 * @License: GPL 3.0
 */
#include <SPI.h>
#include <Ethernet.h>
#include "pin_config.h"
#include <iostream>
#include <memory>
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
// Web relay trigger flag
bool HTML_Relay1_Flag = 0;

/**
 * @brief W5500 reset pin delay
 * @param resetPin W5500 reset pin
 * @return
 * @Date 2023-07-19 11:31:23
 */
void EthernetReset(const uint8_t resetPin)
{
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(250);
    digitalWrite(resetPin, LOW);
    delay(50);
    digitalWrite(resetPin, HIGH);
    delay(350);
}

void setup()
{
    Serial.begin(115200);

    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, HIGH);

    SPI.begin(W5500_SCLK, W5500_MISO, W5500_MOSI, W5500_CS);
    // SPI.setFrequency(80000000);
    EthernetReset(W5500_RST);
    Ethernet.init(W5500_CS);

    W5100.init();
    delay(1000); // give the Ethernet shield a second to initialize

    switch (Ethernet.hardwareStatus())
    {
    case EthernetNoHardware:
        // no point in carrying on, so do nothing forevermore:
        while (true)
        {
            Serial.println("Ethernet No Hardware");
            delay(1000);
        }
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
        while (true)
        {
            Serial.print("Link status: ");
            Serial.println("Unknown");
            Serial.println("Hardware error !");
            delay(1000);
        }
        break;
    case LinkON:
        Serial.print("Link status: ");
        Serial.println("ON");
        break;
    case LinkOFF:
        Serial.print("Link status: ");
        Serial.println("OFF");
        Serial.println("The network cable is not connected !");

        while (Ethernet.linkStatus() == LinkOFF)
        {
            Serial.println("Please insert the network cable and try again !");
            delay(1000);
        }
        if (Ethernet.linkStatus() == LinkON)
        {
            Serial.print("Link status: ");
            Serial.println("ON");
        }
        else
        {
            // no point in carrying on, so do nothing forevermore:
            while (true)
            {
                Serial.println("Hardware error !");
                delay(1000);
            }
        }
        break;
    }

    // start the Ethernet connection:
    Serial.println("Trying to get an IP address using DHCP...");
    if (Ethernet.begin(mac) == 0)
    {
        // initialize the Ethernet device not using DHCP:
        Ethernet.begin(mac, ip, myDns, gateway, subnet);

        Serial.println("-------------------------");
        Serial.println("[INFO] Configuring random DHCP failed !");
        Serial.println("");
        Serial.println("[INFO] Configuring static IP...");
        Serial.print("[Static] IP Address: ");
        Serial.println(Ethernet.localIP());
        Serial.print("[Static] Subnet Mask: ");
        Serial.println(Ethernet.subnetMask());
        Serial.print("[Static] Gateway: ");
        Serial.println(Ethernet.gatewayIP());
        Serial.print("[Static] DNS: ");
        Serial.println(Ethernet.dnsServerIP());
        Serial.println("-------------------------");
        Serial.println("");
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

    // start listening for clients
    server.begin();

    Serial.end();
}

void loop()
{
    EthernetClient client = server.available();
    if (client)
    {                                  // if you get a client,
        Serial.println("New Client."); // print a message out the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                if (c == '\n')
                { // if the byte is a newline character

                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // the content of the HTTP response follows the header:
                        client.println("</head><body><div align=\"center\"><h2>Relay Ethernet Server Test</h2><br />");
                        client.print("<h3>Click <a href=\"/RELAY_1\">here</a> to change the Relay 1 state.<br></h3>");
                        // The HTTP response ends with another blank line:
                        client.println();
                        // break out of the while loop:
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine:
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }

                // Check to see if the client request was "GET /H" or "GET /L":
                if (currentLine.endsWith("GET /RELAY_1"))
                {
                    HTML_Relay1_Flag = !HTML_Relay1_Flag;
                    switch (HTML_Relay1_Flag)
                    {
                    case 0:
                        digitalWrite(RELAY_1, HIGH);
                        break;
                    case 1:
                        digitalWrite(RELAY_1, LOW);
                        break;

                    default:
                        break;
                    }
                }
            }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
        // give the web browser time to receive the data
        delay(500);
    }
}
