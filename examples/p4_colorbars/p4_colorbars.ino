// SPDX-FileCopyrightText: 2026 Adafruit Industries
// SPDX-License-Identifier: Apache-2.0

#include <Adafruit_LT8912B_P4.h>
#include <Adafruit_TestBed.h>

#if !defined(CONFIG_IDF_TARGET_ESP32P4)
#error "This example requires an ESP32-P4 board with PSRAM."
#endif
#if !defined(PIN_DSI_RESET)
#error "Select Adafruit Metro ESP32-P4 and install its updated board definition."
#endif

// The Metro P4 board definition supplies the DSI reset and default I2C pins.
const uint8_t RESET_PIN = PIN_DSI_RESET;
#define MONITOR_EDID_ADDRESS 0x50
Adafruit_TestBed testbed;
Adafruit_LT8912B bridge;
Adafruit_LT8912B_P4 display;
LT8912B_Timing timing; // Previously tested 800 x 480 mode; check your monitor.

void halt(const char *message);

void setup() {
  Serial.begin(115200);
  delay(250); // Allow serial startup without waiting for a USB connection.

  Serial.println("Adafruit LT8912B P4 color bars test");

  // Keep the bridge in reset while checking for an accidentally joined DDC bus.
  digitalWrite(RESET_PIN, LOW);
  pinMode(RESET_PIN, OUTPUT);
  if (testbed.scanI2CBus(MONITOR_EDID_ADDRESS)) {
    halt("DDC detected: turn the adapter DDC switch OFF and remove any QT bypass.");
  }
  if (!bridge.begin(RESET_PIN)) {
    halt("LT8912B not found.");
  }

  // Monitor-specific alternative tested on LT-32X575: AB polarity bits 0x02.
  // timing.hSyncPositive = true;
  if (!display.begin(bridge, timing, 1)) {
    halt("Display startup failed. Check PSRAM and the DSI connection.");
  }

  // Panel framebuffer bytes are blue, green, red, in that order.
  const uint8_t bars[8][3] = {
    {255, 255, 255}, {0, 255, 255}, {255, 255, 0}, {0, 255, 0},
    {255, 0, 255},   {0, 0, 255},   {255, 0, 0},   {0, 0, 0}
  };
  uint8_t *pixels = display.getFrameBuffer();
  if (!pixels) {
    halt("No framebuffer available.");
  }
  for (uint16_t y = 0; y < timing.height; y++) {
    for (uint16_t x = 0; x < timing.width; x++) {
      uint8_t bar = (uint32_t)x * 8 / timing.width;
      *pixels++ = bars[bar][0];
      *pixels++ = bars[bar][1];
      *pixels++ = bars[bar][2];
    }
  }
  if (!display.show()) {
    halt("Could not display the framebuffer.");
  }
  Serial.println("Static color bars displayed in DVI mode.");
}

void loop() {
  delay(100);
}

void halt(const char *message) {
  Serial.println(message);
  while (true) {
    delay(10);
  }
}
