/*
  RFID Card -> OLED Name Display for ESP8266
  -----------------------------------------
  ESP8266 Pinout for standard NodeMCU / Wemos D1 Mini:

  Wiring - RC522 (SPI):
    RC522 pin   ESP8266 Pin   GPIO Pin
    ---------   -----------   --------
    SDA (SS)    D8            GPIO15
    SCK         D5            GPIO14
    MOSI        D7            GPIO13
    MISO        D6            GPIO12
    GND         GND           GND
    RST         D3            GPIO0
    3.3V        3.3V          3.3V

  Wiring - OLED (I2C):
    OLED pin    ESP8266 Pin   GPIO Pin
    ---------   -----------   --------
    GND         GND           GND
    VCC         3V3 / 5V      3V3 / 5V
    SCL         D1            GPIO5
    SDA         D2            GPIO4
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Pin Definitions for ESP8266 ----------
#define SS_PIN  D8  // GPIO15
#define RST_PIN D3  // GPIO0

MFRC522 rfid(SS_PIN, RST_PIN);

// ---------- OLED (SSD1306) ----------
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS   0x3C   // Change to 0x3D if display fails to init
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Known cards ----------
struct KnownCard {
  byte uid[4];
  const char* name;
};

KnownCard knownCards[] = {
  {{0x23, 0x3D, 0x09, 0x03}, "Zia"},
  {{0x56, 0x8D, 0xF5, 0xA6}, "Danial"},
  {{0x3A, 0x84, 0x14, 0xCD}, "Adam"},
};
const int numKnownCards = sizeof(knownCards) / sizeof(knownCards[0]);

// Function declarations
String matchCard(byte *uid, byte size);
String getUidCompact(byte *uid, byte size);
void showMessage(String header, String detail);

void setup() {
  Serial.begin(115200); // Higher baud rate standard for ESP8266

  // Explicitly assign I2C pins for ESP8266 (D2 = SDA, D1 = SCL)
  Wire.begin(D2, D1);

  SPI.begin();
  rfid.PCD_Init();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED not found - check wiring/address"));
    while (true) {
      yield(); // Prevents hardware watchdog timer resets on ESP8266
    }
  }

  showMessage("READY", "Scan card");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  String name = matchCard(rfid.uid.uidByte, rfid.uid.size);

  if (name != "") {
    showMessage("ACCESS GRANTED", name);
  } else {
    showMessage("UNKNOWN CARD", getUidCompact(rfid.uid.uidByte, rfid.uid.size));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);
}

String matchCard(byte *uid, byte size) {
  for (int i = 0; i < numKnownCards; i++) {
    bool match = true;
    for (byte j = 0; j < 4 && j < size; j++) {
      if (knownCards[i].uid[j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return String(knownCards[i].name);
  }
  return "";
}

String getUidCompact(byte *uid, byte size) {
  String result = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

void showMessage(String header, String detail) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(header);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 25);
  display.println(detail);

  display.display();
}