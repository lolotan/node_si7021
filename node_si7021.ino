#include <EEPROM.h>
#include <SPI.h>
#include "RF24.h"

const byte configAddr[] = {'A', 'D', 'D', 'R', '0'}; // Default config address

typedef struct __attribute__((packed))
{
  char nodeAddr[5]; // Node address
  char gwAddr[5];   // Gateway address
} addr_t;

typedef struct __attribute__((packed))
{
  char sensorID[2];  // Sensor ID, in ASCII from 00 to 99
  char hwVersion[2]; // Hardware version, in ASCII from 00 to 99
  char swVersion[2]; // Software version, in ASCII from 00 to 99
} header_t;

typedef struct __attribute__((packed))
{
  header_t  header;       // Frame header
  byte      rfChannel;    // RF CHANNEL, in binary
  byte      rfPower;      // PA, in binary
  addr_t    addresses;    // Node and gateway addresses, in ASCII
} configFrame_t;

typedef struct __attribute__((packed))
{
  header_t  header;       // Frame header
  byte      batteryLvl;   // Battery level, in binary
  byte      xferStats;    // Transfer statistics
  short     sensVal[2];   // Sensor raw values
} sensorFrame_t;

#define HUMIDITY            0
#define TEMPERATURE         1
#define ACT_LED_PIN         9

#define CONFIG_REQUEST_STR  "config_request"
#define CONFIG_SET_STR      "config_set"

#define CONFIG_REQUEST      2
#define CONFIG_SET          1
#define NO_CONFIG           0

sensorFrame_t sensorData;
configFrame_t configData;
char receive_payload[33];

RF24 radio(7, 8);

void setup() {
  // Vars, GPIO and general inits
  memset(&sensorData, 0x00, sizeof(sensorFrame_t));
  memset(&configData, 0x00, sizeof(configFrame_t));
  memset(receive_payload, 0x00, sizeof(receive_payload));
  pinMode(ACT_LED_PIN, OUTPUT);
  Serial.begin(9600);

  // nrf settings
  radio.begin();
  radio.openWritingPipe(configAddr);
  radio.openReadingPipe(0, configAddr);
  radio.setDataRate(RF24_1MBPS) ;
  radio.setChannel(2);
  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(1);
  radio.setRetries(2, 15);
  radio.setCRCLength(RF24_CRC_8);
  radio.enableDynamicPayloads();
  radio.powerUp();
  radio.startListening();

  // Si7021 settings

}

void loop() {
  Serial.println(F("Welcome to Si7021 RF node"));
  // Wait for config frame at config default address for about few seconds
  byte retConf = waitForConfigFrame(40);
  if (retConf == CONFIG_REQUEST) {
    sendConfig(40);
  } else if (retConf == CONFIG_SET) {
    waitConfig(40);
  }
  

  // Or continue with normal program (periodicaly send sensor values)
  for (;;) {
    // Enable activity LED
    // Acquire sensor values
    // Acquire battery level
    // Send sensor frame
    // Disable activity LED
    // Low power sleep for 10 minutes
  }
}

byte waitForConfigFrame(uint8_t retries) {
  for (uint8_t i = 0 ; i < retries ; i++) {
    // Blink LED to inform user
    blinkTimes(1, 100);
    uint8_t len;
    memset(receive_payload, 0x00, sizeof(receive_payload));

    if (radio.available()) {
      len = radio.getDynamicPayloadSize();
      radio.read(receive_payload, len);
      Serial.print(F("Got response size="));
      Serial.print(len);
      
      if (strcmp(receive_payload, CONFIG_REQUEST_STR) == 0) {
        // Config request received
        Serial.println(F("Got config request frame"));
        return CONFIG_REQUEST;
      } else if (strcmp(receive_payload, CONFIG_SET_STR) == 0) {
        // Config set received
        Serial.println(F("Got config set frame"));
        return CONFIG_SET;
      }      
    }
    delay(500);
  }
}

void sendConfig(uint8_t retries) {
  // Prepare config frame with node config data
  configData.header.sensorID[0]  = '0';
  configData.header.sensorID[1]  = '1';
  configData.header.swVersion[0] = '0';
  configData.header.swVersion[1] = '1';
  configData.header.hwVersion[0] = '0';
  configData.header.hwVersion[1] = '1';
 
  configData.rfChannel = radio.getChannel();    // RF CHANNEL, in binary
  configData.rfPower = radio.getPALevel();      // PA, in binary
  
  EEPROM.get(0, configData.addresses.gwAddr);
  EEPROM.get(5, configData.addresses.nodeAddr);
  radio.stopListening();

  for (uint8_t i = 0 ; i < retries ; i++) {
    if (!radio.writeFast(&configData, sizeof(configFrame_t))) {   // Write to the FIFO buffers        
        Serial.println(F("Writefast failed"));
    }

    if(!radio.txStandBy()) {
      Serial.println(F("txStandby failed"));
    }

    delay(500);
  }
    
  radio.startListening();
}

void waitConfig(uint8_t retries) {
  /*if (len == sizeof(configFrame_t)) {
        // config frame received
        memcpy(&configData, receive_payload, len);
        Serial.println(F("Data size match"));
        Serial.print(F("Header sensor ID "));
        Serial.println(configData.header.sensorID);
        Serial.print(F("Header sw ver "));
        Serial.println(configData.header.swVersion);
        Serial.print(F("Header hw ver "));
        Serial.println(configData.header.hwVersion);
        Serial.print(F("GW addr "));
        Serial.println(configData.addresses.gwAddr);
        Serial.print(F("ND addr "));
        Serial.println(configData.addresses.nodeAddr);
      }*/
}

void blinkTimes(uint8_t blinkTimes, int blinkDelay) {
  for (uint8_t i = 0 ; i < blinkTimes ; i++) {
    digitalWrite(ACT_LED_PIN, HIGH);
    delay(blinkDelay);
    digitalWrite(ACT_LED_PIN, LOW);
    delay(blinkDelay);
  }
}



