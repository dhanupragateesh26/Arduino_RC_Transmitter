# Receiver (RX)

Arduino based **NRF24 receiver** that receives control packets and drives servo motors.

It includes **failsafe protection** and **acknowledgment feedback** to the transmitter.

---

# Features

* 4 servo output channels
* RF link monitoring
* Failsafe protection
* Acknowledgment payload
* Packet rate monitoring

---

# Hardware

| Component    | Description    |
| ------------ | -------------- |
| Arduino Nano | Controller     |
| NRF24L01     | RF receiver    |
| Servo motors | Output devices |

---

# Servo Mapping

| Channel | Servo Pin |
| ------- | --------- |
| P1X     | D5        |
| P1Y     | D6        |
| P2X     | D9        |
| P2Y     | D10       |

Control range:

```
1000 µs  -> minimum
1500 µs  -> center
2000 µs  -> maximum
```

---

# Failsafe System

If no packet is received within **500ms**:

* Servos move to safe positions
* RF link lost warning prints to serial
* Failsafe status is sent in ACK payload

Failsafe positions:

| Servo  | Position |
| ------ | -------- |
| Servo1 | 1000 µs  |
| Servo2 | 1500 µs  |
| Servo3 | 1500 µs  |
| Servo4 | 1500 µs  |

---

# RF Configuration

| Parameter | Value   |
| --------- | ------- |
| Frequency | 2.4GHz  |
| Data Rate | 1Mbps   |
| Auto ACK  | Enabled |
| Retries   | Enabled |

---

# Packet Structure

Receiver expects an **8 byte packet**:

```
P1X
P1Y
P2X
P2Y
pot1
D1
D2
D3
```

Each analog channel ranges:

```
0 → 255
```

Converted to servo pulse:

```
1000µs → 2000µs
```

---

# Upload

1. Connect Arduino Nano
2. Install **RF24 library**
3. Open `rx.ino`
4. Upload to board

---

# Debugging

Open **Serial Monitor (9600 baud)** to see:

```
Packets/sec: 100
RF LINK LOST - FAILSAFE ACTIVE
```

---

# Notes

Use **separate 5V power supply for servos** if driving large loads.

NRF24 module should use **3.3V supply with capacitor (10–100µF)** for stability.
