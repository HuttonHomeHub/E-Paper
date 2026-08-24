# WaveshareEPD

Vendored (copied in, not fetched at build time) from Waveshare's official e-Paper
repository: <https://github.com/waveshareteam/e-Paper>, MIT licensed.

There is no Waveshare library in the PlatformIO/Arduino library registry for this
panel, so the driver sources live in the repo. That also means the pin mapping and
panel driver are pinned and readable rather than hidden behind a dependency.

## Where each file came from

| File(s) | Upstream path |
| --- | --- |
| `DEV_Config.h`, `DEV_Config.cpp` | `E-paper_Separate_Program/1in54_e-Paper_G/ESP32/` |
| `Debug.h` | `E-paper_Separate_Program/1in54_e-Paper_G/ESP32/` |
| `GUI_Paint.h`, `GUI_Paint.cpp` | `E-paper_Separate_Program/1in54_e-Paper_G/ESP32/` |
| `fonts.h`, `font8/12/16/20/24.cpp` | `E-paper_Separate_Program/1in54_e-Paper_G/ESP32/` |
| `EPD_7in5_V2.h`, `EPD_7in5_V2.cpp` | `RaspberryPi_JetsonNano/c/lib/e-Paper/EPD_7in5_V2.{h,c}` |

`DEV_Config` / `GUI_Paint` / the fonts are taken from Waveshare's ESP32 program
because those are the ESP32-specific parts (Arduino `pinMode`/`digitalWrite`, the
driver board's GPIO numbers).

The 7.5" panel driver itself is Waveshare's portable C driver. It only calls the
`DEV_*` interface that `DEV_Config` provides, so it drops straight in. The only
change made was renaming `.c` to `.cpp` so PlatformIO compiles it alongside the
Arduino C++ sources.

## Pin mapping

Set in `DEV_Config.h`, and matching the Waveshare e-Paper ESP32 Driver Board:

| Signal | ESP32 GPIO |
| --- | --- |
| SCK / CLK | 13 |
| MOSI / DIN | 14 |
| CS | 15 |
| RST | 26 |
| DC | 27 |
| BUSY | 25 |
| PWR (9-pin panels only, off by default) | 33 |

## Note on `EPD_7IN5_V2_Display()`

It inverts the buffer you hand it, in place, as part of writing the panel's second
data channel. If you want to reuse a frame buffer across refreshes, redraw it
rather than assuming it survived the call.

## Local modifications to the vendored code

Two deliberate changes from upstream, both marked `LOCAL MODIFICATION` in the source:

* **`EPD_7in5_V2.cpp` — bounded busy-wait.** Upstream's `EPD_WaitUntilIdle()`
  spins forever if the panel never releases BUSY, so a badly seated ribbon cable
  presents as a silent hang. It now times out after `EPD_BUSY_TIMEOUT_MS`
  (default 20s) and prints what to check.
* **`DEV_Config.h` — `D_9PIN` is overridable.** Wrapped in `#ifndef` so panel
  power gating on GPIO33 can be enabled with `-D D_9PIN=1` from `platformio.ini`
  rather than by editing the vendored file.
* **`GUI_Paint.cpp` — swapped colour arguments.** Upstream
  `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime` each passed
  `(Color_Background, Color_Foreground)` down to `Paint_DrawChar`, the reverse
  of their own declared parameter order, so black-on-white came out as white
  text on a black block. `Paint_DrawChar` and `Paint_DrawString_CN` did not
  have the swap, so the library disagreed with itself. All three call sites now
  pass the pair in the declared order.

  **If you paste a Waveshare demo snippet into this project, exchange its two
  colour arguments** — their examples are written against the swapped
  behaviour.
