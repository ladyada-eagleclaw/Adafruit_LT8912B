/*!
 * @file Adafruit_LT8912B.h
 * @brief Portable I2C control for the LT8912B MIPI DSI to HDMI/DVI bridge.
 *
 * SPDX-FileCopyrightText: 2024-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Adafruit Industries
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified by Adafruit: Arduino/BusIO interface, timing validation and
 * video-only configuration. Adapted from Espressif's esp_lcd_lt8912b driver.
 */

#ifndef ADAFRUIT_LT8912B_H
#define ADAFRUIT_LT8912B_H

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Arduino.h>
#include <Wire.h>

/** @brief DSI video timing. Defaults describe the 800 x 480 example mode. */
struct LT8912B_Timing {
  uint16_t width = 800;       ///< Active horizontal pixels; nonzero.
  uint16_t height = 480;      ///< Active vertical lines; nonzero.
  uint16_t hFrontPorch = 112; ///< Horizontal front porch, in pixel clocks.
  uint16_t hSync = 48;        ///< Horizontal sync width, 1 to 255 pixel clocks.
  uint16_t hBackPorch = 40;   ///< Horizontal back porch, in pixel clocks.
  uint16_t vFrontPorch = 13;  ///< Vertical front porch, in lines.
  uint16_t vSync = 3;         ///< Vertical sync width, 1 to 255 lines.
  uint16_t vBackPorch = 29;   ///< Vertical back porch, in lines.
  float pixelClockMHz = 31.5; ///< Positive finite source pixel clock, in MHz.
  bool hSyncPositive =
      false; ///< Horizontal polarity control; true is positive.
  bool vSyncPositive = false; ///< Vertical polarity control; true is positive.
  uint8_t vic = 0;         ///< AVI video code, 0 to 127; 0 for custom timing.
  uint8_t aspectRatio = 0; ///< AVI aspect: 0 unspecified, 1 is 4:3, 2 is 16:9.
};

/** @brief LT8912B control over its three fixed I2C register-bank addresses. */
class Adafruit_LT8912B {
 public:
  Adafruit_LT8912B();
  ~Adafruit_LT8912B();
  /** Copying owned bus interfaces is forbidden. @param other Unused source. */
  Adafruit_LT8912B(const Adafruit_LT8912B& other) = delete;
  /** Assignment is forbidden. @param other Unused source. @return No result. */
  Adafruit_LT8912B& operator=(const Adafruit_LT8912B& other) = delete;

  bool begin(TwoWire* wire = &Wire, uint16_t resetPin = 0xFFFF);
  bool reset();
  bool configure(const LT8912B_Timing& timing, uint8_t lanes = 2);
  bool setHDMI(bool enabled);
  bool enableOutput(bool enabled);
  int8_t getHPD();

 private:
  /** @brief One ordered vendor initialization write. */
  struct RegisterValue {
    uint8_t address; ///< Register address within the selected bank.
    uint8_t value;   ///< Complete vendor initialization value.
  };

  /** @brief Checked status snapshot; bit 7 is HDMI hot-plug detect. */
  union HotPlugStatus {
    uint8_t raw; ///< Complete status byte for BusIO's checked read.
    struct {
      uint8_t reserved : 7;  ///< Other status bits, not interpreted here.
      uint8_t connected : 1; ///< HDMI hot-plug detect input.
    } bits;                  ///< Named fields in the status register.
  };

  /** @brief Register addresses used outside vendor initialization tables. */
  enum Register : uint8_t {
    MIPI_RESET = 0x03,    ///< Main bank MIPI receiver reset.
    DDS_RESET = 0x05,     ///< Main bank DDS reset.
    TX_CONTROL = 0x33,    ///< Main bank HDMI transmitter control.
    SYNC_POLARITY = 0xAB, ///< Main bank H/V sync polarity fields.
    HDMI_MODE = 0xB2,     ///< Main bank HDMI versus DVI mode.
    HPD_STATUS = 0xC1,    ///< Main bank HDMI cable status.
    DSI_LANES = 0x13,     ///< DSI bank data lane count.
    H_SYNC = 0x18,        ///< DSI bank horizontal sync width.
    V_SYNC = 0x19,        ///< DSI bank vertical sync width.
    H_ACTIVE = 0x1C,      ///< DSI bank 16-bit active width.
    H_TOTAL = 0x34,       ///< DSI bank 16-bit total width.
    V_TOTAL = 0x36,       ///< DSI bank 16-bit total height.
    V_BACK_PORCH = 0x38,  ///< DSI bank 16-bit vertical back porch.
    V_FRONT_PORCH = 0x3A, ///< DSI bank 16-bit vertical front porch.
    H_BACK_PORCH = 0x3C,  ///< DSI bank 16-bit horizontal back porch.
    H_FRONT_PORCH = 0x3E, ///< DSI bank 16-bit horizontal front porch.
    AVI_CHECKSUM = 0x43,  ///< AVI bank checksum followed by payload.
  };

  bool writeSequence(Adafruit_I2CDevice* device, const RegisterValue* sequence,
                     size_t count);
  bool writeTiming(const LT8912B_Timing& timing);
  bool resetReceiver();

  Adafruit_I2CDevice* _main = nullptr; ///< Owned main register-bank device.
  Adafruit_I2CDevice* _dsi = nullptr;  ///< Owned DSI register-bank device.
  Adafruit_I2CDevice* _avi = nullptr; ///< Owned AVI/audio register-bank device.
  uint16_t _resetPin = 0xFFFF; ///< Active-low reset, or no connected pin.
  bool _initialized = false;   ///< All three register banks acknowledged.
};

#endif
