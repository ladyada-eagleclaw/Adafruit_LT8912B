// SPDX-FileCopyrightText: 2026 Adafruit Industries
// SPDX-License-Identifier: Apache-2.0

#include <Adafruit_LT8912B.h>

Adafruit_LT8912B bridge;

void setup() {
  Serial.begin(115200);
  delay(250); // Allow serial to start without requiring a connected computer.

  Serial.println("Adafruit LT8912B bridge status test");

  // Connect the bridge's I2C control bus to this board's default SDA/SCL.
  // Keep the adapter DDC switch OFF. No reset pin is used in this example;
  // the bridge must already be powered and released from reset.
  if (!bridge.begin()) {
    Serial.println("LT8912B not found. Check power, reset, SDA and SCL.");
    while (true) {
      delay(10);
    }
  }
  Serial.println("LT8912B found. This example checks status without a DSI host.");
}

void loop() {
  if (bridge.getHPD()) {
    Serial.println("HDMI cable connected.");
  } else {
    Serial.println("HDMI cable disconnected.");
  }
  delay(1000);
}
