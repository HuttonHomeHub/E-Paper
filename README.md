# E-Paper calendar — Waveshare 7.5" (800×480) on an ESP32 driver board

Firmware that draws a calendar (month grid plus an agenda of upcoming events)
on a Waveshare 7.5" e-Paper panel, driven by a Waveshare e-Paper ESP32 Driver
Board. It currently shows placeholder data; there is no WiFi or live calendar
feed yet.

| | |
| --- | --- |
| **Display** | Waveshare 7.5inch e-Paper raw display, 800×480, black/white (SKU 13187, panel `WF0583CZ09`) |
| **Controller board** | Waveshare e-Paper ESP32 Driver Board (SKU 15823, ESP32-WROOM-32) |
| **Driver** | `EPD_7in5_V2`, vendored from Waveshare into [`lib/WaveshareEPD/`](lib/WaveshareEPD/) |
| **Framework** | Arduino, via PlatformIO |

The screen is drawn once at boot and the panel is then put to sleep. E-paper
holds its image with no power, so **press RESET on the board to redraw.**

## Quick start

1. Install PlatformIO and the USB serial driver, and connect the panel:
   [docs/hardware-setup.md](docs/hardware-setup.md).
2. Build, flash and watch the log:

   ```
   pio run -t upload
   pio device monitor
   ```

No board yet? Preview the screen on your computer and run the tests:

```
tools/preview.sh calendar.png     # renders the exact screen the ESP32 draws
pio test -e native                # unit tests for the date arithmetic
```

## Documentation

* [docs/hardware-setup.md](docs/hardware-setup.md) — install, wiring, pin mapping, first flash
* [docs/troubleshooting.md](docs/troubleshooting.md) — blank screen, BUSY timeouts, upload failures
* [docs/development.md](docs/development.md) — code layout, data model, testing, drawing API

## Credits

Driver code is Waveshare's, from <https://github.com/waveshareteam/e-Paper>,
MIT licensed. See [`lib/WaveshareEPD/README.md`](lib/WaveshareEPD/README.md) for
the file-by-file provenance.
