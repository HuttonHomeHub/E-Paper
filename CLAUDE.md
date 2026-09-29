# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

PlatformIO/Arduino firmware that draws a calendar (month grid + agenda) on a Waveshare 7.5" e-Paper panel (800x480, black/white, `EPD_7in5_V2` driver) via a Waveshare e-Paper ESP32 Driver Board (`board = esp32dev`). The screen is drawn once in `setup()`, the panel is slept, and `loop()` idles; the image persists unpowered and RESET redraws it. It shows placeholder data; there is no WiFi or live feed yet.

`README.md` is a short overview; details live in `docs/` (`hardware-setup.md`, `troubleshooting.md`, `development.md`). Keep them in step when behaviour, serial output or build envs change. Work happens directly on the `claude/esp32-epaper-platformio-r136dm` branch (the remote's only branch and HEAD); no PRs.

## Commands

- Build / flash / monitor: `pio run`, `pio run -t upload`, `pio device monitor` (115200 baud). Default env is `esp32`.
- Unit tests (host, no board): `pio test -e native`. Run one suite with `-f test_calendar_date`.
- Preview the screen as a PNG without hardware: `bash tools/preview.sh [out.png]` (default `calendar.png`; needs `g++` and zlib).

Dev setup on this ARM64 Windows machine: PlatformIO Core is at `~/.platformio/penv/Scripts` and MSYS2's clangarm64 toolchain (`g++` is clang++) is at `C:\msys64\clangarm64\bin`; both are on the user PATH. A shell started before that change won't see them, so call `pio` by full path or refresh `$env:Path` from the registry. Run `tools/preview.sh` with Git Bash. No ESP32 is normally attached, so upload and serial monitor can't be exercised here.

Optional `build_flags` (commented in `platformio.ini`): `-D D_9PIN=1` (panel power gated through GPIO33; try this if BUSY never releases) and `-D EPD_BUSY_TIMEOUT_MS=<ms>` (default 20000).

## Architecture

Dependencies point one way: `src/main.cpp` → `lib/CalendarRender` → `lib/CalendarCore` and `lib/WaveshareEPD`.

- `lib/CalendarCore/`: `calendar_data.h` (plain-C model `CalView`/`CalEvent`) and `calendar_date` (civil-date arithmetic). No Arduino or display dependencies, so it is unit tested natively. Times are minutes from midnight, already time-zone resolved; `CAL_ALL_DAY` (-1) marks all-day events; `nowMin = -1` hides the clock; `events` must be sorted by date then time; limits `CAL_MAX_EVENTS` 64, `CAL_TITLE_LEN` 64.
- `lib/CalendarRender/`: `CalendarRender_Draw()` clears and draws onto whatever `GUI_Paint` canvas is selected (caller runs `Paint_NewImage`/`Paint_SelectImage` first). Layout is fixed pixel constants at the top of the .cpp.
- `src/dummy_data.cpp`: `DummyData_Fill()` supplies a fixed "today" (2026-08-24) so renders are reproducible. A live source replaces this function.
- `src/main.cpp`: hardware glue only (init, malloc frame buffer, render, `EPD_7IN5_V2_Display`, sleep).

`[env:native]` uses `lib_ignore` to drop `CalendarRender` and `WaveshareEPD`, and `[env:esp32]` sets `test_ignore = *`. Tests therefore only cover code that builds without the Arduino framework, and new tests go in `test/test_<name>/`.

`tools/preview.sh` compiles the real renderer and `GUI_Paint` with desktop g++, using stand-ins for `Arduino.h`/`Wire.h`/`SPI.h` in `tools/host/` (needed because vendored `DEV_Config.h` includes them). A new renderer source file must be added to the compile line in the script; a new Arduino API used by shared code needs a stub in `tools/host/Arduino.h`.

## Vendored driver: `lib/WaveshareEPD/`

Copied from Waveshare's e-Paper repo (provenance table and pin map are in its README). It is locally modified, so don't overwrite it from upstream. Marked `LOCAL MODIFICATION` in source:
- `EPD_7in5_V2.cpp`: bounded busy-wait with diagnostics instead of an infinite spin.
- `DEV_Config.h`: `D_9PIN` wrapped in `#ifndef` so it can be set from `platformio.ini`.
- `GUI_Paint.cpp`: `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime` had their foreground/background arguments swapped upstream. **Waveshare demo snippets pasted here need their two colour arguments exchanged.**

Gotcha: `EPD_7IN5_V2_Display()` inverts the buffer in place, so redraw the frame buffer before reusing it rather than assuming it survived.
