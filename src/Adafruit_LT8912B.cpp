/*!
 * @file Adafruit_LT8912B.cpp
 * @brief Arduino/BusIO adaptation of Espressif's LT8912B video driver.
 *
 * SPDX-FileCopyrightText: 2023-2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2026 Adafruit Industries
 * SPDX-License-Identifier: Apache-2.0
 *
 * Modified by Adafruit: portable I2C control, checked transactions and timing
 * arguments, configurable lane count, optional physical reset, DVI default,
 * and disabled audio. Original: esp_lcd_lt8912b in espressif/esp-bsp.
 */

#include "Adafruit_LT8912B.h"

#include <math.h>

/** @brief Create an uninitialized bridge controller. */
Adafruit_LT8912B::Adafruit_LT8912B() {}

/** @brief Release the owned I2C interfaces without changing the bridge output.
 */
Adafruit_LT8912B::~Adafruit_LT8912B() {
  delete _main;
  delete _dsi;
  delete _avi;
}

/**
 * @brief Initialize I2C control and optionally perform a physical reset.
 * @param wire I2C bus, configured for the board's control pins by the sketch.
 * @param resetPin Active-low reset GPIO, or 0xFFFF to leave reset to the
 * caller.
 * @return True when all three fixed addresses (0x48, 0x49, 0x4A) acknowledge.
 * @note This checks acknowledgements, not silicon identity: no chip-ID check is
 * implemented. Disconnect the adapter's DDC switch before calling this method;
 * a monitor's DDC/ISP devices can collide with the bridge register addresses.
 */
bool Adafruit_LT8912B::begin(TwoWire* wire, uint16_t resetPin) {
  _initialized = false;
  if (!wire) {
    return false;
  }

  delete _main;
  delete _dsi;
  delete _avi;
  // The chip exposes main, DSI/CEC, and AVI/audio banks at fixed I2C addresses.
  _main = new Adafruit_I2CDevice(0x48, wire);
  _dsi = new Adafruit_I2CDevice(0x49, wire);
  _avi = new Adafruit_I2CDevice(0x4A, wire);
  if (!_main || !_dsi || !_avi) {
    return false;
  }

  _resetPin = resetPin;
  if (_resetPin != 0xFFFF && !reset()) {
    return false;
  }
  if (!_main->begin() || !_dsi->begin() || !_avi->begin()) {
    return false;
  }
  _initialized = true;
  return true;
}

/**
 * @brief Pulse the physical reset pin for 10 ms and wait another 10 ms.
 * @return False if no reset pin was supplied to begin(); otherwise true.
 * @note Reset discards configuration. Call configure() again afterward. GPIO
 * writes cannot report whether the physical reset reached the bridge.
 */
bool Adafruit_LT8912B::reset() {
  if (_resetPin == 0xFFFF) {
    return false;
  }
  digitalWrite(_resetPin, LOW);
  pinMode(_resetPin, OUTPUT);
  delay(10);
  digitalWrite(_resetPin, HIGH);
  delay(10);
  return true;
}

/**
 * @brief Write an ordered vendor register sequence, stopping on the first
 * error.
 * @param device Register-bank interface.
 * @param sequence Register/value pairs, in the required order.
 * @param count Number of pairs.
 * @return True if every write succeeded.
 */
bool Adafruit_LT8912B::writeSequence(Adafruit_I2CDevice* device,
                                     const RegisterValue* sequence,
                                     size_t count) {
  for (size_t index = 0; index < count; index++) {
    Adafruit_BusIO_Register reg(device, sequence[index].address);
    if (!reg.write(sequence[index].value)) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Configure RGB888 DSI reception and enable video-only DVI output.
 * @param timing Source timing. Active dimensions must be nonzero; sync widths
 * are 1 to 255. Horizontal and vertical totals must each fit in 16 bits.
 * pixelClockMHz must be finite and positive, VIC 0 to 127, and aspectRatio 0
 * to 2.
 * @param lanes Number of DSI data lanes, 1 to 4, without lane or P/N swapping.
 * @return True on success; false for invalid settings, missing begin(), or I2C
 * failure. A partial I2C failure may require reset() before retrying.
 * @note This configures the bridge only. The DSI source must generate matching
 * timing and pixel clock; pixelClockMHz does not program the source clock here.
 * The defaults retain Espressif's 25 MHz reference-crystal initialization.
 */
bool Adafruit_LT8912B::configure(const LT8912B_Timing& timing, uint8_t lanes) {
  uint32_t hTotal = (uint32_t)timing.width + timing.hFrontPorch + timing.hSync +
                    timing.hBackPorch;
  uint32_t vTotal = (uint32_t)timing.height + timing.vFrontPorch +
                    timing.vSync + timing.vBackPorch;
  if (!_initialized || lanes < 1 || lanes > 4 || !timing.width ||
      !timing.height || !timing.hSync || timing.hSync > 255 || !timing.vSync ||
      timing.vSync > 255 || hTotal > 65535 || vTotal > 65535 ||
      !isfinite(timing.pixelClockMHz) || timing.pixelClockMHz <= 0 ||
      timing.vic > 127 || timing.aspectRatio > 2) {
    return false;
  }

  // Stop the transmitter while changing timing to avoid displaying a partially
  // updated mode. This is a driver coherence safeguard, not a reset sequence.
  if (!enableOutput(false)) {
    return false;
  }

  // These ordered analog/PLL/DDS tables are retained from Espressif's driver.
  // Their complete initialization bytes include vendor tuning fields without
  // public bit definitions; keep the original values and comments together.
  const RegisterValue digitalClock[] = {
      /* Digital clock en */
      {0x02, 0xF7}, {0x08, 0xFF}, {0x09, 0xFF},
      {0x0A, 0xFF}, {0x0B, 0x7C}, {0x0C, 0xFF},
  };
  const RegisterValue txAnalog[] = {
      /* Tx Analog */
      {0x31, 0xE1}, {0x32, 0xE1}, {0x33, 0x0C},
      {0x37, 0x00}, {0x38, 0x22}, {0x60, 0x82},
  };
  const RegisterValue cbusAnalog[] = {
      /* Cbus Analog */
      {0x39, 0x45},
      {0x3A, 0x00},
      {0x3B, 0x00},
  };
  const RegisterValue hdmiPLL[] = {
      /* HDMI PLL Analog */
      {0x44, 0x31},
      {0x55, 0x44},
      {0x57, 0x01},
      {0x5A, 0x02},
  };
  const RegisterValue mipiAnalog[] = {
      {0x3E, 0xD6}, // No MIPI P/N swap.
      {0x3F, 0xD4}, // EQ.
      {0x41, 0x3C}, // MIPI analog control.
  };
  uint8_t laneCode = lanes;
  if (lanes == 4) {
    laneCode = 0; // Vendor encoding: 00=4, 01=1, 02=2, 03=3 lanes.
  }
  const RegisterValue mipiBasic[] = {
      {0x10, 0x01},                        // Termination enable.
      {0x11, 0x10},                        // Settle.
      {DSI_LANES, laneCode}, {0x14, 0x00}, // Debug mux.
      {0x15, 0x00},                        // No lane swap.
      {0x1A, 0x03},                        // H shift 3.
      {0x1B, 0x03},                        // V shift 3.
  };
  const RegisterValue ddsConfig[] = {
      {0x4E, 0x93}, // strm_sw_freq_word[7:0]
      {0x4F, 0x3E}, // strm_sw_freq_word[15:8]
      {0x50, 0x29}, // strm_sw_freq_word[23:16]
      {0x51, 0x80}, // [0]=strm_sw_freq_word[24]
      {0x1E, 0x4F},
      {0x1F, 0x5E}, // full_value
      {0x20, 0x01},
      {0x21, 0x2C}, // full_value1
      {0x22, 0x01},
      {0x23, 0xFA}, // full_value2
      {0x24, 0x00},
      {0x25, 0xC8}, // full_value3
      {0x26, 0x00},
      {0x27, 0x5E}, // empty_value
      {0x28, 0x01},
      {0x29, 0x2C}, // empty_value1
      {0x2A, 0x01},
      {0x2B, 0xFA}, // empty_value2
      {0x2C, 0x00},
      {0x2D, 0xC8}, // empty_value3
      {0x2E, 0x00},
      {0x42, 0x64}, // tmr_set[7:0]:100 us
      {0x43, 0x00}, // tmr_set[15:8]
      {0x44, 0x04}, // Timer step and following DDS tuning coefficients.
      {0x45, 0x00},
      {0x46, 0x59},
      {0x47, 0x00},
      {0x48, 0xF2},
      {0x49, 0x06},
      {0x4A, 0x00},
      {0x4B, 0x72},
      {0x4C, 0x45},
      {0x4D, 0x00},
      {0x52, 0x08}, // Trend step and following DDS tuning
                    // coefficients.
      {0x53, 0x00},
      {0x54, 0xB2},
      {0x55, 0x00},
      {0x56, 0xE4},
      {0x57, 0x0D},
      {0x58, 0x00},
      {0x59, 0xE4},
      {0x5A, 0x8A},
      {0x5B, 0x00},
      {0x5C, 0x34},
      {0x51, 0x00}, // Release DDS control.
  };
  if (!writeSequence(_main, digitalClock,
                     sizeof(digitalClock) / sizeof(RegisterValue)) ||
      !writeSequence(_main, txAnalog,
                     sizeof(txAnalog) / sizeof(RegisterValue)) ||
      !writeSequence(_main, cbusAnalog,
                     sizeof(cbusAnalog) / sizeof(RegisterValue)) ||
      !writeSequence(_main, hdmiPLL, sizeof(hdmiPLL) / sizeof(RegisterValue)) ||
      !writeSequence(_main, mipiAnalog,
                     sizeof(mipiAnalog) / sizeof(RegisterValue)) ||
      !writeSequence(_dsi, mipiBasic,
                     sizeof(mipiBasic) / sizeof(RegisterValue)) ||
      !writeSequence(_dsi, ddsConfig,
                     sizeof(ddsConfig) / sizeof(RegisterValue)) ||
      !writeTiming(timing)) {
    return false;
  }

  // Sync polarity occupies only bits 1:0. Preserve the other reset fields.
  Adafruit_BusIO_Register sync(_main, SYNC_POLARITY);
  Adafruit_BusIO_RegisterBits horizontalPositive(&sync, 1, 1);
  Adafruit_BusIO_RegisterBits verticalPositive(&sync, 1, 0);
  if (!horizontalPositive.write(timing.hSyncPositive) ||
      !verticalPositive.write(timing.vSyncPositive)) {
    return false;
  }

  // AVI header is 82 02 0D. PB1=10 selects RGB with active-format information;
  // PB2 has the picture aspect in bits 5:4 and active aspect "same as picture".
  // PB3 and PB5..PB13 are zero, so checksum=0x5F-PB2-PB4, modulo 256.
  uint8_t aspect = timing.aspectRatio * 16 + 0x08;
  uint8_t checksum = (uint8_t)(0x5F - aspect - timing.vic);
  uint8_t payload[14] = {checksum, 0x10, aspect, 0, timing.vic};
  for (uint8_t index = 0; index < sizeof(payload); index++) {
    Adafruit_BusIO_Register infoframe(_avi, AVI_CHECKSUM + index);
    if (!infoframe.write(payload[index])) {
      return false;
    }
  }
  if (!resetReceiver()) {
    return false;
  }

  const RegisterValue lvdsBypass[] = {
      {0x44, 0x30},               // LVDS power up for bypass configuration.
      {0x51, 0x05}, {0x50, 0x24}, // CP=50 uA.
      {0x51, 0x2D}, // Pixel clock reference, second-order passive LPF PLL.
      {0x52, 0x04}, // loopdiv=0; use second-order PLL.
      {0x69, 0x0E}, // CP_PRESET_DIV_RATIO.
      {0x69, 0x8E}, {0x6A, 0x00},
      {0x6C, 0xB8},               // RGD_CP_SOFT_K_EN, RGD_CP_SOFT_K[13:8].
      {0x6B, 0x51}, {0x04, 0xFB}, // Core PLL reset.
      {0x04, 0xFF}, {0x7F, 0x00}, // Disable scaler.
      {0xA8, 0x13},               // VESA format (0x33 would select JEIDA).
      {0x44, 0x31}, // Disable LVDS output; use HDMI/DVI transmitter only.
  };
  // Video-only change: keep audio acquisition and packets disabled. These are
  // the LT8912B audio shutdown values, not Espressif's default I2S enable
  // table.
  const RegisterValue audioOff[] = {
      {0x06, 0x00}, // Stop audio acquisition.
      {0x07, 0x00}, // Disable audio clock regeneration.
      {0x34, 0x52}, // Disable I2S input.
      {0x3C, 0x40}, // Disable audio/null packet insertion.
  };
  if (!writeSequence(_main, lvdsBypass,
                     sizeof(lvdsBypass) / sizeof(RegisterValue)) ||
      !writeSequence(_avi, audioOff,
                     sizeof(audioOff) / sizeof(RegisterValue)) ||
      !setHDMI(false)) {
    return false;
  }
  return enableOutput(true);
}

/**
 * @brief Write timing as individual bytes, matching the vendor transaction
 * order.
 * @param timing Already-validated video timing.
 * @return True if every register write succeeded.
 */
bool Adafruit_LT8912B::writeTiming(const LT8912B_Timing& timing) {
  uint16_t hTotal =
      timing.width + timing.hFrontPorch + timing.hSync + timing.hBackPorch;
  uint16_t vTotal =
      timing.height + timing.vFrontPorch + timing.vSync + timing.vBackPorch;
  const RegisterValue values[] = {
      {H_SYNC, (uint8_t)timing.hSync},
      {V_SYNC, (uint8_t)timing.vSync},
      {H_ACTIVE, (uint8_t)(timing.width % 256)},
      {H_ACTIVE + 1, (uint8_t)(timing.width / 256)},
      {0x2F, 0x0C}, // FIFO buffer length 12.
      {H_TOTAL, (uint8_t)(hTotal % 256)},
      {H_TOTAL + 1, (uint8_t)(hTotal / 256)},
      {V_TOTAL, (uint8_t)(vTotal % 256)},
      {V_TOTAL + 1, (uint8_t)(vTotal / 256)},
      {V_BACK_PORCH, (uint8_t)(timing.vBackPorch % 256)},
      {V_BACK_PORCH + 1, (uint8_t)(timing.vBackPorch / 256)},
      {V_FRONT_PORCH, (uint8_t)(timing.vFrontPorch % 256)},
      {V_FRONT_PORCH + 1, (uint8_t)(timing.vFrontPorch / 256)},
      {H_BACK_PORCH, (uint8_t)(timing.hBackPorch % 256)},
      {H_BACK_PORCH + 1, (uint8_t)(timing.hBackPorch / 256)},
      {H_FRONT_PORCH, (uint8_t)(timing.hFrontPorch % 256)},
      {H_FRONT_PORCH + 1, (uint8_t)(timing.hFrontPorch / 256)},
  };
  return writeSequence(_dsi, values, sizeof(values) / sizeof(RegisterValue));
}

/** @brief Reset the MIPI receiver and DDS logic with the vendor 10 ms delays.
 * @return True if all four writes succeeded.
 */
bool Adafruit_LT8912B::resetReceiver() {
  Adafruit_BusIO_Register mipiReset(_main, MIPI_RESET);
  // Vendor full-register reset values also retain the other released blocks.
  if (!mipiReset.write(0x7F)) {
    return false;
  }
  delay(10);
  if (!mipiReset.write(0xFF)) {
    return false;
  }
  Adafruit_BusIO_Register ddsReset(_main, DDS_RESET);
  if (!ddsReset.write(0xFB)) {
    return false;
  }
  delay(10);
  return ddsReset.write(0xFF);
}

/**
 * @brief Select HDMI signaling or DVI signaling without enabling audio.
 * @param enabled True selects HDMI; false selects DVI.
 * @return True if the mode field was written successfully.
 */
bool Adafruit_LT8912B::setHDMI(bool enabled) {
  if (!_initialized) {
    return false;
  }
  Adafruit_BusIO_Register mode(_main, HDMI_MODE);
  Adafruit_BusIO_RegisterBits hdmiEnabled(&mode, 1, 0);
  return hdmiEnabled.write(enabled);
}

/**
 * @brief Enable or disable the HDMI/DVI transmitter without changing its mode.
 * @param enabled True to transmit video; false to stop the transmitter.
 * @return True if the transmitter enable field was written successfully.
 */
bool Adafruit_LT8912B::enableOutput(bool enabled) {
  if (!_initialized) {
    return false;
  }
  // Vendor enable/disable values are 0x0E/0x0C; only bit 1 changes.
  Adafruit_BusIO_Register transmitter(_main, TX_CONTROL);
  Adafruit_BusIO_RegisterBits outputEnabled(&transmitter, 1, 1);
  return outputEnabled.write(enabled);
}

/**
 * @brief Read HDMI hot-plug detect from the bridge.
 * @return 1 if connected, 0 if disconnected, or -1 before begin()/on I2C error.
 */
int8_t Adafruit_LT8912B::getHPD() {
  if (!_initialized) {
    return -1;
  }
  // RegisterBits::read() cannot distinguish a failed read from HPD=1. Use a
  // checked byte snapshot for this status decision and return an explicit
  // error.
  HotPlugStatus status = {};
  Adafruit_BusIO_Register hotPlug(_main, HPD_STATUS);
  if (!hotPlug.read(&status.raw)) {
    return -1;
  }
  return status.bits.connected;
}
