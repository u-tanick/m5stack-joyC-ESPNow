#include "JoyC.h"
#include "M5Unified.h"
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

struct PeerInfo {
  const char* name;
  uint8_t mac[6];
};

PeerInfo peers[] = {
  {"Buggy", {0x90, 0x15, 0x06, 0xFA, 0x05, 0xE8}},
  {"Other", {0x34, 0x98, 0x7a, 0x6d, 0x06, 0xb8}},
};


const int numPeers = sizeof(peers) / sizeof(peers[0]);
int selectedPeerIndex = 0;

bool CONNECT_ESPNOW = false;
uint8_t SEND_ESPNOW = 0;
bool isSelectMode = true;  // true: 送信先選択モード, false: 送信モード

uint8_t sendDataLR[3] = {0, 0, 0};

JoyC joyc;
char text_buff[100];

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Wire.begin(0, 26, 400000UL);

  M5.Display.setBrightness(96);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextFont(&fonts::lgfxJapanGothic_12);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW Initialized");

  for (int i = 0; i < numPeers; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peers[i].mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      Serial.printf("Peer %d (%s) added\n", i, peers[i].name);
      CONNECT_ESPNOW = true;
    } else {
      Serial.printf("Failed to add peer %d\n", i);
    }
  }
}

void drawJoystickInfo() {
  M5.Lcd.drawCentreString("^", 33, 65, 4);
  M5.Lcd.drawCentreString("L", 33, 75, 4);
  M5.Lcd.drawCentreString("v", 33, 95, 4);
  M5.Lcd.drawCentreString("< R >", 96, 75, 4);

  if (joyc.GetY(0) > 155 || joyc.GetY(0) < 55) M5.Lcd.setTextColor(ORANGE);
  else M5.Lcd.setTextColor(WHITE);
  sprintf(text_buff, "%d", joyc.GetY(0));
  M5.Lcd.drawCentreString(text_buff, 33, 135, 4);

  if (joyc.GetX(1) > 155 || joyc.GetX(1) < 55) M5.Lcd.setTextColor(ORANGE);
  else M5.Lcd.setTextColor(WHITE);
  sprintf(text_buff, "%d", joyc.GetX(1));
  M5.Lcd.drawCentreString(text_buff, 96, 135, 4);
  M5.Lcd.setTextColor(WHITE);

}

void sendESPNowData() {
  for (int i = 0; i < numPeers; i++) {
    if (selectedPeerIndex == i) {
      esp_err_t result = esp_now_send(peers[i].mac, sendDataLR, sizeof(sendDataLR));
      Serial.printf("Sent to %s (%d): %s\n", peers[i].name, i, result == ESP_OK ? "OK" : "FAILED");
    }
  }
}

void loop() {
  M5.update();

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setCursor(20, 15, 2);
  M5.Lcd.printf("[Mode] %s\n", isSelectMode ? "Select" : "Send");
  M5.Lcd.setCursor(20, 35, 2);
  M5.Lcd.printf("[Target] %s\n", peers[selectedPeerIndex].name);

  drawJoystickInfo();

  if (M5.BtnB.wasPressed()) {
    isSelectMode = !isSelectMode;
  }

  if (isSelectMode) {
    M5.Lcd.drawCentreString("< Select Target >", 65, 215, 2);
    if (joyc.GetY(0) > 200) {
      selectedPeerIndex = (selectedPeerIndex + 1) % numPeers;
      delay(300);
    } else if (joyc.GetY(0) < 50) {
      selectedPeerIndex = (selectedPeerIndex - 1 + numPeers) % numPeers;
      delay(300);
    }
    if (M5.BtnA.wasPressed() || joyc.GetPress(0)) {
      isSelectMode = false;
    }
  } else {
    if (M5.BtnA.wasPressed()) {
      SEND_ESPNOW = (SEND_ESPNOW + 1) % 2;
    }

    if (SEND_ESPNOW == 1) {
      M5.Lcd.drawCentreString("< Sending Data >", 65, 215, 2);
      sendDataLR[2] = 1; // 操作モード
      sendDataLR[0] = (joyc.GetY(0) > 155) ? 1 : (joyc.GetY(0) < 55) ? 2 : 0; // Lスティック 1:前進、2:後退、0:停止

      if (joyc.GetY(0) > 155 || joyc.GetY(0) < 55) {
        // Lスティックの有効データが送信されたときのみLRスティックの両データを送信する
        sendDataLR[1] = (joyc.GetX(1) > 155) ? 1 : (joyc.GetX(1) < 55) ? 2 : 0; // Rスティック 1:左前右後、2:右前左後、0:停止
      } else {
        sendDataLR[1] = 0;
      }
      sendESPNowData();
    } else {
      M5.Lcd.drawCentreString("< Stop Sending >", 65, 215, 2);
      sendDataLR[2] = 0; // 停止モード
      sendDataLR[0] = 0; // Lスティック
      sendDataLR[1] = 0; // Rスティック
      sendESPNowData();
    }
  }

  delay(50);
}
