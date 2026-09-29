# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

PlatformIO/Arduino firmware for a Waveshare 7.5" e-Paper panel (800x480, black/white, `EPD_7in5_V2` driver) on a Waveshare e-Paper ESP32 Driver Board (`board = esp32dev`). Two pages, Calendar (month grid + agenda) and Bin Collection (hero "next collection" panel + following days; bins are Blue, Black, Red, Food, Garden). Each boot draws one page, sleeps the panel, then deep-sleeps the ESP32; the BOOT button (GPIO0) wakes it and advances the page (kept in `RTC_DATA_ATTR`), RESET restarts at the calendar. Both pages show placeholder data; there is no WiFi or live feed yet (the planned bin source is a shared Google Calendar ICS link, bins recognised by event title). The button wake has not been tested on hardware.

`README.md` is a short overview; details live in `docs/` (`hardware-setup.md`, `troubleshooting.md`, `development.md`). Keep them in step when behaviour, serial output or build envs change. Work happens directly on the `claude/esp32-epaper-platformio-r136dm` branch (the remote's only branch and HEAD); no PRs.

## Commands

- Build / flash / monitor: `pio run`, `pio run -t upload`, `pio device monitor` (115200 baud). Default env is `esp32`.
- Unit tests (host, no board): `pio test -e native`. Run one suite with `-f test_calendar_date` or `-f test_bin_schedule`.
- Preview a page as a PNG without hardware: `bash tools/preview.sh [calendar|bins] [out.png]` (default `preview-<page>.png`; needs `g++` and zlib). Look at the PNG after any layout change.

Dev setup on this ARM64 Windows machine: PlatformIO Core is at `~/.platformio/penv/Scripts` and MSYS2's clangarm64 toolchain (`g++` is clang++) is at `C:\msys64\clangarm64\bin`; both are on the user PATH. A shell started before that change won't see them, so call `pio` by full path or refresh `$env:Path` from the registry. Run `tools/preview.sh` with Git Bash. No ESP32 is normally attached, so upload and serial monitor can't be exercised here.

Optional `build_flags` (commented in `platformio.ini`): `-D D_9PIN=1` (panel power gated through GPIO33; try this if BUSY never releases) and `-D EPD_BUSY_TIMEOUT_MS=<ms>` (default 20000).

## Architecture

Dependencies point one way: `src/main.cpp` → `lib/CalendarRender` / `lib/BinRender` → `lib/UiKit` → `lib/WaveshareEPD`; everything that decides *what* to show lives in `lib/CalendarCore`.

- `lib/CalendarCore/`: no Arduino or display dependencies, so it is unit tested natively. `calendar_data.h` (`CalView`/`CalEvent`: times are minutes from midnight, already time-zone resolved; `CAL_ALL_DAY` (-1) marks all-day events; `nowMin = -1` hides the clock; `events` must be sorted by date then time; limits `CAL_MAX_EVENTS` 64, `CAL_TITLE_LEN` 64), `calendar_date` (civil-date arithmetic and month/weekday names), `bin_data.h` (`BinView`/`BinCollection`, `BinType` enum whose order is the display order) and `bin_schedule` (`BinSchedule_Build()` sorts a flat, unordered list into per-day `BinGroup`s, merging same-day bins and dropping past dates; plus the label/advice text). Put decision logic here so it can be tested.
- `lib/UiKit/`: `Ui_*` text helpers (fit, right/centre align, integer-scaled text for headlines the fixed fonts are too small for).
- `lib/CalendarRender/`, `lib/BinRender/`: `CalendarRender_Draw()` / `BinRender_Draw()` clear and draw onto whatever `GUI_Paint` canvas is selected (caller runs `Paint_NewImage`/`Paint_SelectImage` first). Layout is fixed pixel constants at the top of each .cpp. `bin_icon.cpp` draws the bin pictograms from GUI_Paint primitives (no bitmaps); the panel has no colour, so bins are told apart by icon marks plus their printed name.
- `src/dummy_data.cpp`: `DummyData_Fill()` / `DummyBinData_Fill()` supply a fixed "today" (2026-08-24) so renders are reproducible. A live source replaces these.
- `src/main.cpp`: hardware glue only (init, malloc frame buffer, render the current page, display, sleep, deep-sleep on the button).

`[env:native]` uses `lib_ignore` to drop `CalendarRender`, `BinRender`, `UiKit` and `WaveshareEPD`, and `[env:esp32]` sets `test_ignore = *`. Tests therefore only cover code that builds without the Arduino framework, and new tests go in `test/test_<name>/`.

`tools/preview.sh` compiles the real renderers and `GUI_Paint` with desktop g++, using stand-ins for `Arduino.h`/`Wire.h`/`SPI.h` in `tools/host/` (needed because vendored `DEV_Config.h` includes them). A new renderer library must be added to the compile line in the script and a page case to `tools/host/host_render.cpp`; a new Arduino API used by shared code needs a stub in `tools/host/Arduino.h`. Adding a page also means a `Page` value and `drawPage()` case in `main.cpp` and a `lib_ignore` entry.

## Vendored driver: `lib/WaveshareEPD/`

Copied from Waveshare's e-Paper repo (provenance table and pin map are in its README). It is locally modified, so don't overwrite it from upstream. Marked `LOCAL MODIFICATION` in source:
- `EPD_7in5_V2.cpp`: bounded busy-wait with diagnostics instead of an infinite spin.
- `DEV_Config.h`: `D_9PIN` wrapped in `#ifndef` so it can be set from `platformio.ini`.
- `GUI_Paint.cpp`: `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime` had their foreground/background arguments swapped upstream. **Waveshare demo snippets pasted here need their two colour arguments exchanged.**

Gotcha: `EPD_7IN5_V2_Display()` inverts the buffer in place, so redraw the frame buffer before reusing it rather than assuming it survived.
