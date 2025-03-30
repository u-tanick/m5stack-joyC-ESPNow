/*
 * original
 * @Author: Sorzn
 * @Date: 2019-11-22 14:48:10
 * @LastEditTime: 2019-11-22 15:45:27
 * @Description: M5Stack project
 * @FilePath: /M5StickC/examples/Hat/JoyC/JoyC.ino
 *
 * customized by u-tanick
 * @Author: u-tanick
 * @Description: Update use M5Unified.h
 */

#include "JoyC.h"
#include "M5Unified.h"
#include <Wire.h>
#include <stdint.h>

JoyC joyc;
bool CONNECT_ESPNOW = false;
u_int8_t SEND_ESPNOW = 0;

// ==================================
// for ESPNow
#include "WiFi.h"
#include <esp_now.h>

// ブロードキャスト用のアドレス（テスト用）
// uint8_t macAddresses[][6] = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// 複数のMACアドレスを定義可能
uint8_t macAddresses[][6] = {
  {0xe8, 0x9f, 0x6d, 0x08, 0x91, 0x20}
};

// 登録するデバイス数
const int numPeers = sizeof(macAddresses) / sizeof(macAddresses[0]);

// 送信データ（別の型とか構造体でもいい）
uint8_t sendDataLR[3] = {0, 0, 0}; // Lスティック、Rスティック、操作モード

// ================================== End

void setup()
{
  // 設定用の情報を抽出
  auto cfg = M5.config();
  // M5Stackをcfgの設定で初期化
  M5.begin(cfg);

  Wire.begin(0, 26, 400000UL);
  M5.Display.setBrightness(96);
  M5.Lcd.fillScreen(TFT_BLACK);

  // ==== for ESPNow ==============================
  // WiFi初期化
  WiFi.mode(WIFI_STA);
  // ESP-NOWの初期化
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW Initialized");

  for (int i = 0; i < numPeers; i++)
  {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddresses[i], 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      Serial.printf("Failed to add peer %d\n", i);
      CONNECT_ESPNOW = false;
    }
    else
    {
      Serial.printf("Peer %d added\n", i);
      CONNECT_ESPNOW = true;
    }
  }
}

char text_buff[100];

void loop()
{
  M5.update();
  M5.Lcd.fillScreen(TFT_BLACK);

  // ESPNowの接続状態情報
  if(CONNECT_ESPNOW)
  {
    // 接続中
    M5.Lcd.drawCentreString("[Connected]", 65, 10, 2);
  }
  else
  {
    // 接続失敗
    M5.Lcd.drawCentreString("[NO Connection]", 65, 10, 2);
  }

  // 情報ラベル（固定表示）
  M5.Lcd.drawCentreString("L", 30, 35, 4);
  M5.Lcd.drawCentreString("R", 100, 35, 4);
  M5.Lcd.drawCentreString("Agl", 65, 75, 2);
  M5.Lcd.drawCentreString("Dst", 65, 105, 2);
  M5.Lcd.drawCentreString("X", 65, 135, 2);
  M5.Lcd.drawCentreString("Y", 65, 165, 2);
  M5.Lcd.drawCentreString("Prs", 65, 195, 2);

  // L スティックの情報
  sprintf(text_buff, "%d", joyc.GetAngle(0));
  M5.Lcd.drawCentreString(text_buff, 30, 75, 2);
  sprintf(text_buff, "%d", joyc.GetDistance(0));
  M5.Lcd.drawCentreString(text_buff, 30, 105, 2);
  sprintf(text_buff, "%d", joyc.GetX(0));
  M5.Lcd.drawCentreString(text_buff, 30, 135, 2);
  sprintf(text_buff, "%d", joyc.GetY(0));
  M5.Lcd.drawCentreString(text_buff, 30, 165, 2);
  sprintf(text_buff, "%d", joyc.GetPress(0));
  M5.Lcd.drawCentreString(text_buff, 30, 195, 2);

  // R スティックの情報
  sprintf(text_buff, "%d", joyc.GetAngle(1));
  M5.Lcd.drawCentreString(text_buff, 100, 75, 2);
  sprintf(text_buff, "%d", joyc.GetDistance(1));
  M5.Lcd.drawCentreString(text_buff, 100, 105, 2);
  sprintf(text_buff, "%d", joyc.GetX(1));
  M5.Lcd.drawCentreString(text_buff, 100, 135, 2);
  sprintf(text_buff, "%d", joyc.GetY(1));
  M5.Lcd.drawCentreString(text_buff, 100, 165, 2);
  sprintf(text_buff, "%d", joyc.GetPress(1));
  M5.Lcd.drawCentreString(text_buff, 100, 195, 2);

  // === ボタンAを押してデータ送信 開始/停止 ===
  if (M5.BtnA.wasPressed())
  {
    if (SEND_ESPNOW == 0)
    {
      // データ送信開始：Lスティック操作のみ
      SEND_ESPNOW = 1;
    }
    else if (SEND_ESPNOW == 1)
    {
      // データ送信開始：LRスティック操作
      SEND_ESPNOW = 2;
    }
    else
    {
      // データ送信停止
      SEND_ESPNOW = 0;
    }
  }

  if (SEND_ESPNOW == 1)
  {
    // データ送信開始：Lスティック一括操作
    M5.Lcd.drawCentreString("< Sending Data L >", 65, 215, 2);

    // 操作モード：Lスティック一括操作
    sendDataLR[2] = 1;

    // LスティックのＹ軸値が
    // 155以上：前進(1)
    // 55以下：後退(2)
    // それ以外：停止(0)
    // Lスティック
    if (joyc.GetY(0) > 155)
    {
      // LスティックのY軸値が155以上：前進(1)
      sendDataLR[0] = 1;
    }
    else if (joyc.GetY(0) < 55)
    {
      // LスティックのY軸値が55以下：後退(2)
      sendDataLR[0] = 2;
    }
    else
    {
      // LスティックのY軸値が55以上、155以下：停止(0)
      sendDataLR[0] = 0;
    }
    // RスティックのY軸値は0に固定
    sendDataLR[1] = 0;

    // 順番にESP-NOWデータ送信
    for (int i = 0; i < numPeers; i++)
    {
      esp_err_t result = esp_now_send(macAddresses[i], sendDataLR, sizeof(sendDataLR));

      if (result == ESP_OK)
      {
        Serial.printf("Data sent to Peer %d\n", i);
      }
      else
      {
        Serial.printf("Failed to send data to Peer %d, Error: %d\n", i, result);
      }
    }

  }
  else if (SEND_ESPNOW == 2)
  {
    // データ送信開始：LRスティック独立操作
    M5.Lcd.drawCentreString("< Sending Data LR >", 65, 215, 2);

    // 操作モード：LRスティック独立操作
    sendDataLR[2] = 2;

    // 各スティックのＹ軸値が
    // 155以上：前進(1)
    // 55以下：後退(2)
    // それ以外：停止(0)

    // Lスティック
    if (joyc.GetY(0) > 155)
    {
      // LスティックのY軸値が155以上：前進(1)
      sendDataLR[0] = 1;
    }
    else if (joyc.GetY(0) < 55)
    {
      // LスティックのY軸値が55以下：後退(2)
      sendDataLR[0] = 2;
    }
    else
    {
      // LスティックのY軸値が55以上、155以下：停止(0)
      sendDataLR[0] = 0;
    }

    // Rスティック
    if (joyc.GetY(1) > 155)
    {
      // RスティックのY軸値が155以上：前進(1)
      sendDataLR[1] = 1;
    }
    else if (joyc.GetY(1) < 55)
    {
      // RスティックのY軸値が55以下：後退(2)
      sendDataLR[1] = 2;
    }
    else
    {
      // RスティックのY軸値が55以上、155以下：停止(0)
      sendDataLR[1] = 0;
    }
    
    // 順番にESP-NOWデータ送信
    for (int i = 0; i < numPeers; i++)
    {
      esp_err_t result = esp_now_send(macAddresses[i], sendDataLR, sizeof(sendDataLR));

      if (result == ESP_OK)
      {
        Serial.printf("Data sent to Peer %d\n", i);
      }
      else
      {
        Serial.printf("Failed to send data to Peer %d, Error: %d\n", i, result);
      }
    }

  }
  else
  {
    // SEND_ESPNOW == 0
    // データ送信停止
    M5.Lcd.drawCentreString("< Stop Sending >", 65, 215, 2);

    // すべて停止するデータを送信
    sendDataLR[0] = 0; // Lスティック一括操作：停止
    sendDataLR[1] = 0; // LRスティック独立操作：停止
    sendDataLR[2] = 0; // 操作モード：停止

    // 順番にESP-NOWデータ送信
    for (int i = 0; i < numPeers; i++)
    {
      esp_err_t result = esp_now_send(macAddresses[i], sendDataLR, sizeof(sendDataLR));

      if (result == ESP_OK)
      {
        Serial.printf("Data sent to Peer %d\n", i);
      }
      else
      {
        Serial.printf("Failed to send data to Peer %d, Error: %d\n", i, result);
      }
    }
  }

  delay(1);
}