#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "mbedtls/aes.h"

#define AES_KEY_LENGTH 32 // 256-bit key
#define AES_BLOCK_SIZE 16 // AES block size
unsigned char aes_key[AES_KEY_LENGTH] = {0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
                                         0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// nRF24L01 CE and CSN pins
#define CE_PIN 14
#define CSN_PIN 12

// SPI pins
#define SCK_PIN 26
#define MOSI_PIN 27 
#define MISO_PIN 25
#define RADIO_VCC 33

// WS2812 LED pin
#define LED_PIN 13
#define NUM_LEDS 16 // Number of LEDs in the WS2812 strip
#define BRIGHTNESS 50 // Brightness level (0-255)

// Motor Pin
#define MOTOR_PIN 18
#define MOTOR_PIN_VCC 19

#define LED_VCC 2
#define LED_GND 15


#define BEEP_IO 4
#define BEEP_VCC 16
#define BEEP_GND 17

RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

String angleValue;

const uint16_t this_node = 01; // Address of this node in Octal format ( 04,031, etc)

#include "driver/ledc.h"

void setupLEDC() {
  Serial.println("Setting up LEDC Timer...");
  
  ledc_timer_config_t ledc_timer = {
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_10_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 5000
  };

  if (ledc_timer_config(&ledc_timer) != ESP_OK) {
    Serial.println("Failed to configure LEDC Timer!");
  } else {
    Serial.println("LEDC Timer configured.");
  }

  Serial.println("Setting up LEDC Channel...");
  
  ledc_channel_config_t ledc_channel = {
      .gpio_num = BEEP_IO,
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0
  };

  if (ledc_channel_config(&ledc_channel) != ESP_OK) {
    Serial.println("Failed to configure LEDC Channel!");
  } else {
    Serial.println("LEDC Channel configured.");
  }
}


String decryptDataRaw(const unsigned char* encrypted) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, aes_key, AES_KEY_LENGTH * 8);

  unsigned char output[AES_BLOCK_SIZE] = {0};
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encrypted, output);
  mbedtls_aes_free(&aes);

  return String((char*)output); // Convert binary to String
}


void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT); // Initialize motor pin as an output
  pinMode(MOTOR_PIN_VCC, OUTPUT); // Initialize motor pin as an output
  pinMode(BEEP_VCC, OUTPUT); // Initialize motor pin as an output
  pinMode(BEEP_GND, OUTPUT); // Initialize motor pin as an output
  pinMode(LED_VCC, OUTPUT); // Initialize motor pin as an output
  pinMode(LED_GND, OUTPUT); // Initialize motor pin as an output
  digitalWrite(MOTOR_PIN, LOW); // Ensure the motor is off initially
  digitalWrite(MOTOR_PIN_VCC, LOW); // Ensure the motor is off initially
  digitalWrite(BEEP_VCC, HIGH); // Ensure the motor is off initially
  digitalWrite(BEEP_GND, LOW); // Ensure the motor is off initially
  pinMode(BEEP_IO, OUTPUT);
  digitalWrite(LED_VCC, HIGH); // Ensure the VCC LED 
  digitalWrite(LED_GND, LOW); // Ensure the GND off 
  pinMode(RADIO_VCC, OUTPUT); // Initialize radio pin as an output
  digitalWrite(RADIO_VCC, HIGH); // Ensure the radio is on initially
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CSN_PIN); // Initialize SPI with custom pins
  if (!radio.begin()) {
    Serial.println("Radio hardware is not responding!");
    while (1) {} // Loop forever if the radio is not responding
  }
  network.begin(90, this_node); // (channel, node address)
  radio.setDataRate(RF24_1MBPS); // RF24_250KBPS, RF24_1MBPS, RF24_2MBPS
  radio.setPALevel(RF24_PA_MAX); // RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm
  strip.begin();
  strip.setBrightness(BRIGHTNESS); // Set brightness level
  strip.show(); // Initialize all pixels to 'off'
  setupLEDC(); // Initialize LEDC
}

void loop() {
  network.update();
  //===== Receiving =====//
  if (network.available()) {
    RF24NetworkHeader header;
    unsigned char receivedData[AES_BLOCK_SIZE] = {0};
    network.read(header, &receivedData, sizeof(receivedData));
    String decryptedData = decryptDataRaw(receivedData);
    Serial.println("Decrypted data: " + decryptedData);
    angleValue = decryptedData; // Use decrypted data
    Serial.println("Received data: " + angleValue);
  }
  if (angleValue == "1") {
    // Blink the WS2812 LEDs red when data is "1"
    tone(BEEP_IO, 1000);
    blinkStripColor(255, 0, 0); // Red color
    digitalWrite(MOTOR_PIN, HIGH); // Turn on the motor
    digitalWrite(MOTOR_PIN_VCC, HIGH); // Turn on the motor
    noTone(BEEP_IO);     // Stop sound...
  } else {
    // Turn off the LEDs
    setStripColor(0, 0, 0); // Off
    digitalWrite(MOTOR_PIN, LOW); // Turn off the motor
    digitalWrite(MOTOR_PIN_VCC, LOW); // Turn on the motor
    noTone(BEEP_IO);     // Stop sound...
  }
  delay(100); // Delay to reduce serial output frequency
}

void setStripColor(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(red, green, blue));
  }
  strip.show();
}

void blinkStripColor(uint8_t red, uint8_t green, uint8_t blue) {
  setStripColor(red, green, blue); // Turn on LEDs with the specified color
  delay(100); // Keep the LEDs on for 100ms
  setStripColor(0, 0, 0); // Turn off LEDs
  delay(100); // Keep the LEDs off for 100ms
}
