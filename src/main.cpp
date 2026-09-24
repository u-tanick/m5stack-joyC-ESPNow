#include "JoyC.h"
#include "M5Unified.h"
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

struct PeerInfo {
  const char *name;
  uint8_t mac[6];
};

PeerInfo peers[] = {
    {"Tank1:Blue", {0xc8, 0x85, 0x41, 0x67, 0xe1, 0x78}},
    {"Tank2:Yellow", {0x90, 0x15, 0x06, 0xFA, 0x05, 0xE8}},
    {"Tank3:Green", {0x30, 0xed, 0xa0, 0xca, 0x5a, 0xc4}},
    {"Kani", {0x2c, 0xbc, 0xbb, 0x94, 0x32, 0x0c}},
    {"Tako", {0xb4, 0x8a, 0x0a, 0xbf, 0x4a, 0x7c}},
    {"Car", {0xe8, 0x9f, 0x6d, 0x08, 0x91, 0x20}},
    {"Broad", {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
};

const int numPeers = sizeof(peers) / sizeof(peers[0]);
int selectedPeerIndex = 0;

bool CONNECT_ESPNOW = false;
uint8_t SEND_ESPNOW = 0;
bool isSelectMode = true; // true: 送信先選択モード, false: 送信モード

uint8_t sendDataLR[3] = {0, 0, 0};

JoyC joyc;
M5Canvas canvas(&M5.Lcd);
char text_buff[100];

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Wire.begin(0, 26, 400000UL);

  M5.Display.setBrightness(96);
  canvas.createSprite(135, 240);
  canvas.setTextFont(&fonts::lgfxJapanGothic_12);

  joyc.Init();
  Serial.printf("Detected Hat: %s\n", joyc.GetHatName());

  // 起動時の初期化画面表示
  canvas.fillScreen(TFT_BLACK);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawCentreString("CONTROLLER INIT", 67, 70, 2);
  canvas.setTextColor(joyc.GetHatType() != HAT_NONE ? TFT_GREEN : TFT_RED);
  canvas.drawCentreString(joyc.GetHatName(), 67, 105, 2);
  canvas.pushSprite(0, 0);
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
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
  if (joyc.GetHatType() == HAT_MINI_JOYC) {
    // MiniJoyC (1スティック) 専用UI
    canvas.setTextColor(TFT_CYAN);
    canvas.drawCentreString("- Mini JoyC -", 67, 50, 2);

    uint8_t stick_x = joyc.GetX(0); // 0..200 (center 100)
    uint8_t stick_y = joyc.GetY(0); // 0..200 (center 100)
    bool pressed = (joyc.GetPress(0) != 0);

    // ジョイスティックレーダー（円＋十字線＋ドット）
    const int cx = 67;
    const int cy = 96;
    const int r = 26;
    canvas.drawCircle(cx, cy, r, TFT_DARKGRAY);
    canvas.drawFastHLine(cx - r + 4, cy, (r - 4) * 2, TFT_DARKGRAY);
    canvas.drawFastVLine(cx, cy - r + 4, (r - 4) * 2, TFT_DARKGRAY);

    // ドット描画 (画面上: 右倒し=+X, 前倒し=-Y)
    int dx = (int)(100 - stick_x) * (r - 6) / 100;
    int dy = -(int)(stick_y - 100) * (r - 6) / 100;
    if (dx < -(r - 6)) dx = -(r - 6);
    if (dx > (r - 6)) dx = (r - 6);
    if (dy < -(r - 6)) dy = -(r - 6);
    if (dy > (r - 6)) dy = (r - 6);

    bool isMoving = (stick_y > 155 || stick_y < 55 || stick_x > 155 || stick_x < 55);
    uint16_t dotColor = pressed ? TFT_YELLOW : (isMoving ? TFT_ORANGE : TFT_GREEN);
    canvas.fillCircle(cx + dx, cy + dy, 5, dotColor);
    canvas.drawCircle(cx + dx, cy + dy, 5, TFT_WHITE);

    // 数値表示
    if (stick_x > 155 || stick_x < 55) canvas.setTextColor(TFT_ORANGE);
    else canvas.setTextColor(TFT_WHITE);
    sprintf(text_buff, "X:%3d", stick_x);
    canvas.drawString(text_buff, 18, 130, 2);

    if (stick_y > 155 || stick_y < 55) canvas.setTextColor(TFT_ORANGE);
    else canvas.setTextColor(TFT_WHITE);
    sprintf(text_buff, "Y:%3d", stick_y);
    canvas.drawString(text_buff, 76, 130, 2);

    canvas.setTextColor(TFT_WHITE);
    if (pressed) {
      canvas.setTextColor(TFT_YELLOW);
      canvas.drawCentreString("[PUSH]", 67, 72, 2);
      canvas.setTextColor(TFT_WHITE);
    }
  } else if (joyc.GetHatType() == HAT_JOYC) {
    // JoyC Hat (2スティック) 従来UI
    canvas.drawFastVLine(67, 52, 95, TFT_DARKGRAY);

    canvas.setTextColor(TFT_WHITE);
    canvas.drawCentreString("^", 33, 55, 4);
    canvas.drawCentreString("L", 33, 72, 4);
    canvas.drawCentreString("v", 33, 90, 4);
    canvas.drawCentreString("< R >", 101, 72, 4);

    if (joyc.GetY(0) > 155 || joyc.GetY(0) < 55)
      canvas.setTextColor(TFT_ORANGE);
    else
      canvas.setTextColor(TFT_WHITE);
    sprintf(text_buff, "%d", joyc.GetY(0));
    canvas.drawCentreString(text_buff, 33, 125, 4);

    if (joyc.GetX(1) > 155 || joyc.GetX(1) < 55)
      canvas.setTextColor(TFT_ORANGE);
    else
      canvas.setTextColor(TFT_WHITE);
    sprintf(text_buff, "%d", joyc.GetX(1));
    canvas.drawCentreString(text_buff, 101, 125, 4);
    canvas.setTextColor(TFT_WHITE);
  } else {
    // Hat 未検出時
    canvas.setTextColor(TFT_RED);
    canvas.drawCentreString("NO HAT DETECTED", 67, 85, 2);
    canvas.setTextColor(TFT_LIGHTGRAY);
    canvas.drawCentreString("Checking I2C...", 67, 110, 2);
  }
}

void sendESPNowData() {
  for (int i = 0; i < numPeers; i++) {
    if (selectedPeerIndex == i) {
      esp_err_t result =
          esp_now_send(peers[i].mac, sendDataLR, sizeof(sendDataLR));
      static uint32_t last_log = 0;
      if (millis() - last_log >= 500) {
        last_log = millis();
        Serial.printf("Sent to %s (%d): %s | FB=%d, Turn=%d, Mode=%d | "
                      "raw(L_Y=%d, R_X=%d)\n",
                      peers[i].name, i, result == ESP_OK ? "OK" : "FAILED",
                      sendDataLR[0], sendDataLR[1], sendDataLR[2], joyc.GetY(0),
                      joyc.GetX(1));
      }
    }
  }
}

void loop() {
  M5.update();

  // 定期的なHat接続チェック（未接続の場合）
  static uint32_t s_last_hat_check = 0;
  if (millis() - s_last_hat_check >= 1000) {
    s_last_hat_check = millis();
    if (joyc.GetHatType() == HAT_NONE) {
      joyc.CheckHat();
    }
  }

  // スティック押し込みのエッジ検出 (wasPressed)
  static bool s_last_stick_press = false;
  bool current_stick_press = (joyc.GetPress(0) != 0);
  bool stick_pressed_edge = (current_stick_press && !s_last_stick_press);
  s_last_stick_press = current_stick_press;

  canvas.fillScreen(TFT_BLACK);

  // ヘッダー部
  uint16_t headerBg = isSelectMode
                          ? TFT_NAVY
                          : (SEND_ESPNOW == 1 ? TFT_DARKGREEN : TFT_DARKGREY);
  canvas.fillRect(0, 0, 135, 46, headerBg);
  canvas.drawFastHLine(0, 46, 135, TFT_WHITE);

  canvas.setTextColor(TFT_WHITE, headerBg);
  canvas.drawCentreString(
      isSelectMode ? "[ TARGET SELECT ]" : "[ CONTROL MODE ]", 67, 5, 2);
  sprintf(text_buff, ">> %s <<", peers[selectedPeerIndex].name);
  canvas.drawCentreString(text_buff, 67, 24, 2);

  // スティック情報部
  drawJoystickInfo();

  if (M5.BtnB.wasPressed()) {
    isSelectMode = !isSelectMode;
    if (isSelectMode) {
      // 選択モードに入った時は送信を停止
      SEND_ESPNOW = 0;
      sendDataLR[0] = 0;
      sendDataLR[1] = 0;
      sendDataLR[2] = 0;
      sendESPNowData();
      M5.Power.setLed(0);
      joyc.SetLedColor(0x000030); // 選択モード: 青
    }
  }

  if (isSelectMode) {
    joyc.SetLedColor(0x000030); // 選択モード: 青

    // 選択モード中のステータスバッジ
    canvas.fillRoundRect(8, 160, 119, 36, 6, TFT_BLUE);
    canvas.drawRoundRect(8, 160, 119, 36, 6, TFT_CYAN);
    canvas.setTextColor(TFT_WHITE, TFT_BLUE);
    canvas.drawCentreString("SELECTING", 67, 170, 2);

    // 操作ガイド（2行表示で幅135pxに収める）
    canvas.drawFastHLine(15, 201, 105, TFT_DARKGRAY);
    canvas.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);
    canvas.drawCentreString("Stick-Y: Move", 67, 205, 2);
    canvas.drawCentreString("BtnA/Press: OK", 67, 221, 2);
    M5.Power.setLed(0);

    uint8_t ly = joyc.GetY(0);
    if (ly < 60) {
      // スティック下: 次の選択肢へ進む
      selectedPeerIndex = (selectedPeerIndex + 1) % numPeers;
      delay(250);
    } else if (ly > 140) {
      // スティック上: 前の選択肢へ戻る
      selectedPeerIndex = (selectedPeerIndex - 1 + numPeers) % numPeers;
      delay(250);
    }
    if (M5.BtnA.wasPressed() || stick_pressed_edge) {
      isSelectMode = false;
    }
  } else {
    // スティック押し込みでも START / STOP をトグル可能
    if (M5.BtnA.wasPressed() || stick_pressed_edge) {
      SEND_ESPNOW = (SEND_ESPNOW + 1) % 2;
    }

    if (SEND_ESPNOW == 1) {
      joyc.SetLedColor(0x003000); // 送信中: 緑

      // 送信中ステータスバッジ（緑色背景＋黒文字で視認性向上）
      canvas.fillRoundRect(8, 158, 119, 38, 6, TFT_GREEN);
      canvas.drawRoundRect(8, 158, 119, 38, 6, TFT_WHITE);
      canvas.setTextColor(TFT_BLACK, TFT_GREEN);
      canvas.drawCentreString("[ TX ON ]", 67, 166, 4);

      // 操作ガイド（2行表示で幅135pxに収める）
      canvas.drawFastHLine(15, 201, 105, TFT_DARKGRAY);
      canvas.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);
      canvas.drawCentreString("BtnA: STOP", 67, 205, 2);
      canvas.drawCentreString("BtnB: SELECT", 67, 221, 2);
      M5.Power.setLed(1);

      sendDataLR[2] = 1; // 操作モード

      const char *targetName = peers[selectedPeerIndex].name;

      if (strcmp(targetName, "Kani") == 0) {
        // "Kani" (Crab) operation specification:
        // Right stick: tilt left/right to walk left/right
        sendDataLR[0] = (joyc.GetX(1) > 155) ? 1 : (joyc.GetX(1) < 55) ? 2 : 0;

        // Left stick: tilt up/down while operating the right stick to perform
        // turn/spin motion
        if (sendDataLR[0] != 0) {
          sendDataLR[1] = (joyc.GetY(0) > 155)  ? 1
                          : (joyc.GetY(0) < 55) ? 2
                                                : 0;
        } else {
          sendDataLR[1] = 0;
        }
      } else if (strcmp(targetName, "Tako") == 0) {
        // "Tako" (Octopus) operation specification:
        // Left stick: tilt up/down to move face up/down
        sendDataLR[0] = (joyc.GetY(0) > 155) ? 1 : (joyc.GetY(0) < 55) ? 2 : 0;

        // Right stick: tilt left/right to rotate leg servo (always active,
        // independent)
        sendDataLR[1] = (joyc.GetX(1) > 155) ? 1 : (joyc.GetX(1) < 55) ? 2 : 0;
      } else if (strncmp(targetName, "Tank", 4) == 0 ||
                 strcmp(targetName, "Broad") == 0) {
        // "Tank1" / "Tank2" / "Tank3" / "Broad" operation specification:
        // Right stick turn takes priority over Left stick operation
        bool rStickActive = (joyc.GetX(1) > 155 || joyc.GetX(1) < 55);
        if (rStickActive) {
          // 右倒し(<55)で1(右旋回)、左倒し(>155)で2(左旋回)
          sendDataLR[1] = (joyc.GetX(1) > 155) ? 2 : 1;
          sendDataLR[0] = 0; // Overridden by Right stick (takes priority)
        } else {
          sendDataLR[1] = 0;
          sendDataLR[0] = (joyc.GetY(0) > 155)  ? 1
                          : (joyc.GetY(0) < 55) ? 2
                                                : 0;
        }
      } else {
        // "Car" / "Buggy" operation specification:
        // Left stick Y and Right stick X are sent independently
        sendDataLR[0] = (joyc.GetY(0) > 155) ? 1 : (joyc.GetY(0) < 55) ? 2 : 0;
        sendDataLR[1] = (joyc.GetX(1) > 155) ? 1 : (joyc.GetX(1) < 55) ? 2 : 0;
      }
      sendESPNowData();
    } else {
      joyc.SetLedColor(0x300000); // 停止中: 赤

      // 停止中ステータスバッジ（赤色背景＋白文字で視認性向上）
      canvas.fillRoundRect(8, 158, 119, 38, 6, TFT_RED);
      canvas.drawRoundRect(8, 158, 119, 38, 6, TFT_WHITE);
      canvas.setTextColor(TFT_WHITE, TFT_RED);
      canvas.drawCentreString("[ TX OFF ]", 67, 166, 4);

      // 操作ガイド（2行表示で幅135pxに収める）
      canvas.drawFastHLine(15, 201, 105, TFT_DARKGRAY);
      canvas.setTextColor(TFT_LIGHTGRAY, TFT_BLACK);
      canvas.drawCentreString("BtnA: START", 67, 205, 2);
      canvas.drawCentreString("BtnB: SELECT", 67, 221, 2);
      M5.Power.setLed(0);

      sendDataLR[2] = 0; // 停止モード
      sendDataLR[0] = 0; // Lスティック
      sendDataLR[1] = 0; // Rスティック
      sendESPNowData();
    }
  }

  // ダブルバッファ転送でちらつきを完全防止
  canvas.pushSprite(0, 0);

  delay(20);
}

