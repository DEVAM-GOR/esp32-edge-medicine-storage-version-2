#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ===== PINS =====
#define DHT_PIN 4
#define SERVO_PIN 18
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;

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

// ===== SERVO =====
int angle = 0;
bool forward = true;
unsigned long lastMove = 0;

// ===== CONTROL =====
bool runActive = false;
unsigned long runStart = 0;
const unsigned long runTime = 10000;

// ===== TIMER =====
unsigned long lastSend = 0;

// ===== RECEIVE =====
void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  message_t data;
  memcpy(&data, incomingData, sizeof(data));

  Serial.print("📩 Command: ");
  Serial.println(data.relayCmd);

  if (data.relayCmd == 1) {
    runActive = true;
    runStart = millis();
  } else {
    runActive = false;
    servo.write(0);
  }
}

// ===== SERVO SWEEP =====
void runServo() {
  if (millis() - lastMove > 15) {
    lastMove = millis();

    if (forward) angle++;
    else angle--;

    servo.write(angle);

    if (angle >= 180) forward = false;
    if (angle <= 0) forward = true;
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) return;

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterMAC, 6);
  peer.channel = 6;
  esp_now_add_peer(&peer);

  dht.begin();
  servo.attach(SERVO_PIN);
  servo.write(0);

  Serial.println("🚀 NODE 2 READY");
}

void loop() {

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp)) return;

  // ===== SERVO CONTROL =====
  if (runActive) {

    if (millis() - runStart < runTime) {
      runServo();
    } else {
      runActive = false;
      servo.write(0);
    }

  } else {

    if (temp >= 30) {
      runServo();
    } else {
      servo.write(0);
    }
  }

  // ===== SEND EVERY 5 SEC =====
  if (millis() - lastSend >= 5000) {
    lastSend = millis();

    msg.nodeId = 2;
    msg.temp = temp;
    msg.hum = hum;

    esp_now_send(masterMAC, (uint8_t *)&msg, sizeof(msg));

    Serial.print("📡 Sent: ");
    Serial.print(temp);
    Serial.print(" | ");
    Serial.println(hum);
  }

  delay(20);
}