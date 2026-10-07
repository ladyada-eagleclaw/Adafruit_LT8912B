// SPDX-FileCopyrightText: 2026 Adafruit Industries
// SPDX-License-Identifier: Apache-2.0

#include <Adafruit_GFX.h>
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
LT8912B_Timing timing; // Defaults to 800 x 480.
GFXcanvas1 canvas(800, 480);

void halt(const char *message);

void setup() {
  Serial.begin(115200);
  delay(250); // Allow serial startup without waiting for a USB connection.

  Serial.println("Adafruit LT8912B P4 graphics test");

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
  if (!canvas.getBuffer()) {
    halt("Could not allocate the GFX canvas.");
  }

  canvas.fillScreen(0);
  canvas.drawRoundRect(16, 16, 768, 448, 20, 1);
  canvas.setTextColor(1);
  canvas.setTextSize(4);
  canvas.setCursor(48, 56);
  canvas.println("Adafruit LT8912B");
  canvas.setTextSize(2);
  canvas.setCursor(48, 112);
  canvas.println("ESP32-P4 DSI + Adafruit GFX");
  canvas.fillCircle(160, 280, 75, 1);
  canvas.drawCircle(400, 280, 75, 1);
  canvas.fillTriangle(560, 355, 640, 205, 720, 355, 1);

  uint8_t *pixels = display.getFrameBuffer();
  if (!pixels) {
    halt("No framebuffer available.");
  }
  // Convert the one-bit canvas to the panel's blue, green, red byte order.
  // Render only once; repeated writes into an active single buffer can tear.
  for (uint16_t y = 0; y < timing.height; y++) {
    for (uint16_t x = 0; x < timing.width; x++) {
      bool lit = canvas.getPixel(x, y);
      if (lit) {
        *pixels++ = 255;
        *pixels++ = 220;
        *pixels++ = 80;
      } else {
        *pixels++ = 24;
        *pixels++ = 12;
        *pixels++ = 8;
      }
    }
  }
  if (!display.show()) {
    halt("Could not display the framebuffer.");
  }
  Serial.println("Static GFX canvas displayed in DVI mode.");
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
