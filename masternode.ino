// ===== BLYNK CONFIG =====
#define BLYNK_TEMPLATE_ID "TMPL3zKJcoqE0"
#define BLYNK_TEMPLATE_NAME "Medicine Storage"
#define BLYNK_AUTH_TOKEN "wdI86px0W3y3YjINpUItQAnqoAHszaBK"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <BlynkSimpleEsp32.h>

// ===== WIFI =====
char ssid[] = "Bluetooth 1";
char pass[] = "87654321";

// ===== STRUCT =====
typedef struct {
  int nodeId;
  float temp;
  float hum;
  int ldr;
  int relayCmd;
} message_t;

message_t incomingData;

// ===== MAC ADDRESSES =====
uint8_t node1MAC[] = {0xF4, 0x2D, 0xC9, 0x86, 0xA7, 0x58}; // LDR Node
uint8_t node2MAC[] = {0x1C, 0xC3, 0xAB, 0xBB, 0xC1, 0x70}; // DHT Node

int wifiChannel;

// ===== RECEIVE =====
void onReceive(const uint8_t * mac, const uint8_t *incomingDataPtr, int len) {

  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  if (incomingData.nodeId == 2) {
    Serial.print("🌡 Temp: ");
    Serial.println(incomingData.temp);

    Serial.print("💧 Hum: ");
    Serial.println(incomingData.hum);

    Blynk.virtualWrite(V0, incomingData.temp);
    Blynk.virtualWrite(V1, incomingData.hum);
  }

  if (incomingData.nodeId == 1) {
    Serial.print("💡 LDR: ");
    Serial.println(incomingData.ldr);

    Blynk.virtualWrite(V2, incomingData.ldr);
  }
}

// ===== SEND COMMAND =====
void sendCommand(uint8_t *mac, int cmd) {

  message_t msg;
  msg.nodeId = 0;
  msg.relayCmd = cmd;

  Serial.print("📤 Sending: ");
  Serial.println(cmd);

  esp_now_send(mac, (uint8_t *)&msg, sizeof(msg));
}

// ===== BLYNK =====
BLYNK_WRITE(V3) {
  sendCommand(node1MAC, param.asInt());
}

BLYNK_WRITE(V4) {
  sendCommand(node2MAC, param.asInt());
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected");

  wifiChannel = WiFi.channel();
  esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer1 = {};
  memcpy(peer1.peer_addr, node1MAC, 6);
  peer1.channel = wifiChannel;
  esp_now_add_peer(&peer1);

  esp_now_peer_info_t peer2 = {};
  memcpy(peer2.peer_addr, node2MAC, 6);
  peer2.channel = wifiChannel;
  esp_now_add_peer(&peer2);

  Blynk.config(BLYNK_AUTH_TOKEN);

  Serial.println("🚀 MASTER READY");
}

// ===== LOOP =====
void loop() {
  Blynk.run();
}