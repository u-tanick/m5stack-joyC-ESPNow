/*
 * @Author: Sorzn
 * @Date: 2019-11-22 14:48:24
 * @LastEditTime: 2019-11-22 15:44:44
 * @Description: M5Stack project
 * @FilePath: /M5StickC/examples/Hat/JoyC/JoyC.cpp
 */

#include "JoyC.h"
#include "M5Unified.h"
#include <Wire.h>
#include <stdint.h>

#define JOYC_ADDR 0x38

#define JOYC_COLOR_REG          0x20
#define JOYC_LEFT_X_REG         0x60
#define JOYC_LEFT_Y_REG         0x61
#define JOYC_RIGHT_X_REG        0x62
#define JOYC_RIGHT_Y_REG        0x63
#define JOYC_PRESS_REG          0x64
#define JOYC_LEFT_ANGLE_REG     0x70
#define JOYC_LEFT_DISTANCE_REG  0x74
#define JOYC_RIGHT_ANGLE_REG    0x72
#define JOYC_RIGHT_DISTANCE_REG 0x76

#define MINI_JOYC_ADDR          0x54
#define MINI_JOYC_POS_8BIT_X    0x20
#define MINI_JOYC_POS_8BIT_Y    0x21
#define MINI_JOYC_BUTTON_REG    0x30
#define MINI_JOYC_RGB_REG       0x40

void JoyC::Init() {
    CheckHat();
}

HatType JoyC::CheckHat() {
    // 1. 0x38 (JoyC Hat) をプローブ
    Wire.beginTransmission(JOYC_ADDR);
    if (Wire.endTransmission() == 0) {
        _hat_type = HAT_JOYC;
        return _hat_type;
    }

    // 2. 0x54 (MiniJoyC Hat) をプローブ
    Wire.beginTransmission(MINI_JOYC_ADDR);
    if (Wire.endTransmission() == 0) {
        _hat_type = HAT_MINI_JOYC;
        return _hat_type;
    }

    _hat_type = HAT_NONE;
    return _hat_type;
}

const char* JoyC::GetHatName() const {
    switch (_hat_type) {
        case HAT_JOYC:      return "JoyC (2-Stick)";
        case HAT_MINI_JOYC: return "Mini JoyC (1-Stick)";
        default:            return "No Hat";
    }
}

void JoyC::WriteBytes(uint8_t address, uint8_t Register_address, uint8_t* data,
                      size_t size) {
    Wire.beginTransmission(address);
    Wire.write(Register_address);
    for (size_t i = 0; i < size; i++) {
        Wire.write(*(data + i));
    }
    Wire.endTransmission();
}

uint8_t JoyC::ReadBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                        uint8_t* dest) {
    Wire.beginTransmission(address);
    Wire.write(subAddress);
    // MiniJoyC (STM32) は STOP フラグ(true)必須、JoyC は Repeated Start(false)
    bool sendStop = (address == MINI_JOYC_ADDR);
    if (Wire.endTransmission(sendStop) == 0 &&
        Wire.requestFrom(address, (uint8_t)count)) {
        uint8_t i = 0;
        while (Wire.available() && i < count) {
            dest[i++] = Wire.read();
        }
        return (i == count);
    }
    return false;
}

void JoyC::SetLedColor(uint32_t color) {
    if (_current_led_color == color) {
        return; // 色が同じならI2C書き込みをスキップしてバス負荷を激減
    }
    _current_led_color = color;

    uint8_t color_buff[3];
    color_buff[0] = (color & 0xff0000) >> 16;
    color_buff[1] = (color & 0xff00) >> 8;
    color_buff[2] = color & 0xff;

    if (_hat_type == HAT_JOYC) {
        WriteBytes(JOYC_ADDR, JOYC_COLOR_REG, color_buff, 3);
    } else if (_hat_type == HAT_MINI_JOYC) {
        WriteBytes(MINI_JOYC_ADDR, MINI_JOYC_RGB_REG, color_buff, 3);
    }
}

int8_t JoyC::GetRawMiniX() {
    uint8_t val = 0;
    ReadBytes(MINI_JOYC_ADDR, MINI_JOYC_POS_8BIT_X, 1, &val);
    return (int8_t)val;
}

int8_t JoyC::GetRawMiniY() {
    uint8_t val = 0;
    ReadBytes(MINI_JOYC_ADDR, MINI_JOYC_POS_8BIT_Y, 1, &val);
    return (int8_t)val;
}

uint8_t JoyC::GetX(uint8_t pos) {
    if (_hat_type == HAT_JOYC) {
        uint8_t value = 100;
        uint8_t read_reg = (pos == 0) ? JOYC_LEFT_X_REG : JOYC_RIGHT_X_REG;
        ReadBytes(JOYC_ADDR, read_reg, 1, &value);
        return value;
    } else if (_hat_type == HAT_MINI_JOYC) {
        // Mini JoyC は1スティックのみ。
        // X軸（左右旋回）を pos==0 / pos==1 どちらの要求にも返す。
        // raw: -128..127 (中心~0) -> JoyC互換の 0..200 (中心100) にマッピング
        // ※JoyC 右スティック準拠: 右倒し(<55)で右旋回、左倒し(>155)で左旋回
        int8_t raw_x = GetRawMiniX();
        // MiniJoyCの生値: 右に倒すと+側、左に倒すと-側の場合:
        // JoyCのGetX(1)は右倒しで小さく(<55)、左倒しで大きく(>155)なるため、
        // 100 - raw_x で自然に方向を一致させる
        int mapped = 100 - (int)raw_x;
        if (mapped < 0) mapped = 0;
        if (mapped > 200) mapped = 200;
        return (uint8_t)mapped;
    }
    return 100;
}

uint8_t JoyC::GetY(uint8_t pos) {
    if (_hat_type == HAT_JOYC) {
        uint8_t value = 100;
        uint8_t read_reg = (pos == 0) ? JOYC_LEFT_Y_REG : JOYC_RIGHT_Y_REG;
        ReadBytes(JOYC_ADDR, read_reg, 1, &value);
        return value;
    } else if (_hat_type == HAT_MINI_JOYC) {
        if (pos == 0) {
            // 左スティック(pos=0)相当として前後のY軸を返す
            // JoyC: 上倒し(前進)で大きく(>155)、下倒し(後退)で小さく(<55)
            // MiniJoyC: 前(上)に倒すと+側、手前(下)に倒すと-側
            int8_t raw_y = GetRawMiniY();
            int mapped = 100 + (int)raw_y;
            if (mapped < 0) mapped = 0;
            if (mapped > 200) mapped = 200;
            return (uint8_t)mapped;
        } else {
            return 100; // pos=1 のY軸はニュートラル
        }
    }
    return 100;
}

uint16_t JoyC::GetAngle(uint8_t pos) {
    if (_hat_type == HAT_JOYC) {
        uint8_t i2c_read_buff[2];
        uint8_t read_reg = (pos == 0) ? JOYC_LEFT_ANGLE_REG : JOYC_RIGHT_ANGLE_REG;
        ReadBytes(JOYC_ADDR, read_reg, 2, i2c_read_buff);
        return (i2c_read_buff[0] << 8) | i2c_read_buff[1];
    }
    return 0;
}

uint16_t JoyC::GetDistance(uint8_t pos) {
    if (_hat_type == HAT_JOYC) {
        uint8_t i2c_read_buff[2];
        uint8_t read_reg = (pos == 0) ? JOYC_LEFT_DISTANCE_REG : JOYC_RIGHT_DISTANCE_REG;
        ReadBytes(JOYC_ADDR, read_reg, 2, i2c_read_buff);
        return (i2c_read_buff[0] << 8) | i2c_read_buff[1];
    }
    return 0;
}

uint8_t JoyC::GetPress(uint8_t pos) {
    if (_hat_type == HAT_JOYC) {
        uint8_t press_value = 0;
        ReadBytes(JOYC_ADDR, JOYC_PRESS_REG, 1, &press_value);
        return (press_value & ((pos == 0) ? 0x10 : 0x01)) != 0;
    } else if (_hat_type == HAT_MINI_JOYC) {
        if (pos == 0) {
            uint8_t btn = 0;
            ReadBytes(MINI_JOYC_ADDR, MINI_JOYC_BUTTON_REG, 1, &btn);
            // 公式ドライバ仕様: 押下時=1, 離脱時=0
            return (btn != 0) ? 1 : 0;
        }
        return 0;
    }
    return 0;
}