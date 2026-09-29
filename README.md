# E-Paper display — Waveshare 7.5" (800×480) on an ESP32 driver board

Firmware for a Waveshare 7.5" e-Paper panel, driven by a Waveshare e-Paper ESP32
Driver Board. It has two pages:

* **Calendar** — a month grid with an agenda of upcoming events.
* **Bin Collection** — which bins go out next (Blue, Black, Red, Food, Garden),
  with the following collection days listed beside it.

The Bin Collection page reads the council's calendar feed over WiFi once you
add your details ([docs/development.md](docs/development.md#live-bin-data));
until then, and for the Calendar page, placeholder data is shown.

| | |
| --- | --- |
| **Display** | Waveshare 7.5inch e-Paper raw display, 800×480, black/white (SKU 13187, panel `WF0583CZ09`) |
| **Controller board** | Waveshare e-Paper ESP32 Driver Board (SKU 15823, ESP32-WROOM-32) |
| **Driver** | `EPD_7in5_V2`, vendored from Waveshare into [`lib/WaveshareEPD/`](lib/WaveshareEPD/) |
| **Framework** | Arduino, via PlatformIO |

One page is drawn per boot, then the board goes into deep sleep (e-paper holds
its image with no power). **Press BOOT to show the next page; press RESET to
start again from the calendar.**

## Quick start

1. Install PlatformIO and the USB serial driver, and connect the panel:
   [docs/hardware-setup.md](docs/hardware-setup.md).
2. Optional, for live bin dates: copy `include/secrets.example.h` to
   `include/secrets.h` and fill it in
   ([details](docs/development.md#live-bin-data)).
3. Build, flash and watch the log:

   ```
   pio run -t upload
   pio device monitor
   ```

No board yet? Preview the screen on your computer and run the tests:

```
tools/preview.sh bins             # renders the exact screen the ESP32 draws
                                  # (or: calendar) to preview-<page>.png
pio test -e native                # unit tests for the date, bin-schedule and feed-parsing logic
```

## Documentation

* [docs/hardware-setup.md](docs/hardware-setup.md) — install, wiring, pin mapping, first flash
* [docs/troubleshooting.md](docs/troubleshooting.md) — blank screen, BUSY timeouts, upload failures
* [docs/development.md](docs/development.md) — code layout, data model, testing, drawing API

## Credits

Driver code is Waveshare's, from <https://github.com/waveshareteam/e-Paper>,
MIT licensed. See [`lib/WaveshareEPD/README.md`](lib/WaveshareEPD/README.md) for
the file-by-file provenance.
