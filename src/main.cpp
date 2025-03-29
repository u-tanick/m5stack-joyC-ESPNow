/*
 * @Author: Sorzn
 * @Date: 2019-11-22 14:48:10
 * @LastEditTime: 2019-11-22 15:45:27
 * @Description: M5Stack project
 * @FilePath: /M5StickC/examples/Hat/JoyC/JoyC.ino
 *
 * @Author: u-tanick
 * @Description: Update use M5Unified.h
 */

#include "JoyC.h"
#include "M5Unified.h"
#include <Wire.h>
#include <stdint.h>

JoyC joyc;
bool SEND_ESPNOW = false;

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Wire.begin(0, 26, 400000UL);
  M5.Display.setBrightness(96);
  M5.Lcd.fillScreen(TFT_BLACK);
}

void loop()
{
  char text_buff[100];

  M5.update();
  M5.Lcd.fillScreen(TFT_BLACK);

  M5.Lcd.drawCentreString("[Connected OK]", 65, 10, 2);

  M5.Lcd.drawCentreString("Agl", 65, 75, 2);
  M5.Lcd.drawCentreString("Dst", 65, 105, 2);
  M5.Lcd.drawCentreString("X", 65, 135, 2);
  M5.Lcd.drawCentreString("Y", 65, 165, 2);
  M5.Lcd.drawCentreString("Prs", 65, 195, 2);

  M5.Lcd.drawCentreString("L", 30, 35, 4);
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

  M5.Lcd.drawCentreString("R", 100, 35, 4);
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

  if (M5.BtnA.wasPressed())
  {
    if (SEND_ESPNOW)
    {
      // データ送信停止
      SEND_ESPNOW = false;
    }
    else
    {
      // データ送信開始
      SEND_ESPNOW = true;
    }
  }

  if (SEND_ESPNOW)
  {
    // データ送信中
    M5.Lcd.drawCentreString("< Sending Data >", 65, 215, 2);
  }
  else
  {
    // データ送信停止中
    M5.Lcd.drawCentreString("< Stop Sending >", 65, 215, 2);
  }

  delay(1);
}