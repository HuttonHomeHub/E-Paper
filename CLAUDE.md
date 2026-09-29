# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

PlatformIO/Arduino firmware for a Waveshare 7.5" e-Paper panel (800x480, black/white, `EPD_7in5_V2` driver) on a Waveshare e-Paper ESP32 Driver Board (`board = esp32dev`). Each build draws one screen in `setup()`, sleeps the panel, and idles in `loop()`; the image persists unpowered and RESET redraws it. The only screen so far is a calendar (month grid + agenda) showing placeholder data; there is no WiFi or live feed yet.

`README.md` is a beginner guide (setup, wiring, troubleshooting, calendar preview); keep it in step when behaviour, serial output or build envs change. Entry points are `src/main_calendar.cpp` and `src/main_hello.cpp`. Work happens directly on the `claude/esp32-epaper-platformio-r136dm` branch (the remote's only branch and HEAD); no PRs.

## Commands

- Build / flash / monitor: `pio run`, `pio run -t upload`, `pio device monitor` (115200 baud). Default env is `calendar`; add `-e hello` for the wiring-test page.
- Preview on the host without hardware: `tools/render.sh [out.png]` (default `calendar.png`).
- Date-maths checks: `tools/test.sh` (one program, `tools/date_test.cpp`; no way to run a single case).

The two `tools/` scripts are bash and need `g++` and zlib (PNG output). Dev setup on this ARM64 Windows machine: PlatformIO Core is at `~/.platformio/penv/Scripts` and MSYS2's clangarm64 toolchain (`g++` is clang++) is at `C:\msys64\clangarm64\bin`; both are on the user PATH. A shell started before that change won't see them, so call `pio` by full path or refresh `$env:Path` from the registry. Run the scripts with Git Bash (`bash tools/test.sh`). No ESP32 is normally attached, so upload and serial monitor can't be exercised here.

Optional `build_flags` (commented in `platformio.ini`): `-D D_9PIN=1` (panel power gated through GPIO33; try this if BUSY never releases) and `-D EPD_BUSY_TIMEOUT_MS=<ms>` (default 20000).

## Architecture

**Two envs share `src/`** through `build_src_filter`: `calendar` excludes `main_hello.cpp`; `hello` excludes `main_calendar.cpp`, `calendar_render.cpp` and `dummy_data.cpp`. Any new `src/` file must be added to the right filters or it lands in both builds.

**The calendar is layered so drawing has no hardware dependency:**
- `calendar_data.h`: plain-C model (`CalView`, `CalEvent`). Times are minutes from midnight, already time-zone resolved; `CAL_ALL_DAY` (-1) marks all-day events; `nowMin = -1` hides the clock. `events` must be sorted by date then time. Limits: `CAL_MAX_EVENTS` 64, `CAL_TITLE_LEN` 64.
- `calendar_render.cpp`: `CalendarRender_Draw()` clears and draws onto whatever `GUI_Paint` canvas is selected (caller runs `Paint_NewImage`/`Paint_SelectImage` first). Layout is fixed pixel constants at the top of the file. Also exports the `Cal_*` date helpers.
- `dummy_data.cpp`: `DummyData_Fill()` supplies a fixed "today" (2026-08-24) so renders are reproducible. A live source replaces this function.
- `main_calendar.cpp`: hardware glue only (init, malloc frame buffer, render, `EPD_7IN5_V2_Display`, sleep).

**Host harness:** `tools/render.sh` and `tools/test.sh` compile the real renderer and `GUI_Paint` with desktop g++. `tools/arduino_stub.h` is copied to a temp dir as `Arduino.h`/`Wire.h`/`SPI.h` to satisfy `DEV_Config.h`. When adding a renderer dependency, add the file to the g++ lines in both scripts; when shared code uses a new Arduino API, stub it in `arduino_stub.h`.

## Vendored driver: `lib/WaveshareEPD/`

Copied from Waveshare's e-Paper repo (provenance table and pin map are in its README). It is locally modified, so don't overwrite it from upstream. Marked `LOCAL MODIFICATION` in source:
- `EPD_7in5_V2.cpp`: bounded busy-wait with diagnostics instead of an infinite spin.
- `DEV_Config.h`: `D_9PIN` wrapped in `#ifndef` so it can be set from `platformio.ini`.
- `GUI_Paint.cpp`: `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime` had their foreground/background arguments swapped upstream. **Waveshare demo snippets pasted here need their two colour arguments exchanged.**

Gotcha: `EPD_7IN5_V2_Display()` inverts the buffer in place, so redraw the frame buffer before reusing it rather than assuming it survived.
