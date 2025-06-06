#include <Wire.h>
#include "Adafruit_DS3502.h"

// — Wi‑FiS3 (UNO R4 WiFi) —
#include <WiFiS3.h>        // built‑in Wi‑Fi support
#include <WiFiUdp.h>

// — ArduinoOSC over Wi‑Fi —
#include <ArduinoOSCWiFi.h> // provides OscWiFi :contentReference[oaicite:2]{index=2}

// — Your Wi‑Fi credentials —
const char* SSID = "DX-STUDIO";
const char* PWD  = "dxartsword";


// — OSC receive port (must match your Python sender) —
const int OSC_RECV_PORT = 8000;
const int g_brainchip = 9;
const int r_brainchip = 10;
// — DS3502 + sensors —
Adafruit_DS3502 pot;
const int flexPin  = A1;
const int forcePin = A2;

volatile int takeover = 0;
// — OSC callback: toggle LED on /color 1/0 —
void onColorOSC(int &val) {
  digitalWrite(LED_BUILTIN, val == 1 ? HIGH : LOW);
  Serial.print("OSC /color -> ");
  Serial.println(val);
  takeover = val;
}
// global state of the last /color message (1=red, 0=none)


void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // LED indicator
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
 pinMode(r_brainchip, OUTPUT);
  digitalWrite(r_brainchip, LOW);
   pinMode(g_brainchip, OUTPUT);
  digitalWrite(g_brainchip, HIGH);
  // I2C + DS3502 init
  Wire.begin();
  if (!pot.begin()) {
    Serial.println("DS3502 not found! Check wiring.");
    while (1) { delay(10); }
  }
  Serial.println("DS3502 ready.");

  // — Connect to Wi‑Fi —
  Serial.print("Connecting to Wi‑Fi ");
  Serial.println(SSID);
  WiFi.begin(SSID, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP=");
  Serial.println(WiFi.localIP());

  // — Subscribe OSC address “/color” —
  OscWiFi.subscribe(OSC_RECV_PORT, "/color", onColorOSC);
  Serial.print("OSC listening on port ");
  Serial.print(OSC_RECV_PORT);
  Serial.println(", address /color");
}

void loop() {
  // Handle incoming OSC packets
  OscWiFi.update();

  // Read sensors
  int flexRaw  = analogRead(flexPin);
  int forceRaw = analogRead(forcePin);

  // Map values
  int mappedFlex = map(flexRaw, 990, 30, 0, 900);
  int force = map(forceRaw, 1014, 200, 0, 65);

  // Update DS3502 if flex above threshold
  if (mappedFlex > 500 &&  takeover == 0) {
    digitalWrite(g_brainchip, HIGH);
    pot.setWiper(force);
    Serial.println("Flex high -> updating wiper.");
  } else {
    Serial.println("Flex low -> not updating wiper.");
  }
  if (takeover == 1) {
    digitalWrite(r_brainchip, HIGH);
    digitalWrite(g_brainchip, LOW);
    pot.setWiper(65);
    delay(200);
    pot.setWiper(0);
    delay(200);
    pot.setWiper(65);
  } else {
    digitalWrite(r_brainchip, LOW);
    digitalWrite(g_brainchip, HIGH);
  }

  // Debug print
  Serial.print("FlexRaw: ");    Serial.print(flexRaw);
  Serial.print(" MappedFlex: ");Serial.print(mappedFlex);
  Serial.print(" | ForceRaw: ");Serial.print(forceRaw);
  Serial.print(" Force: ");     Serial.print(force);
  Serial.print(" | LED: ");     
  Serial.println(digitalRead(r_brainchip) ? "ON" : "OFF");
  delay(100);
}
