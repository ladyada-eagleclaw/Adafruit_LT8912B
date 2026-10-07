// SPDX-FileCopyrightText: 2026 Adafruit Industries
// SPDX-License-Identifier: Apache-2.0
/** @file Adafruit_LT8912B_P4.h
 * @brief ESP32-P4 DSI host for the LT8912B bridge.
 */
#pragma once

#include "Adafruit_LT8912B.h"
#ifdef ARDUINO_ARCH_ESP32
#include "sdkconfig.h"
#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"

/** ESP32-P4 host setup, separate from the portable I2C bridge driver.
 * Requires Arduino ESP32 3.3.11 or a compatible IDF 5.5 build and PSRAM.
 * The caller owns the bridge and must keep it alive until end().
 */
class Adafruit_LT8912B_P4 {
 public:
  Adafruit_LT8912B_P4();
  ~Adafruit_LT8912B_P4();
  /** Copying host resources is forbidden. @param other Unused source. */
  Adafruit_LT8912B_P4(const Adafruit_LT8912B_P4& other) = delete;
  /** Assignment is forbidden. @param other Unused source. @return No result. */
  Adafruit_LT8912B_P4& operator=(const Adafruit_LT8912B_P4& other) = delete;
  bool begin(Adafruit_LT8912B& bridge, const LT8912B_Timing& timing,
             uint8_t buffers = 1, uint32_t laneBitRateMbps = 960);
  void end();
  uint8_t* getFrameBuffer(uint8_t index = 0);
  bool show(uint8_t index = 0);
  esp_lcd_panel_handle_t getPanel();

 private:
  Adafruit_LT8912B* _bridge = nullptr;
  esp_ldo_channel_handle_t _power = nullptr;
  esp_lcd_dsi_bus_handle_t _bus = nullptr;
  esp_lcd_panel_handle_t _panel = nullptr;
  uint8_t* _pixels[3] = {};
  uint8_t _buffers = 0;
  uint16_t _width = 0;
  uint16_t _height = 0;
};
#endif
#endif
