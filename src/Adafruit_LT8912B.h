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

/** Fixed I2C addresses of the LT8912B register banks. */
#define LT8912B_I2C_MAIN 0x48 ///< MAIN bank I2C address.
#define LT8912B_I2C_DSI 0x49  ///< DSI bank I2C address.
#define LT8912B_I2C_AVI 0x4A  ///< AVI bank I2C address.

/** MAIN bank register addresses. Vendor tuning names follow Espressif. */
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_02 0x02 ///< MAIN bank digital clock 02.
#define LT8912B_REG_MAIN_MIPI_RESET 0x03       ///< MAIN bank mipi reset.
#define LT8912B_REG_MAIN_CORE_PLL_RESET 0x04   ///< MAIN bank core pll reset.
#define LT8912B_REG_MAIN_DDS_RESET 0x05        ///< MAIN bank dds reset.
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_08 0x08 ///< MAIN bank digital clock 08.
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_09 0x09 ///< MAIN bank digital clock 09.
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_0A 0x0A ///< MAIN bank digital clock 0a.
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_0B 0x0B ///< MAIN bank digital clock 0b.
#define LT8912B_REG_MAIN_DIGITAL_CLOCK_0C 0x0C ///< MAIN bank digital clock 0c.
#define LT8912B_REG_MAIN_TX_ANALOG_31 0x31     ///< MAIN bank tx analog 31.
#define LT8912B_REG_MAIN_TX_ANALOG_32 0x32     ///< MAIN bank tx analog 32.
#define LT8912B_REG_MAIN_TX_CONTROL 0x33       ///< MAIN bank tx control.
#define LT8912B_REG_MAIN_TX_ANALOG_37 0x37     ///< MAIN bank tx analog 37.
#define LT8912B_REG_MAIN_TX_ANALOG_38 0x38     ///< MAIN bank tx analog 38.
#define LT8912B_REG_MAIN_CBUS_ANALOG_39 0x39   ///< MAIN bank cbus analog 39.
#define LT8912B_REG_MAIN_CBUS_ANALOG_3A 0x3A   ///< MAIN bank cbus analog 3a.
#define LT8912B_REG_MAIN_CBUS_ANALOG_3B 0x3B   ///< MAIN bank cbus analog 3b.
#define LT8912B_REG_MAIN_MIPI_PN_CONTROL 0x3E  ///< MAIN bank mipi pn control.
#define LT8912B_REG_MAIN_MIPI_EQ 0x3F          ///< MAIN bank mipi eq.
#define LT8912B_REG_MAIN_MIPI_ANALOG_CONTROL \
  0x41 ///< MAIN bank mipi analog control.
#define LT8912B_REG_MAIN_LVDS_PLL_CONTROL 0x44 ///< MAIN bank lvds pll control.
#define LT8912B_REG_MAIN_LVDS_PLL_CURRENT 0x50 ///< MAIN bank lvds pll current.
#define LT8912B_REG_MAIN_LVDS_PLL_REFERENCE \
  0x51                                      ///< MAIN bank lvds pll reference.
#define LT8912B_REG_MAIN_LVDS_PLL_LOOP 0x52 ///< MAIN bank lvds pll loop.
#define LT8912B_REG_MAIN_HDMI_PLL_ANALOG_55 \
  0x55 ///< MAIN bank hdmi pll analog 55.
#define LT8912B_REG_MAIN_HDMI_PLL_ANALOG_57 \
  0x57 ///< MAIN bank hdmi pll analog 57.
#define LT8912B_REG_MAIN_HDMI_PLL_ANALOG_5A \
  0x5A                                       ///< MAIN bank hdmi pll analog 5a.
#define LT8912B_REG_MAIN_TX_ANALOG_60 0x60   ///< MAIN bank tx analog 60.
#define LT8912B_REG_MAIN_LVDS_CP_PRESET 0x69 ///< MAIN bank lvds cp preset.
#define LT8912B_REG_MAIN_LVDS_CP_6A 0x6A     ///< MAIN bank lvds cp 6a.
#define LT8912B_REG_MAIN_LVDS_CP_SOFT_K_LOW \
  0x6B ///< MAIN bank lvds cp soft k low.
#define LT8912B_REG_MAIN_LVDS_CP_SOFT_K_HIGH \
  0x6C                                       ///< MAIN bank lvds cp soft k high.
#define LT8912B_REG_MAIN_SCALER_CONTROL 0x7F ///< MAIN bank scaler control.
#define LT8912B_REG_MAIN_LVDS_FORMAT 0xA8    ///< MAIN bank lvds format.
#define LT8912B_REG_MAIN_SYNC_POLARITY 0xAB  ///< MAIN bank sync polarity.
#define LT8912B_REG_MAIN_HDMI_MODE 0xB2      ///< MAIN bank hdmi mode.
#define LT8912B_REG_MAIN_HPD_STATUS 0xC1     ///< MAIN bank hpd status.

/** DSI bank register addresses. Vendor tuning names follow Espressif. */
#define LT8912B_REG_DSI_TERMINATION 0x10       ///< DSI bank termination.
#define LT8912B_REG_DSI_SETTLE 0x11            ///< DSI bank settle.
#define LT8912B_REG_DSI_LANES 0x13             ///< DSI bank lanes.
#define LT8912B_REG_DSI_DEBUG_MUX 0x14         ///< DSI bank debug mux.
#define LT8912B_REG_DSI_LANE_SWAP 0x15         ///< DSI bank lane swap.
#define LT8912B_REG_DSI_H_SYNC 0x18            ///< DSI bank h sync.
#define LT8912B_REG_DSI_V_SYNC 0x19            ///< DSI bank v sync.
#define LT8912B_REG_DSI_H_SHIFT 0x1A           ///< DSI bank h shift.
#define LT8912B_REG_DSI_V_SHIFT 0x1B           ///< DSI bank v shift.
#define LT8912B_REG_DSI_H_ACTIVE 0x1C          ///< DSI bank h active.
#define LT8912B_REG_DSI_DDS_CONTROL_1E 0x1E    ///< DSI bank dds control 1e.
#define LT8912B_REG_DSI_FIFO_FULL_0_LOW 0x1F   ///< DSI bank fifo full 0 low.
#define LT8912B_REG_DSI_FIFO_FULL_0_HIGH 0x20  ///< DSI bank fifo full 0 high.
#define LT8912B_REG_DSI_FIFO_FULL_1_LOW 0x21   ///< DSI bank fifo full 1 low.
#define LT8912B_REG_DSI_FIFO_FULL_1_HIGH 0x22  ///< DSI bank fifo full 1 high.
#define LT8912B_REG_DSI_FIFO_FULL_2_LOW 0x23   ///< DSI bank fifo full 2 low.
#define LT8912B_REG_DSI_FIFO_FULL_2_HIGH 0x24  ///< DSI bank fifo full 2 high.
#define LT8912B_REG_DSI_FIFO_FULL_3_LOW 0x25   ///< DSI bank fifo full 3 low.
#define LT8912B_REG_DSI_FIFO_FULL_3_HIGH 0x26  ///< DSI bank fifo full 3 high.
#define LT8912B_REG_DSI_FIFO_EMPTY_0_LOW 0x27  ///< DSI bank fifo empty 0 low.
#define LT8912B_REG_DSI_FIFO_EMPTY_0_HIGH 0x28 ///< DSI bank fifo empty 0 high.
#define LT8912B_REG_DSI_FIFO_EMPTY_1_LOW 0x29  ///< DSI bank fifo empty 1 low.
#define LT8912B_REG_DSI_FIFO_EMPTY_1_HIGH 0x2A ///< DSI bank fifo empty 1 high.
#define LT8912B_REG_DSI_FIFO_EMPTY_2_LOW 0x2B  ///< DSI bank fifo empty 2 low.
#define LT8912B_REG_DSI_FIFO_EMPTY_2_HIGH 0x2C ///< DSI bank fifo empty 2 high.
#define LT8912B_REG_DSI_FIFO_EMPTY_3_LOW 0x2D  ///< DSI bank fifo empty 3 low.
#define LT8912B_REG_DSI_FIFO_EMPTY_3_HIGH 0x2E ///< DSI bank fifo empty 3 high.
#define LT8912B_REG_DSI_FIFO_BUFFER_LENGTH \
  0x2F                                      ///< DSI bank fifo buffer length.
#define LT8912B_REG_DSI_H_TOTAL 0x34        ///< DSI bank h total.
#define LT8912B_REG_DSI_V_TOTAL 0x36        ///< DSI bank v total.
#define LT8912B_REG_DSI_V_BACK_PORCH 0x38   ///< DSI bank v back porch.
#define LT8912B_REG_DSI_V_FRONT_PORCH 0x3A  ///< DSI bank v front porch.
#define LT8912B_REG_DSI_H_BACK_PORCH 0x3C   ///< DSI bank h back porch.
#define LT8912B_REG_DSI_H_FRONT_PORCH 0x3E  ///< DSI bank h front porch.
#define LT8912B_REG_DSI_DDS_TIMER_LOW 0x42  ///< DSI bank dds timer low.
#define LT8912B_REG_DSI_DDS_TIMER_HIGH 0x43 ///< DSI bank dds timer high.
#define LT8912B_REG_DSI_DDS_TIMER_STEP 0x44 ///< DSI bank dds timer step.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_45 \
  0x45 ///< DSI bank dds timer coefficient 45.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_46 \
  0x46 ///< DSI bank dds timer coefficient 46.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_47 \
  0x47 ///< DSI bank dds timer coefficient 47.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_48 \
  0x48 ///< DSI bank dds timer coefficient 48.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_49 \
  0x49 ///< DSI bank dds timer coefficient 49.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_4A \
  0x4A ///< DSI bank dds timer coefficient 4a.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_4B \
  0x4B ///< DSI bank dds timer coefficient 4b.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_4C \
  0x4C ///< DSI bank dds timer coefficient 4c.
#define LT8912B_REG_DSI_DDS_TIMER_COEFFICIENT_4D \
  0x4D ///< DSI bank dds timer coefficient 4d.
#define LT8912B_REG_DSI_DDS_FREQUENCY_LOW 0x4E ///< DSI bank dds frequency low.
#define LT8912B_REG_DSI_DDS_FREQUENCY_MIDDLE \
  0x4F ///< DSI bank dds frequency middle.
#define LT8912B_REG_DSI_DDS_FREQUENCY_HIGH \
  0x50                                      ///< DSI bank dds frequency high.
#define LT8912B_REG_DSI_DDS_CONTROL_51 0x51 ///< DSI bank dds control 51.
#define LT8912B_REG_DSI_DDS_TREND_STEP 0x52 ///< DSI bank dds trend step.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_53 \
  0x53 ///< DSI bank dds trend coefficient 53.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_54 \
  0x54 ///< DSI bank dds trend coefficient 54.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_55 \
  0x55 ///< DSI bank dds trend coefficient 55.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_56 \
  0x56 ///< DSI bank dds trend coefficient 56.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_57 \
  0x57 ///< DSI bank dds trend coefficient 57.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_58 \
  0x58 ///< DSI bank dds trend coefficient 58.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_59 \
  0x59 ///< DSI bank dds trend coefficient 59.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_5A \
  0x5A ///< DSI bank dds trend coefficient 5a.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_5B \
  0x5B ///< DSI bank dds trend coefficient 5b.
#define LT8912B_REG_DSI_DDS_TREND_COEFFICIENT_5C \
  0x5C ///< DSI bank dds trend coefficient 5c.

/** AVI bank register addresses. Vendor tuning names follow Espressif. */
#define LT8912B_REG_AVI_AUDIO_ACQUISITION 0x06 ///< AVI bank audio acquisition.
#define LT8912B_REG_AVI_AUDIO_CLOCK 0x07       ///< AVI bank audio clock.
#define LT8912B_REG_AVI_I2S_CONTROL 0x34       ///< AVI bank i2s control.
#define LT8912B_REG_AVI_PACKET_CONTROL 0x3C    ///< AVI bank packet control.
#define LT8912B_REG_AVI_CHECKSUM 0x43          ///< AVI bank checksum.

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

  bool begin(int16_t resetPin = -1, TwoWire* wire = &Wire);
  bool reset();
  bool configure(const LT8912B_Timing& timing, uint8_t lanes = 2);
  bool setHDMI(bool enabled);
  bool enableOutput(bool enabled);
  bool getHPD();

 private:
  /** @brief One ordered vendor initialization write. */
  struct RegisterValue {
    uint8_t address; ///< Register address within the selected bank.
    uint8_t value;   ///< Complete vendor initialization value.
  };

  bool writeSequence(Adafruit_I2CDevice* device, const RegisterValue* sequence,
                     size_t count);
  bool writeTiming(const LT8912B_Timing& timing);
  bool resetReceiver();

  Adafruit_I2CDevice* _main = nullptr; ///< Owned main register-bank device.
  Adafruit_I2CDevice* _dsi = nullptr;  ///< Owned DSI register-bank device.
  Adafruit_I2CDevice* _avi = nullptr; ///< Owned AVI/audio register-bank device.
  int16_t _resetPin = -1;    ///< Active-low reset, or no connected pin.
  bool _initialized = false; ///< All three register banks acknowledged.
};

#endif
