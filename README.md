# Adafruit LT8912B

Arduino support for the LT8912B MIPI DSI to HDMI/DVI bridge. The bridge is
configured over I2C; video arrives through its separate MIPI DSI input.
Sending pixels over I2C is not supported by this chip.

The portable driver uses Wire and Adafruit BusIO. The `Adafruit_LT8912B_P4` helper creates
the DSI display and exposes its ESP-IDF panel handle for drawing and separate
video integration. Other processors may use the bridge driver with their own
DSI host implementation.

This is an initial **0.1.0 review snapshot**. The originating Metro P4 sketches
have produced working video on hardware; that does not establish hardware
validation of this library port. HDMI audio remains unresolved and is not
included in this version.

## Getting started

Install this folder as an Arduino library, together with **Adafruit BusIO**,
**Adafruit GFX Library**, and **Adafruit TestBed**. The P4 examples require an
ESP32-P4 board with PSRAM. They compile with Arduino-ESP32 3.3.11 and the
current 4.0.x source core. The helper uses
the legacy PHY clock selection tested on the original P4 silicon.

Select **Adafruit Metro ESP32-P4** in the Arduino board menu. These examples
require the updated Metro board definition, whose `pins_arduino.h` supplies
`PIN_DSI_RESET` and the shared default I2C pins. Update an older prototype board
definition before compiling; no manual SDA/SCL pin setup is needed in the sketches.
The board definition is proposed in
[Arduino-ESP32 #12980](https://github.com/espressif/arduino-esp32/pull/12980).

- `bridge_status`: portable I2C connection and bridge status example.
- `p4_colorbars`: ESP32-P4 framebuffer output with a static test pattern.
- `p4_graphics`: ESP32-P4 output using an Adafruit GFX canvas.

The P4 examples use the Metro P4 board definition and the previously tested
800 x 480 video timing. Check the board definition and timing configuration
before using another board or monitor. Monitor acceptance of a timing or sync
polarity must be tested; this library does not select modes from EDID.

Keep the Adafruit adapter's **DDC switch OFF**, with no QT cable bypassing it,
while controlling the bridge. Connecting the monitor DDC bus to the control
bus can introduce conflicting I2C devices. The bridge control addresses are
0x48, 0x49, and 0x4A.

## Video playback

SD access and video decoding belong in a separate player. The existing
[Metro P4 SD video sketch](https://github.com/adafruit/MBAdafruitBoards/tree/master/Development/Video/LT8912B%20DSI%20DVI/Arduino/MetroP4DviVideo)
is a reference for that integration. Its player code is GPL-2.0-only and is
not copied into this Apache-2.0 library. That demonstration plays silent
MJPEG video; MP4 soundtrack decoding is not supplied here.

## Build coverage

The Adafruit CI workflow builds all examples on its `metro_esp32p4` target,
and checks formatting and standard Doxygen documentation. This target uses
the source core while the new board definition awaits upstream inclusion.
It temporarily uses the CI fork containing
[ci-arduino #234](https://github.com/adafruit/ci-arduino/pull/234).

Local checks on 2026-10-07: all three examples compiled for the new Metro P4
board definition on the Arduino-ESP32 4.0.x source core. `p4_colorbars` also
compiled with Arduino-ESP32 3.3.11. Formatting checks passed, and Doxygen
1.8.13 using the standard CI template passed with no warnings. The new library
has not been uploaded to the board; the paused audio test is unchanged.

## Credits and license

The bridge initialization and register programming are adapted from
[Espressif's esp_lcd_lt8912b driver](https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_lt8912b).
Copyright notices are retained, and this library uses the same
**Apache License, Version 2.0**. See [license.txt](license.txt) for the complete
license and [NOTICE](NOTICE) for source provenance and the changes made for
Arduino. Dependencies retain their own licenses.
