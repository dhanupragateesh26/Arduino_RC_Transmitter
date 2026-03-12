# NRF24 Arduino RC Transmitter & Receiver

A **custom 2.4GHz RC radio system** built using **Arduino Nano** and **NRF24L01 modules**.

This project implements a **low-latency wireless controller** capable of driving multiple servo channels using joysticks and switches. It includes features typically found in commercial RC transmitters such as trim adjustment, channel inversion, battery monitoring, and failsafe protection.

---

# Features

### Wireless Communication

* NRF24L01 2.4GHz transceiver
* Bidirectional communication using **ACK payload**
* Link status monitoring
* ~100Hz control update rate

### Transmitter

* Dual joystick control (4 analog channels)
* Toggle switches
* Channel trimming
* Channel inversion
* OLED display UI
* Battery voltage monitoring
* Low battery alarm
* Link loss alarm
* EEPROM storage for settings
* Startup animation and sound

### Receiver

* 4 servo outputs
* Automatic failsafe
* Acknowledgment feedback
* Packet rate monitoring
* Reliable RF communication

---

# Hardware Used

## Transmitter

* Arduino Nano
* NRF24L01 module
* SSD1306 OLED display
* 2x joystick modules
* 2x push buttons (trim buttons)
* 2x toggle switches
* trim selector potentiometer
* buzzer
* LiPo battery

## Receiver

* Arduino Nano
* NRF24L01 module
* 4x servo motors

---

# Communication Protocol

The transmitter sends an **8 byte packet**.

| Byte | Description  |
| ---- | ------------ |
| P1X  | Joystick 1 X |
| P1Y  | Joystick 1 Y |
| P2X  | Joystick 2 X |
| P2Y  | Joystick 2 Y |
| pot1 | Reserved     |
| D1   | Toggle 1     |
| D2   | Toggle 2     |
| D3   | Reserved     |

Receiver sends an **ACK payload**:

| Byte     | Description     |
| -------- | --------------- |
| alive    | receiver status |
| failsafe | failsafe state  |

---

# Repository Structure

```
nrf24-arduino-rc
│
├── transmitter
│   ├── tx.ino
│   └── README.md
│
├── receiver
│   ├── rx.ino
│   └── README.md
│
└── README.md
```

---

# Performance

| Parameter   | Value                |
| ----------- | -------------------- |
| Data Rate   | 1 Mbps               |
| Packet Rate | ~100 packets/sec     |
| Latency     | ~10ms                |
| Channels    | 4 analog + 2 digital |

---

# Future Improvements

* Model memory support
* Menu system on OLED
* Telemetry feedback
* RSSI display
* SBUS / PPM output
* RF channel hopping

---

# License

MIT License

---

# Author

Custom RC system built using Arduino and NRF24L01 modules for robotics and RC experimentation.
