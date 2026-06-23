#include <Wire.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Keypad.h>
#include <TM1637Display.h>
#include "mbedtls/aes.h"

//AES Config
#define AES_KEY_LENGTH 32 // 256-bit key
#define AES_BLOCK_SIZE 16 // AES block size
unsigned char aes_key[AES_KEY_LENGTH] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
                                         0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// Define the CLK and DIO connections for TM1637
#define CLK 32
#define DIO 33

// Create an instance of the TM1637Display class
TM1637Display display(CLK, DIO);

//Keypad Config
const byte ROWS = 4; // Four rows
const byte COLS = 3; // Three columns

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {5, 17, 16, 4};
byte colPins[COLS] = {0, 2, 15};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

//nRF24L01 Config
// nRF24L01 Pins Connection Helper
// VCC	3.3V
// GND	GND
// CE	GPIO 26
// CSN	GPIO 21
// SCK	GPIO 18
// MOSI	GPIO 23
// MISO	GPIO 19
// CE and CSN pins for the Master node
#define CE_PIN 26
#define CSN_PIN 21

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio); // Include the radio in the network

const uint16_t this_node = 00; // Address of this node in Octal format ( 04,031, etc)

//Global Strings
String pagerID = "";
String command = "";

// Encrypt command as raw binary
void encryptDataRaw(const String& data, unsigned char* encrypted) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aes_key, AES_KEY_LENGTH * 8);

  unsigned char input[AES_BLOCK_SIZE] = {0};
  strncpy((char*)input, data.c_str(), AES_BLOCK_SIZE); // Copy data to input buffer
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, encrypted);
  mbedtls_aes_free(&aes);
}

// Node Address from string
uint16_t getNodeAddress(String pagerID) {
  int id = pagerID.toInt(); // Convert pagerID to integer
  if (id >= 1 && id <= 999) {
    return id;
  }
  return 0; // Default or unknown node
}

//if needed print the encrypted data
void printEncryptedData(const unsigned char* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 0x10) Serial.print("0"); // Add leading zero for single-digit hex values
    Serial.print(data[i], HEX); // Print each byte in hexadecimal
    Serial.print(" "); // Separate bytes with a space for readability
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  if (!radio.begin()) {
    Serial.println("Radio hardware is not responding!");
    while (1) {} // Loop forever if the radio is not responding
  }
  network.begin(90, this_node); // (channel, node address)
  radio.setDataRate(RF24_1MBPS); // RF24_250KBPS, RF24_1MBPS, RF24_2MBPS
  radio.setPALevel(RF24_PA_MAX); // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm
  radio.stopListening();//To make sure it is not receiving data

  // Initialize the TM1637 display
  display.setBrightness(0x0f); // Set the brightness (0x00 to 0x0f)
  display.showNumberDec(0); // Display 0 initially
}

void loop() {
  network.update();
  char key = keypad.getKey();

  if (key) {
    // Check if * or # is pressed to send the command
    if (key == '#' || key == '*') {
      if (pagerID != "") {
        command = key == '#' ? "0" : "1";
        uint16_t nodeAddress = getNodeAddress(pagerID);
        if (nodeAddress != 0) {
          RF24NetworkHeader header(nodeAddress); // Address where the data is going
          unsigned char encryptedCommand[AES_BLOCK_SIZE] = {0};
          encryptDataRaw(command, encryptedCommand);
          bool ok = network.write(header, encryptedCommand, AES_BLOCK_SIZE); // Send encrypted binary data
          if (ok) {
            Serial.println("Data sent successfully to node " + String(nodeAddress) + ": " + command);
            display.showNumberDec(0); // Example success code
          } else {
            Serial.println("Data sending failed");
            display.showNumberDec(9999); // Example error code
          }
        } else {
          Serial.println("Invalid pagerID: " + pagerID);
          display.showNumberDec(0); // Example invalid ID code
        }
        delay(1000); // Send data every second
        pagerID = "";
      }
    } else {
      // Handle other key presses
      if (pagerID.length() < 4) {
        pagerID += key;
        Serial.println(pagerID);
        display.showNumberDec(pagerID.toInt()); // Display the current pagerID
      }
    }
  }
}