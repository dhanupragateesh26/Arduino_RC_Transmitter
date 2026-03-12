#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BATTERY_LOW_VOLTAGE 6.8     // Low battery threshold (2S Li-ion/LiPo)
#define BATTERY_BEEP_INTERVAL 3000  // ms between warning beeps

// EEPROM Memory Addresses
#define EEPROM_INIT_FLAG 0      // First time initialization check
#define EEPROM_P1X_FINE 1       // P1X fine trim value
#define EEPROM_P1Y_FINE 2       // P1Y fine trim value
#define EEPROM_P2X_FINE 3       // P2X fine trim value
#define EEPROM_P2Y_FINE 4       // P2Y fine trim value
#define EEPROM_P1X_INV 5        // P1X inversion flag
#define EEPROM_P1Y_INV 6        // P1Y inversion flag
#define EEPROM_P2X_INV 7        // P2X inversion flag
#define EEPROM_P2Y_INV 8        // P2Y inversion flag
#define EEPROM_T1_INV 9         // Toggle 1 inversion flag
#define EEPROM_T2_INV 10        // Toggle 2 inversion flag
#define EEPROM_SOUND 11         // Sound on/off preference

#define EEPROM_INIT_VALUE 55    // Value to indicate EEPROM has been initialized

// ========== NEW: TASK TIMING CONSTANTS ==========
#define RADIO_INTERVAL 10      // 100Hz radio updates (10ms)
#define OLED_INTERVAL 100      // 10Hz OLED updates (100ms)
#define BATTERY_INTERVAL 1000  // 1Hz battery check (1000ms)
#define BUTTON_INTERVAL 20     // 50Hz button debounce (20ms)

unsigned long lastBatteryBeep = 0;
float batteryVoltage = 0.0;

static unsigned long lastOled = 0;

static unsigned long lastRadioTime = 0;
static unsigned long lastOledTime = 0;
static unsigned long lastBatteryTime = 0;
static unsigned long lastButtonTime = 0;

static unsigned long txLastTime = 0;
static int txPacketCount = 0;
static unsigned long lastTime = 0;
static int packetCount = 0;

// Data structure for transmission
struct Transmit {
  byte P1X;
  byte P1Y;
  byte P2X;
  byte P2Y;
  byte pot1;
  byte D1;
  byte D2;
  byte D3;
};

Transmit transmit;

const uint64_t pipeOut = 0xE8E8F0F0E1LL;
RF24 radio(9, 10); // CE, CSN pins

struct AckPayload {
  byte alive;   // 1 = receiver is alive
};

AckPayload ack;


// Pin definitions
int pot_1x_in = A0;
int pot_1y_in = A1;
int pot_2x_in = A2;
int pot_2y_in = A3;
int trim_pot = A6;
int toggle_1_in = 6;
int toggle_2_in = 8;
int trim1_in = 3;
int trim2_in = 4;
int battery_in = A7;
int sound_pin = 7;


// Variables
float battery_level = 0;
int p1x_val = 0;
int p1y_val = 0;
int p2x_val = 0;
int p2y_val = 0;
int trim_pot_val = 0;
int t1_val = 0;
int t2_val = 0;
bool sound = true;

// Fine adjustment variables (127 = center/neutral)
int p1x_fine = 127;
int p1y_fine = 127;
int p2x_fine = 127;
int p2y_fine = 127;

// Inversion flags
bool p1x_inverted = false;
bool p1y_inverted = false;
bool p2x_inverted = false;
bool p2y_inverted = false;
bool t1_inverted = false;
bool t2_inverted = false;

char pot_text[5];

// Button state tracking
bool b1_press = false;
bool b2_press = false;

bool linkOK = false;
unsigned long lastAckTime = 0;
unsigned long lastLinkBeep = 0;

bool displayNeedsUpdate = true;

  
// Function prototypes
void resetData();
void loadFromEEPROM();
void saveToEEPROM();

void setup() {
  Serial.begin(9600);
  
  // Pin setup (must be done before checking reset button)
  pinMode(trim1_in, INPUT);
  pinMode(trim2_in, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sound_pin, OUTPUT);
  pinMode(toggle_1_in, INPUT);
  pinMode(toggle_2_in, INPUT);
  
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Check if both trim buttons are pressed for factory reset
  if (digitalRead(trim1_in) && digitalRead(trim2_in)) {
    display.setCursor(15, 20);
    display.println(F("FACTORY RESET"));
    display.setCursor(20, 35);
    display.println(F("Release buttons"));
    display.display();
    
    // Wait for buttons to be released
    while (digitalRead(trim1_in) || digitalRead(trim2_in)) {
      delay(10);
    }
    
    resetData();
    // saveToEEPROM();
    
    display.clearDisplay();
    display.setCursor(25, 25);
    display.println(F("Reset Complete!"));
    display.display();
    
    if (sound) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(sound_pin, HIGH);
        delay(100);
        digitalWrite(sound_pin, LOW);
        delay(100);
      }
    }
    delay(1500);
  }
  
  // Load settings from EEPROM or initialize if first time
  // loadFromEEPROM();

  // ===== STARTUP SCREEN =====
  display.clearDisplay();
  
  // 1. Draw Header and Border
  display.drawRoundRect(0, 0, 128, 64, 4, SSD1306_WHITE); // Outer frame
  display.setTextSize(2);
  display.setCursor(15, 10);
  display.println(F("SKY-LINK")); // Your Custom Name
  
  display.setTextSize(1);
  display.setCursor(40, 30);
  display.println(F("v2.4 Pro")); // Version number
  
  // 2. Animated Loading Bar
  for(int i = 0; i <= 100; i += 10) {
    display.drawRect(20, 45, 88, 8, SSD1306_WHITE); // Progress bar outline
    display.fillRect(22, 47, (i * 84 / 100), 4, SSD1306_WHITE); // Filling bar
    display.display();
    delay(75); // Small delay for visual effect
  }

  
  // Initialize NRF24L01
  radio.begin();
  radio.setAutoAck(true);
  radio.enableAckPayload();
  // radio.setAutoAck(false); // to disable autoack
  // radio.disableAckPayload();
  radio.setRetries(2, 3);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_1MBPS);
  radio.openWritingPipe(pipeOut);
  radio.stopListening();
  
  // Startup sound - Mario theme
  if (sound) {
    playMarioStartup();
  }
  
  Serial.println(F("Transmitter initialized"));

  lastOledTime = millis();


}

// Reset all data to default values
void resetData() {
  // Set fine trim to center (127)
  p1x_fine = 127;
  p1y_fine = 127;
  p2x_fine = 127;
  p2y_fine = 127;
  
  // Clear all inversions
  p1x_inverted = false;
  p1y_inverted = false;
  p2x_inverted = false;
  p2y_inverted = false;
  t1_inverted = false;
  t2_inverted = false;
  
  // Sound ON by default
  sound = true;
  
  Serial.println(F("Data reset to defaults"));
}

// Load settings from EEPROM
void loadFromEEPROM() {
  // Check if EEPROM has been initialized
  if (EEPROM.read(EEPROM_INIT_FLAG) != EEPROM_INIT_VALUE) {
    Serial.println(F("First time initialization"));
    resetData();
    saveToEEPROM();
  } else {
    // Load fine trim values
    p1x_fine = EEPROM.read(EEPROM_P1X_FINE);
    p1y_fine = EEPROM.read(EEPROM_P1Y_FINE);
    p2x_fine = EEPROM.read(EEPROM_P2X_FINE);
    p2y_fine = EEPROM.read(EEPROM_P2Y_FINE);
    
    // Load inversion flags
    p1x_inverted = EEPROM.read(EEPROM_P1X_INV);
    p1y_inverted = EEPROM.read(EEPROM_P1Y_INV);
    p2x_inverted = EEPROM.read(EEPROM_P2X_INV);
    p2y_inverted = EEPROM.read(EEPROM_P2Y_INV);
    t1_inverted = EEPROM.read(EEPROM_T1_INV);
    t2_inverted = EEPROM.read(EEPROM_T2_INV);
    
    // Load sound preference
    sound = EEPROM.read(EEPROM_SOUND);
    
    Serial.println(F("Settings loaded from EEPROM"));
  }
}

// Save all settings to EEPROM
void saveToEEPROM() {
  // Set initialization flag
  EEPROM.write(EEPROM_INIT_FLAG, EEPROM_INIT_VALUE);
  
  // Save fine trim values
  EEPROM.write(EEPROM_P1X_FINE, p1x_fine);
  EEPROM.write(EEPROM_P1Y_FINE, p1y_fine);
  EEPROM.write(EEPROM_P2X_FINE, p2x_fine);
  EEPROM.write(EEPROM_P2Y_FINE, p2y_fine);
  
  // Save inversion flags
  EEPROM.write(EEPROM_P1X_INV, p1x_inverted);
  EEPROM.write(EEPROM_P1Y_INV, p1y_inverted);
  EEPROM.write(EEPROM_P2X_INV, p2x_inverted);
  EEPROM.write(EEPROM_P2Y_INV, p2y_inverted);
  EEPROM.write(EEPROM_T1_INV, t1_inverted);
  EEPROM.write(EEPROM_T2_INV, t2_inverted);
  
  // Save sound preference
  EEPROM.write(EEPROM_SOUND, sound);
  
  Serial.println(F("Settings saved to EEPROM"));
}

// Map analog values with inversion support
int map_analog(int val, bool invert, int startVal, int endVal) {
  val = constrain(val, 0, 1023);

  int midVal = (startVal + endVal) / 2;

  if (val < midVal) {
    val = map(val, startVal, midVal, 0, 127);
  } else {
    val = map(val, midVal, endVal, 127, 255);
  }

  val = constrain(val, 0, 255);

  return invert ? 255 - val : val;
}



void loop() {
    display.clearDisplay();

  unsigned long currentMillis = millis();

  // display.clearDisplay();
  
  // Read trim potentiometer
  trim_pot_val = analogRead(trim_pot);

  // Handle fine calibration and channel inversion
  fine_cal(trim1_in, b1_press, 1, trim_pot_val, p1x_fine, p1y_fine, p2x_fine, p2y_fine);
  fine_cal(trim2_in, b2_press, -1, trim_pot_val, p1x_fine, p1y_fine, p2x_fine, p2y_fine);
  invert_chn(trim1_in, b1_press, trim_pot_val, p1x_inverted, p1y_inverted, p2x_inverted, p2y_inverted, t1_inverted, t2_inverted);
  
  // Read and process analog inputs with fine trim adjustment
  p1x_val = map_analog(analogRead(pot_1x_in) + 2 * (p1x_fine - 127), p1x_inverted,512,1023);
  p1y_val = map_analog(analogRead(pot_1y_in) + 2 * (p1y_fine - 127), p1y_inverted,0,1023);
  p2x_val = map_analog(analogRead(pot_2x_in) + 2 * (p2x_fine - 127), p2x_inverted,0,1023);
  p2y_val = map_analog(analogRead(pot_2y_in) + 2 * (p2y_fine - 127), p2y_inverted,0,1023);

  // Constrain values
  p1x_val = constrain(p1x_val, 0, 255);
  p1y_val = constrain(p1y_val, 0, 255);
  p2x_val = constrain(p2x_val, 0, 255);
  p2y_val = constrain(p2y_val, 0, 255);

  // Read toggle switches with inversion applied
  t1_val = digitalRead(toggle_1_in);
  t2_val = digitalRead(toggle_2_in);
  
  // Apply toggle inversion
  if (t1_inverted) {
    t1_val = !t1_val;
  }
  if (t2_inverted) {
    t2_val = !t2_val;
  }

  // Read and display battery voltage
  batteryVoltage = analogRead(battery_in) * (5.0 / 1023.0) * 3.0; // Adjust multiplier based on your voltage divider
  
  // Low battery warning
  if (batteryVoltage < BATTERY_LOW_VOLTAGE && sound && (millis() - lastBatteryBeep > BATTERY_BEEP_INTERVAL)) {
    tone(sound_pin, 400, 200);
    delay(220);
    tone(sound_pin, 300, 200);
    noTone(sound_pin);
    lastBatteryBeep = millis();
  }

  // Prepare transmission data
  transmit.P1X = p1x_val;
  transmit.P1Y = p1y_val;
  transmit.P2X = p2x_val;
  transmit.P2Y = p2y_val;
  transmit.pot1 = 0;  // Reserved for future use
  transmit.D1 = t1_val;
  transmit.D2 = t2_val;
  transmit.D3 = 0;    // Reserved for future use

  // Transmit data with acknowledgment


  if (radio.write(&transmit, sizeof(transmit))) {
    // checking packet rate
    packetCount++;

    if (millis() - lastTime >= 1000) {
      // Serial.print("Packets/sec: ");
      // Serial.println(packetCount);
      packetCount = 0;
      lastTime = millis();
    }
// if (millis() - txLastTime >= 10) {          // 10 ms → 100 Hz target
//   bool ok = radio.write(&transmit, sizeof(transmit));

//     packetCount++;

//     if (millis() - lastTime >= 1000) {
//       // Serial.print("Packets/sec: ");
//       // Serial.println(packetCount);
//       packetCount = 0;
//       lastTime = millis();
//       }

//   txLastTime = millis();


  // ACK received
  if (radio.isAckPayloadAvailable()) {
    radio.read(&ack, sizeof(ack));

    if (ack.alive == 1) {
      linkOK = true;
      lastAckTime = millis();
    }
  }

} else {
  // No ACK received (receiver probably off)
}


  // Link lost warning
  if (!linkOK && sound && millis() - lastLinkBeep > 1000) {
    tone(sound_pin, 300, 200);
    delay(220);
    noTone(sound_pin);
    lastLinkBeep = millis();
  }

  if (millis() - lastAckTime > 1000) {   // 1 second timeout
  linkOK = false;
}

  if (currentMillis - lastOledTime >= OLED_INTERVAL) {
    lastOledTime = currentMillis;
    
      updateDisplay(); // New function to handle all display logic

    }

  }



// Button press handler with sound feedback and EEPROM save
int buttonpress(int digpin, bool &press, int i, int &trim_val) {
  if (digitalRead(digpin) && !press) {
    press = true;
    trim_val += i;
    
    // Save to EEPROM immediately when trim value changes
    // saveToEEPROM();
    
    if (sound) {
      digitalWrite(sound_pin, HIGH);
      delay(50);
      digitalWrite(sound_pin, LOW);
    }
    return i;
  }
  else if (!digitalRead(digpin)) {
    press = false;
    return 0;
  }
  return 0;
}

// Button invert handler for toggle switches with EEPROM save
void buttoninvert(int digpin, bool &press, bool &inverted) {
  if (digitalRead(digpin) && !press) {
    press = true;
    inverted = !inverted;
    
    // Save to EEPROM immediately when inversion changes
    // saveToEEPROM();
    
    Serial.print("Inverted: ");
    Serial.println(inverted);
    digitalWrite(LED_BUILTIN, HIGH);
    
    if (sound) {
      digitalWrite(sound_pin, HIGH);
      delay(50);
      digitalWrite(sound_pin, LOW);
    }
  }
  else if (!digitalRead(digpin)) {
    press = false;
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// Fine calibration function
void fine_cal(int digpin, bool &press, int i, int trim_pot_val, int &p1x_fine, int &p1y_fine, int &p2x_fine, int &p2y_fine) {
  // display.setCursor(70, 50);
  
  if (trim_pot_val >= 0 && trim_pot_val < 500) {
    if (trim_pot_val >= 0 && trim_pot_val < 100) {
      // display.print("SND");
        strcpy(pot_text, "SND");

      if (digitalRead(digpin) && !press) {
        press = true;
        sound = !sound;
        
        // Save sound preference to EEPROM
        // EEPROM.write(EEPROM_SOUND, sound);
        
        if (sound) {
          digitalWrite(sound_pin, HIGH);
          delay(50);
          digitalWrite(sound_pin, LOW);
        }
      }
      else if (!digitalRead(digpin)) {
        press = false;
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    else if (trim_pot_val >= 100 && trim_pot_val < 200) {
      buttonpress(digpin, press, i, p1x_fine);
      // display.print("P1X");
  strcpy(pot_text, "P1X");
    }
    else if (trim_pot_val >= 200 && trim_pot_val < 300) {
      buttonpress(digpin, press, i, p1y_fine);
      // display.print("P1Y");
  strcpy(pot_text, "P1Y");

    }
    else if (trim_pot_val >= 300 && trim_pot_val < 400) {
      buttonpress(digpin, press, i, p2x_fine);
      // display.print("P2X");
  strcpy(pot_text, "P2X");

    }
    else if (trim_pot_val >= 400 && trim_pot_val < 500) {
      buttonpress(digpin, press, i, p2y_fine);
      // display.print("P2Y");
  strcpy(pot_text, "P2Y");

    }
  }
}

// Channel inversion function - EXTENDED WITH TOGGLE INVERSION
void invert_chn(int digpin, bool &press, int trim_pot_val, bool &p1xi, bool &p1yi, bool &p2xi, bool &p2yi, bool &t1i, bool &t2i) {
  // display.setCursor(70, 50);
  
  if (trim_pot_val >= 500 && trim_pot_val < 1023) {
    if (trim_pot_val >= 500 && trim_pot_val < 550) {
      buttoninvert(digpin, press, p1xi);
      // display.print("P1X_I");
  strcpy(pot_text, "P1XI");

    }
    else if (trim_pot_val >= 550 && trim_pot_val < 600) {
      buttoninvert(digpin, press, p1yi);
      // display.print("P1Y_I");
  strcpy(pot_text, "P1YI");

    }
    else if (trim_pot_val >= 600 && trim_pot_val < 650) {
      buttoninvert(digpin, press, p2xi);
      // display.print("P2X_I");
  strcpy(pot_text, "P2XI");

    }
    else if (trim_pot_val >= 650 && trim_pot_val < 700) {
      buttoninvert(digpin, press, p2yi);
      // display.print("P2Y_I");
  strcpy(pot_text, "P2YI");

    }
    // Toggle 1 inversion (700-750)
    else if (trim_pot_val >= 700 && trim_pot_val < 750) {
      buttoninvert(digpin, press, t1i);
      // display.print("T1_I");
  strcpy(pot_text, "T1I");

    }
    // Toggle 2 inversion (750-800)
    else if (trim_pot_val >= 750 && trim_pot_val < 800) {
      buttoninvert(digpin, press, t2i);
      // display.print("T2_I");
  strcpy(pot_text, "T2I");

    }
    else {
        strcpy(pot_text, "");

    }
  }
}

// Mario startup melody
void playMarioStartup() {
  // Super Mario Bros intro notes (simplified)
  int melody[] = {
    659, 659, 0, 659,
    0, 523, 659, 0,
    784, 0, 392
  };

  int noteDurations[] = {
    150, 150, 150, 150,
    150, 150, 150, 150,
    300, 150, 300
  };

  for (int i = 0; i < 11; i++) {
    if (melody[i] > 0) {
      tone(sound_pin, melody[i], noteDurations[i]);
    }
    delay(noteDurations[i] * 1.3);
    noTone(sound_pin);
  }
}

void updateDisplay() {
  display.clearDisplay();

  
  // ===== OLED DISPLAY =====
  // Top row: Sound status and battery
  display.setCursor(5, 5);
  display.print("SND:");
  display.print(sound ? "ON" : "OFF");
  
  display.setCursor(65, 5);
  display.print("BAT:");
  display.print(batteryVoltage, 1);
  display.print("V");
  
  // Link status
  display.setCursor(5, 15);
  display.print("LINK:");
  display.print(linkOK ? "OK" : "LOST");

  display.setCursor(65, 15);
  display.print(packetCount);


  // Channel values
  display.setCursor(5, 27);
  display.print("P1X:");
  display.print(p1x_val);
  display.setCursor(65, 27);
  display.print("P1Y:");
  display.print(p1y_val);
  
  display.setCursor(5, 38);
  display.print("P2X:");
  display.print(p2x_val);
  display.setCursor(65, 38);
  display.print("P2Y:");
  display.print(p2y_val);
  
  // Toggle switches
  display.setCursor(5, 50);
  display.print("T1:");
  display.print(t1_val);
  display.setCursor(40, 50);
  display.print("T2:");
  display.print(t2_val);
  
  display.setCursor(70, 50);
  display.print(pot_text);

  display.setCursor(100, 50);
  display.print(trim_pot_val);

  

  // Mode indicator (shows current function from trim pot position)
  // This is displayed by fine_cal() and invert_chn() functions
  
  display.display();
}




