#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ===== PINS =====
#define LDR_PIN 27
#define LED_PIN 2
#define RELAY_PIN 5
#define BUTTON_PIN 13

// ===== MASTER MAC =====
uint8_t masterMAC[] = {0x1C, 0xC3, 0xAB, 0xBA, 0x9D, 0x9C};

// ===== STRUCT =====
typedef struct {
  int nodeId;
  float temp;
  float hum;
  int ldr;
  int relayCmd;
} message_t;

message_t msg;

// ===== CONFIG =====
bool LIGHT_IS_ONE = false;

// ===== STATE =====
bool overrideActive = false;
int overrideState = 0;
unsigned long overrideStartTime = 0;
const unsigned long overrideDuration = 10000;

int lastLdr = -1;
volatile unsigned long lastInterruptTime = 0;

// ===== BUTTON INTERRUPT =====
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();

  if (now - lastInterruptTime > 300) {
    overrideActive = true;
    overrideState = 0;
    overrideStartTime = now;
  }

  lastInterruptTime = now;
}

// ===== RECEIVE FROM MASTER =====
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  message_t data;
  memcpy(&data, incomingData, sizeof(data));

  overrideActive = true;
  overrideState = data.relayCmd;
  overrideStartTime = millis();

  Serial.print("📩 Blynk Command: ");
  Serial.println(overrideState);
}

// ===== LDR READ =====
int readLDR() {
  int count = 0;

  for (int i = 0; i < 5; i++) {
    count += digitalRead(LDR_PIN);
    delay(5);
  }

  return (count >= 3) ? 1 : 0;
}

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);

  // ===== WIFI + CHANNEL =====
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  // ===== ESP-NOW =====
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterMAC, 6);
  peer.channel = 6;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("🚀 NODE 1 READY");
}

void loop() {

  int raw = readLDR();
  bool lightDetected = LIGHT_IS_ONE ? (raw == 1) : (raw == 0);

  // ===== OVERRIDE TIMEOUT =====
  if (overrideActive && millis() - overrideStartTime > overrideDuration) {
    overrideActive = false;
    Serial.println("⏱ Override expired");
  }

  // ===== CONTROL (NO SERIAL SPAM) =====
  if (overrideActive) {

    if (overrideState == 1) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(RELAY_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(RELAY_PIN, LOW);
    }

  } else {

    if (lightDetected) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(RELAY_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  // ===== ONLY ON CHANGE =====
  if (raw != lastLdr) {

    Serial.print("🔔 LDR STATE CHANGED → ");

    if (lightDetected) {
      Serial.println("LIGHT DETECTED (ON)");
    } else {
      Serial.println("DARK (OFF)");
    }

    msg.nodeId = 1;
    msg.ldr = raw;
    msg.temp = 0;
    msg.hum = 0;
    msg.relayCmd = 0;

    esp_now_send(masterMAC, (uint8_t *)&msg, sizeof(msg));
  }

  lastLdr = raw;

  delay(100);
}