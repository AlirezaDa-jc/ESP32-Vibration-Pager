# ESP32 Wireless Vibration Pager System

A wireless master-slave paging system built on **ESP32** microcontrollers with **AES-256 encrypted** RF communication. Designed for industrial notification use cases and assistive technology (non-auditory alerts).

---

## System Overview

```
┌─────────────────────┐          nRF24L01 RF            ┌─────────────────────┐
│     MASTER UNIT     │  ───── channel 90, 1Mbps ───>   │     SLAVE UNIT      │
│                     │       AES-256 ECB encrypted     │                     │
│  4x3 Keypad         │                                 │  WS2812 LED strip   │
│  TM1637 Display     │                                 │  Vibration motor    │
│  nRF24L01 TX        │                                 │  Passive buzzer     │
│  ESP32              │                                 │  nRF24L01 RX        │
│                     │                                 │  ESP32              │
└─────────────────────┘                                 └─────────────────────┘
```

The **Master** accepts a pager ID (1–999) via keypad, encrypts a command, and transmits it wirelessly to the target Slave node. The **Slave** decrypts the payload and triggers haptic, visual, and audio feedback simultaneously.

---

## Features

- **AES-256 ECB encryption** on every RF packet via ESP32's built-in `mbedtls`
- **RF24Network mesh addressing** — supports up to 999 addressable slave nodes
- **Multi-modal feedback** on the slave: vibration motor + WS2812 RGB LEDs + passive buzzer (LEDC PWM tone)
- **Real-time TM1637 display** on master showing pager ID as it's typed; success/error codes on send
- **Custom SPI pin mapping** on slave for flexible PCB layout

---

## Hardware

### Master Unit

| Component | Pin(s) |
|---|---|
| nRF24L01 CE | GPIO 26 |
| nRF24L01 CSN | GPIO 21 |
| nRF24L01 SCK/MOSI/MISO | GPIO 18 / 23 / 19 (default SPI) |
| TM1637 CLK / DIO | GPIO 32 / 33 |
| 4x3 Keypad Rows | GPIO 5, 17, 16, 4 |
| 4x3 Keypad Cols | GPIO 0, 2, 15 |

### Slave Unit

| Component | Pin(s) |
|---|---|
| nRF24L01 CE / CSN | GPIO 14 / 12 |
| nRF24L01 SCK / MOSI / MISO | GPIO 26 / 27 / 25 (custom SPI) |
| nRF24L01 VCC (software-controlled) | GPIO 33 |
| WS2812 LED strip (16 LEDs) | GPIO 13 |
| WS2812 VCC / GND | GPIO 2 / 15 |
| Vibration motor signal / VCC | GPIO 18 / 19 |
| Passive buzzer signal / VCC / GND | GPIO 4 / 16 / 17 |

---

## How It Works

### Master flow
1. User types a pager ID (up to 4 digits) on the keypad — displayed live on TM1637
2. Press `#` → sends command `"0"` | Press `*` → sends command `"1"` to the target node
3. Command is AES-256 encrypted into a 16-byte block and transmitted via RF24Network
4. Display shows `0` on success, `9999` on failure

### Slave flow
1. Continuously polls `RF24Network` for incoming packets
2. On receive: decrypts the 16-byte AES block back to plaintext
3. If command == `"1"`: blinks LEDs red + activates motor + plays 1kHz tone
4. If command == `"0"`: turns off all outputs

### Encryption
Both units share a hardcoded **256-bit AES key** (`aes_key[32]`). Encryption/decryption uses ESP32's native `mbedtls_aes_crypt_ecb`. The `Cipher` helper class (included) provides a higher-level string encryption API if needed for future extensions.

---

## Libraries Required

Install via Arduino Library Manager or PlatformIO:

```
RF24                  — nRF24L01 driver
RF24Network           — mesh network layer on top of RF24
Keypad                — 4x3 matrix keypad
TM1637Display         — 4-digit 7-segment display
Adafruit NeoPixel     — WS2812 LED strip
```

`mbedtls` is built into the ESP32 Arduino core — no extra installation needed.

---

## Project Structure

```
├── master/
│   └── master.ino        # Master unit firmware
|   ├── Cipher.h          # AES-128 helper class (header)
│   └── Cipher.cpp        # AES-128 helper class (implementation)
├── slave/
│   ├── Slave.ino         # Slave unit firmware
|   ├── Cipher.h          # AES-128 helper class (header)
│   └── Cipher.cpp        # AES-128 helper class (implementation)
└── README.md
```

---

## Use Cases

- **Industrial floor paging** — silent, non-intrusive worker notifications
- **Assistive technology** — haptic + visual alerts for individuals with hearing impairments
- **Hospital / clean-room environments** — where audible pagers are disruptive

---

## Notes

- RF channel is set to `90`, data rate `1Mbps`, PA level `MAX` — adjust for your RF environment
- The AES key is hardcoded for simplicity; in production, use a key exchange mechanism
- Slave node address is `01` (octal) — change `this_node` per device for a multi-slave deployment
