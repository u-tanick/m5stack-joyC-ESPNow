/*
 * @Author: Sorzn
 * @Date: 2019-11-22 14:48:31
 * @LastEditTime: 2019-11-22 15:22:45
 * @Description: M5Stack HAT JOYC project
 * @FilePath: /M5StickC/examples/Hat/JoyC/JoyC.h
 */

#ifndef _HAT_JOYC_H_
#define _HAT_JOYC_H_

#include "M5Unified.h"
#include <Wire.h>
#include <stdint.h>

enum HatType {
    HAT_NONE = 0,
    HAT_JOYC,       // JoyC Hat (0x38, 左右2スティック)
    HAT_MINI_JOYC   // Hat Mini JoyC (0x54, 1スティック)
};

class JoyC {
   public:
    void Init();
    HatType CheckHat();
    HatType GetHatType() const { return _hat_type; }
    const char* GetHatName() const;

    /**
     * @description: Set Hat RGB LED
     * @param color: (r << 16) | (g << 8) | b
     */
    void SetLedColor(uint32_t color);

    /**
     * @param pos: 0: left, 1: right (MiniJoyC では 0:Y軸前後/X軸左右, 1:X軸左右)
     * @return: x value: 0 ~ 200 (center ~100)
     */
    uint8_t GetX(uint8_t pos);

    /**
     * @param pos: 0: left, 1: right (MiniJoyC では 0:Y軸前後, 1:100ニュートラル)
     * @return: y value: 0 ~ 200 (center ~100)
     */
    uint8_t GetY(uint8_t pos);

    uint16_t GetAngle(uint8_t pos);
    uint16_t GetDistance(uint8_t pos);

    /**
     * @return: 1 if pressed, 0 otherwise
     */
    uint8_t GetPress(uint8_t pos);

    int8_t GetRawMiniX();
    int8_t GetRawMiniY();

   private:
    HatType _hat_type = HAT_NONE;
    uint32_t _last_check = 0;
    uint32_t _current_led_color = 0xFFFFFFFF;

    void WriteBytes(uint8_t address, uint8_t Register_address, uint8_t* data,
                    size_t size);
    uint8_t ReadBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                      uint8_t* dest);
};

#endif