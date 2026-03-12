# Transmitter (TX)

Arduino based **RF transmitter** for the custom NRF24 RC system.

This device reads joystick inputs, switch states, and trim settings then transmits control packets to the receiver using an **NRF24L01 module**.

---

# Features

* 4 analog control channels
* 2 digital switches
* OLED display interface
* Channel trim adjustment
* Channel inversion
* EEPROM stored settings
* Battery monitoring
* Link status indicator
* Low battery alarm
* RF link lost alarm
* Startup animation

---

# Hardware

| Component       | Description      |
| --------------- | ---------------- |
| Arduino Nano    | Main controller  |
| NRF24L01        | RF module        |
| SSD1306 OLED    | Display          |
| Joysticks       | Analog input     |
| Toggle switches | Digital channels |
| Trim buttons    | Calibration      |
| Buzzer          | Audio feedback   |
| LiPo battery    | Power source     |

---

# Control Layout

### Analog Channels

| Channel | Input        |
| ------- | ------------ |
| P1X     | Joystick 1 X |
| P1Y     | Joystick 1 Y |
| P2X     | Joystick 2 X |
| P2Y     | Joystick 2 Y |

### Digital Channels

| Channel | Input           |
| ------- | --------------- |
| D1      | Toggle switch 1 |
| D2      | Toggle switch 2 |

---

# Trim System

Trim adjustments allow fine calibration of joystick centers.

The **trim selector potentiometer** selects which parameter to adjust.

Example:

| Pot Range | Function     |
| --------- | ------------ |
| 0-100     | Sound toggle |
| 100-200   | P1X trim     |
| 200-300   | P1Y trim     |
| 300-400   | P2X trim     |
| 400-500   | P2Y trim     |

---

# Channel Inversion

Channels can be inverted using the trim selector.

Example:

| Range   | Function        |
| ------- | --------------- |
| 500-550 | P1X invert      |
| 550-600 | P1Y invert      |
| 600-650 | P2X invert      |
| 650-700 | P2Y invert      |
| 700-750 | Toggle 1 invert |
| 750-800 | Toggle 2 invert |

---

# OLED Display

Displays:

* Battery voltage
* Link status
* Packet rate
* Channel values
* Toggle states
* Current trim selection

---

# RF Configuration

| Setting   | Value   |
| --------- | ------- |
| Frequency | 2.4GHz  |
| Data Rate | 1Mbps   |
| Auto ACK  | Enabled |
| Retries   | Enabled |

---

# Upload

1. Connect Arduino Nano
2. Install libraries:

   * RF24
   * Adafruit GFX
   * Adafruit SSD1306
3. Open `tx.ino`
4. Upload to board

---

# Notes

Power NRF24 module using a **3.3V regulator with decoupling capacitor** for stable operation.
