/*
   RadioLib SX126x Ping-Pong Example

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include "RadioLib.h"
#include "pin_config.h"

// uncomment the following only on one
// of the nodes to initiate the pings
#define INITIATING_NODE

// SX1276 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1276 radio = new Module(SX1276_CS, SX1276_DIO1, SX1276_RST, SX1276_BUSY, SPI);

// or using RadioShield
// https://github.com/jgromes/RadioShield
// SX1276 radio = RadioShield.ModuleA;

// or using CubeCell
// SX1276 radio = new Module(RADIOLIB_BUILTIN_MODULE);

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    // we sent or received a packet, set the flag
    operationDone = true;
}

void setup()
{
    Serial.begin(115200);

    pinMode(W5500_CS, OUTPUT);
    pinMode(SCREEN_CS, OUTPUT);
    digitalWrite(W5500_CS, HIGH);
    digitalWrite(SCREEN_CS, HIGH);
#ifdef T_Connect_Pro_V1_0_SX1262
    pinMode(SX1262_CS, OUTPUT);
    digitalWrite(SX1262_CS, HIGH);
#elif defined T_Connect_Pro_V1_0_SX1276
    pinMode(SX1276_CS, OUTPUT);
    digitalWrite(SX1276_CS, HIGH);
#else
#error "Unknown macro definition. Please select the correct macro definition."
#endif

    // initialize SX1276 with default settings
    Serial.println("[SX1276] Initializing ... ");
    SPI.begin(SX1276_SCLK, SX1276_MISO, SX1276_MOSI);
    int state = radio.beginFSK();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("success!");
    }
    else
    {
        Serial.print("failed, code ");
        Serial.println(state);
        while (true)
            ;
    }

    radio.setFrequency(945.0);
    radio.setBandwidth(125.0);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(8);
    radio.setSyncWord(0xAB);
    radio.setOutputPower(17);
    radio.setCurrentLimit(240);
    radio.setPreambleLength(16);
    radio.setCRC(false);

    radio.setPacketReceivedAction(setFlag);

    radio.transmitDirect();
}

void loop()
{
}
