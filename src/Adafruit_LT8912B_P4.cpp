// SPDX-FileCopyrightText: 2026 Adafruit Industries
// SPDX-License-Identifier: Apache-2.0
/** @file Adafruit_LT8912B_P4.cpp
 * @brief ESP32-P4 DSI setup for an independently controlled LT8912B.
 */
#include "Adafruit_LT8912B_P4.h"
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32P4)
#include <math.h>

/** Create an uninitialized P4 DSI host. */
Adafruit_LT8912B_P4::Adafruit_LT8912B_P4() {}

#include "esp_lcd_panel_ops.h"

/** Release display resources. Keep the bridge alive until this completes. */
Adafruit_LT8912B_P4::~Adafruit_LT8912B_P4() {
  end();
}

/** Start two-lane RGB888 DSI and configure the bridge for video-only DVI.
 * @param bridge Bridge whose begin() has already succeeded.
 * @param timing Matching DSI and bridge timing; pixel clock is in MHz.
 * @param buffers Number of panel-owned framebuffers, from 1 through 3.
 * @param laneBitRateMbps DSI rate per lane in megabits per second.
 * @return True on success. Failure releases partially allocated host resources.
 * @note Uses P4 internal PHY LDO channel 3 at 2.5 V. begin() is single-owner;
 * call end() before changing timing. This does not configure the I2C pins.
 */
bool Adafruit_LT8912B_P4::begin(Adafruit_LT8912B& bridge,
                                const LT8912B_Timing& timing, uint8_t buffers,
                                uint32_t laneBitRateMbps) {
  uint32_t hTotal = (uint32_t)timing.width + timing.hFrontPorch + timing.hSync +
                    timing.hBackPorch;
  uint32_t vTotal = (uint32_t)timing.height + timing.vFrontPorch +
                    timing.vSync + timing.vBackPorch;
  if (_bridge || buffers < 1 || buffers > 3 || !timing.width ||
      !timing.height || !isfinite(timing.pixelClockMHz) ||
      timing.pixelClockMHz <= 0 || !laneBitRateMbps || !timing.hSync ||
      timing.hSync > 255 || !timing.vSync || timing.vSync > 255 ||
      hTotal > 65535 || vTotal > 65535 || timing.vic > 127 ||
      timing.aspectRatio > 2) {
    return false;
  }
  _bridge = &bridge;
  esp_ldo_channel_config_t power = {};
  power.chan_id = 3;
  power.voltage_mv = 2500;
  if (esp_ldo_acquire_channel(&power, &_power) != ESP_OK) {
    end();
    return false;
  }
  esp_lcd_dsi_bus_config_t bus = {};
  bus.bus_id = 0;
  bus.num_data_lanes = 2;
  bus.phy_clk_src = MIPI_DSI_PHY_PLLREF_CLK_SRC_DEFAULT_LEGACY;
  bus.lane_bit_rate_mbps = laneBitRateMbps;
  if (esp_lcd_new_dsi_bus(&bus, &_bus) != ESP_OK) {
    end();
    return false;
  }
  esp_lcd_dpi_panel_config_t dpi = {};
  dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_PLL_F160M;
  dpi.dpi_clock_freq_mhz = timing.pixelClockMHz;
  dpi.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
  dpi.in_color_format = LCD_COLOR_FMT_RGB888;
  dpi.out_color_format = LCD_COLOR_FMT_RGB888;
  dpi.num_fbs = buffers;
  dpi.video_timing.h_size = timing.width;
  dpi.video_timing.v_size = timing.height;
  dpi.video_timing.hsync_back_porch = timing.hBackPorch;
  dpi.video_timing.hsync_pulse_width = timing.hSync;
  dpi.video_timing.hsync_front_porch = timing.hFrontPorch;
  dpi.video_timing.vsync_back_porch = timing.vBackPorch;
  dpi.video_timing.vsync_pulse_width = timing.vSync;
  dpi.video_timing.vsync_front_porch = timing.vFrontPorch;
  dpi.flags.disable_lp = true;
  if (esp_lcd_new_panel_dpi(_bus, &dpi, &_panel) != ESP_OK) {
    end();
    return false;
  }
  // Match the vendor sequence: program the bridge, then start DPI scanout.
  if (!bridge.configure(timing, 2) || esp_lcd_panel_init(_panel) != ESP_OK) {
    end();
    return false;
  }
  esp_err_t result;
  if (buffers == 1) {
    result = esp_lcd_dpi_panel_get_frame_buffer(_panel, 1, (void**)&_pixels[0]);
  } else if (buffers == 2) {
    result = esp_lcd_dpi_panel_get_frame_buffer(_panel, 2, (void**)&_pixels[0],
                                                (void**)&_pixels[1]);
  } else {
    result = esp_lcd_dpi_panel_get_frame_buffer(_panel, 3, (void**)&_pixels[0],
                                                (void**)&_pixels[1],
                                                (void**)&_pixels[2]);
  }
  if (result != ESP_OK) {
    end();
    return false;
  }
  _width = timing.width;
  _height = timing.height;
  _buffers = buffers;
  return true;
}

/** Stop output and release only this object's DSI, panel, and PHY resources.
 * The caller's Wire bus and bridge object remain available.
 */
void Adafruit_LT8912B_P4::end() {
  if (_bridge) {
    _bridge->enableOutput(false);
  }
  if (_panel) {
    esp_lcd_panel_del(_panel);
    _panel = nullptr;
  }
  if (_bus) {
    esp_lcd_del_dsi_bus(_bus);
    _bus = nullptr;
  }
  if (_power) {
    esp_ldo_release_channel(_power);
    _power = nullptr;
  }
  for (uint8_t index = 0; index < 3; index++) {
    _pixels[index] = nullptr;
  }
  _buffers = 0;
  _bridge = nullptr;
}

/** Get a panel-owned framebuffer, valid until end().
 * @param index Framebuffer index, less than the begin() buffer count.
 * @return BGR byte triplets in row order, or nullptr if unavailable.
 * @note With multiple buffers, the caller must coordinate scanout completion
 * before reusing a displayed buffer; show() does not wait for vertical blank.
 */
uint8_t* Adafruit_LT8912B_P4::getFrameBuffer(uint8_t index) {
  if (index >= _buffers) {
    return nullptr;
  }
  return _pixels[index];
}

/** Flush CPU writes and submit a panel-owned framebuffer to the DPI driver.
 * @param index Framebuffer index, less than the begin() buffer count.
 * @return True when the IDF driver accepts the buffer, not when scanout ends.
 */
bool Adafruit_LT8912B_P4::show(uint8_t index) {
  uint8_t* pixels = getFrameBuffer(index);
  if (!pixels) {
    return false;
  }
  return esp_lcd_panel_draw_bitmap(_panel, 0, 0, _width, _height, pixels) ==
         ESP_OK;
}

/** Access the borrowed ESP-IDF panel for renderers and frame callbacks.
 * @return Panel handle, or nullptr before begin() or after end(). Do not delete
 * the handle; this object owns it. Stop external renderers before end().
 */
esp_lcd_panel_handle_t Adafruit_LT8912B_P4::getPanel() {
  return _panel;
}
#endif
