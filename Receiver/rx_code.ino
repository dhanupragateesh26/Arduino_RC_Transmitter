#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

// ================= NRF24 =================
RF24 radio(3,2); // CE, CSN
const uint64_t pipeIn = 0xE8E8F0F0E1LL;

// ================= SERVOS =================
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// ================= PAYLOAD =================
struct Payload {
  byte P1X;
  byte P1Y;
  byte P2X;
  byte P2Y;
  byte pot1;
  byte D1;
  byte D2;
  byte D3;
};

Payload rxData;

// ================= ACK PAYLOAD =================
struct AckPayload {
  byte alive;
  byte failsafe;
};

AckPayload ackData;

// ================= FAILSAFE =================
unsigned long lastRecvTime = 0;
const unsigned long FAILSAFE_TIMEOUT = 500; // ms

static unsigned long lastTime = 0;
static int packetCount = 0;

// ================= SETUP =================
void setup() {
  Serial.begin(9600);

  // Attach servos
  servo1.attach(5);
  servo2.attach(6);
  servo3.attach(9);
  servo4.attach(10);

  // Neutral position (1500 µs)
  servo1.writeMicroseconds(1000);
  servo2.writeMicroseconds(1500);
  servo3.writeMicroseconds(1500);
  servo4.writeMicroseconds(1500);

  // NRF24 init
  radio.begin();
  radio.setAutoAck(true);
  radio.enableAckPayload();
  // radio.setAutoAck(false);
  // radio.disableAckPayload();
  radio.setRetries(2, 3);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.openReadingPipe(0, pipeIn);
  radio.startListening();

  Serial.println(F("✅ RC Receiver Initialized"));
}

// ================= LOOP =================
void loop() {
  if (radio.available()) {
    radio.read(&rxData, sizeof(rxData));
    lastRecvTime = millis();

    // Prepare ACK payload
    ackData.alive = 1;
    ackData.failsafe = 0;
    radio.writeAckPayload(0, &ackData, sizeof(ackData));

    // checking packet rate
    static unsigned long lastTime = 0;
    static int packetCount = 0;

    packetCount++;

    if (millis() - lastTime >= 1000) {
      Serial.print("Packets/sec: ");
      Serial.println(packetCount);
      packetCount = 0;
      lastTime = millis();
    }

    // Map joystick to servos (1000–2000 µs)
    servo1.writeMicroseconds(map(rxData.P1X, 0, 255, 1000, 2000));
    servo2.writeMicroseconds(map(rxData.P1Y, 0, 255, 1000, 2000));
    servo3.writeMicroseconds(map(rxData.P2X, 0, 255, 1000, 2000));
    servo4.writeMicroseconds(map(rxData.P2Y, 0, 255, 1000, 2000));

        // ===== SERIAL MONITOR =====
    // Serial.print("P1X: "); Serial.print(rxData.P1X);
    // Serial.print(" | P1Y: "); Serial.print(rxData.P1Y);
    // Serial.print(" | P2X: "); Serial.print(rxData.P2X);
    // Serial.print(" | P2Y: "); Serial.print(rxData.P2Y);
    // Serial.print(" | POT1: "); Serial.print(rxData.pot1);
    // Serial.print(" | D1: "); Serial.print(rxData.D1);
    // Serial.print(" | D2: "); Serial.print(rxData.D2);
    // Serial.print(" | D3: "); Serial.println(rxData.D3);
  }

  // ===== FAILSAFE =====
  if (millis() - lastRecvTime > FAILSAFE_TIMEOUT) {
    servo1.writeMicroseconds(1000);
    servo2.writeMicroseconds(1500);
    servo3.writeMicroseconds(1500);
    servo4.writeMicroseconds(1500);

    ackData.alive = 0;
    ackData.failsafe = 1;

    Serial.println("RF LINK LOST - FAILSAFE ACTIVE");
    delay(200);
  }
}
