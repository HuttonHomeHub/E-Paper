# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

PlatformIO/Arduino firmware for a Waveshare 7.5" e-Paper panel (800x480, black/white, `EPD_7in5_V2` driver) on a Waveshare e-Paper ESP32 Driver Board (`board = esp32dev`). Two pages, Calendar (month grid + agenda) and Bin Collection (hero "next collection" panel + following days; bins are Blue, Black, Red, Food, Garden). Each boot draws one page, sleeps the panel, then deep-sleeps the ESP32. Wake sources: the BOOT button (GPIO0) advances the page (kept in `RTC_DATA_ATTR`); a timer for 00:05 local redraws the same page when the dates roll over (only set once the clock is known, i.e. after the bin page's NTP sync); a quiet retry timer (1h, 2h, 4h, then hourly; `RTC_DATA_ATTR gRetryStep`/`gWakeIsRetry`) used only while the bin dates are stale, which re-runs the download WITHOUT initialising the panel and redraws only on success; RESET restarts at the calendar. **Design rule: pages show dates, never the time of day**, because they are only redrawn at midnight or on a button press. The Calendar page shows placeholder data. The Bin Collection page loads the council's ReCollect `.ics` feed over WiFi when `include/secrets.h` (gitignored; copy `include/secrets.example.h`) is filled in, else placeholder data under a black "SAMPLE DATA" header banner (one banner at a time: out-of-date > sample > "CALENDAR ENDS SOON/ENDED" when the feed's last date is within 14 days). A failed download is retried once, then the last good copy (saved to flash on every success) is shown under an "OUT OF DATE - last updated ..." banner; only with no saved copy or no known date does it show a plain error message. Verified on the real board on 29 Sep 2026 (COM3, CH343): panel output, deep sleep, BOOT wake, WiFi join, NTP, and the HTTPS download + parse of the live feed (10039 bytes, 59 entries, certificate check passed). Still unverified on hardware: the midnight timer wake, the "OUT OF DATE" fallback to the saved copy, the quiet retry, and the ends-soon banner. Bench-test the retry with `PLATFORMIO_BUILD_FLAGS=-DREFRESH_RETRY_UNIT_SECONDS=30 pio run -t upload` (retries every 30 s instead of hours); reflash a normal build afterwards.

`README.md` is a short overview; details live in `docs/` (`hardware-setup.md`, `troubleshooting.md`, `development.md`). Keep them in step when behaviour, serial output or build envs change. Work happens directly on the `claude/esp32-epaper-platformio-r136dm` branch (the remote's only branch and HEAD); no PRs.

## Commands

- Build / flash / monitor: `pio run`, `pio run -t upload`, `pio device monitor` (115200 baud). Default env is `esp32`.
- Unit tests (host, no board): `pio test -e native`. Run one suite with `-f <name>`: `test_calendar_date`, `test_bin_schedule`, `test_ics_bins`, `test_refresh_cache`.
- Layout regression: `bash tools/golden.sh` (compares every screen scenario with `test/golden/*.png`, pixel for pixel; `--update` after an intended change, then look at the images and commit them). Runs in CI.
- Preview a screen as a PNG without hardware: `bash tools/preview.sh [scenario] [out.png]` (default scenario `calendar`; `--list` shows all; default output `preview-<scenario>.png`; needs `g++` and zlib). Look at the PNG after any layout change.

Dev setup on this ARM64 Windows machine: PlatformIO Core is at `~/.platformio/penv/Scripts` and MSYS2's clangarm64 toolchain (`g++` is clang++) is at `C:\msys64\clangarm64\bin`; both are on the user PATH. A shell started before that change won't see them, so call `pio` by full path or refresh `$env:Path` from the registry. Run `tools/preview.sh` with Git Bash. An ESP32 may or may not be attached; when it is, it shows up as a USB device (`pio device list`; it was COM3 with ID 1A86:55D3) and can be flashed with `pio run -t upload --upload-port COM3`. To read its log without the interactive monitor, use PlatformIO's Python with pyserial (`~/.platformio/penv/Scripts/python.exe`): opening the port resets the board via DTR/RTS, so a capture starts from a fresh boot, and a BOOT press is needed to reach the bin page. `include/secrets.h` already exists on this machine (filled in, gitignored), so builds here compile the live-feed path. Tooling quirk: the Bash tool has rejected large inline heredocs (e.g. `python - <<EOF` with big bodies); write the script to a file and run that instead.

Optional `build_flags` (commented in `platformio.ini`): `-D D_9PIN=1` (panel power gated through GPIO33; try this if BUSY never releases) and `-D EPD_BUSY_TIMEOUT_MS=<ms>` (default 20000).

## Architecture

Dependencies point one way: `src/main.cpp` → `lib/CalendarRender` / `lib/BinRender` → `lib/UiKit` → `lib/WaveshareEPD`; everything that decides *what* to show lives in `lib/CalendarCore`.

- `lib/CalendarCore/`: no Arduino or display dependencies, so it is unit tested natively. `calendar_data.h` (`CalView`/`CalEvent`: times are minutes from midnight, already time-zone resolved; `CAL_ALL_DAY` (-1) marks all-day events; `events` must be sorted by date then time; limits `CAL_MAX_EVENTS` 64, `CAL_TITLE_LEN` 64), `calendar_date` (civil-date arithmetic and month/weekday names), `bin_data.h` (`BinView`/`BinCollection`, `BinType` enum whose order is the display order; `BinView.warning` is the header banner text, NULL when the data is fine), `bin_schedule` (`BinSchedule_CalendarEnd()` gives the ends-soon banner text; `BinSchedule_Build()` sorts a flat, unordered list into per-day `BinGroup`s, merging same-day bins and dropping past dates; plus the label/advice text) `ics_bins` (`IcsBins_Parse()`; `IcsBins_FromSummary()` is the keyword table mapping event wording to bins), `refresh_schedule` (seconds until the 00:05 refresh, and `Refresh_NextWake()` choosing between a retry and midnight; `-DREFRESH_RETRY_UNIT_SECONDS` overrides the hour) and `bin_cache` (32-bit packing of a collection for the flash cache, rejecting corrupt entries). Put decision logic here so it can be tested.
- `lib/UiKit/`: `Ui_*` text helpers (fit, right/centre align, integer-scaled text for headlines the fixed fonts are too small for).
- `lib/CalendarRender/`, `lib/BinRender/`: `CalendarRender_Draw()` / `BinRender_Draw()` clear and draw onto whatever `GUI_Paint` canvas is selected (caller runs `Paint_NewImage`/`Paint_SelectImage` first). Layout is fixed pixel constants at the top of each .cpp. `bin_icon.cpp` draws the bin pictograms from GUI_Paint primitives (no bitmaps); the panel has no colour, so each bin is an icon showing its contents (Blue bottle+can = glass & cans, Black sack = general, Red box = cardboard, Food caddy, Garden sprig) plus its printed name; the hero also captions each with `Bin_Contents()`.
- `lib/UkClock/`: Arduino-only. `UkClock_ApplyTimezone()` must run every boot (TZ is lost in deep sleep; the system clock is not), NTP sync, `UkClock_SecondsUntilRefresh()`.
- `lib/BinFeed/`: Arduino-only. WiFi + HTTPS (CA bundle, follows redirects, 2 attempts) download feeding `IcsBins_Parse()`, saved to flash (`Preferences`, namespace `binfeed`) on success and used as the stale fallback on failure. `main.cpp` runs it BEFORE allocating the 48 KB frame buffer and before the panel is initialised (a TLS handshake needs a large contiguous heap block; heap is logged before/after). `BinFeed_LastLoadWasFresh()` tells fresh from the saved copy.
- `src/dummy_data.cpp`: `DummyData_Fill()` / `DummyBinData_Fill()` supply a fixed "today" (2026-08-24) so renders are reproducible. The calendar page has no live source yet; the bin page falls back to `DummyBinData_Fill()` only when `secrets.h` is not configured.
- `src/main.cpp`: hardware glue only (init, malloc frame buffer, render the current page, display, sleep, deep-sleep on the button).

`[env:native]` uses `lib_ignore` to drop `CalendarRender`, `BinRender`, `BinFeed`, `UkClock`, `UiKit` and `WaveshareEPD`, and `[env:esp32]` sets `test_ignore = *`. Tests therefore only cover code that builds without the Arduino framework, and new tests go in `test/test_<name>/`.

`tools/host/build.sh` (used by `preview.sh` and `golden.sh`) compiles the real renderers and `GUI_Paint` with desktop g++, using stand-ins for `Arduino.h`/`Wire.h`/`SPI.h` in `tools/host/` (needed because vendored `DEV_Config.h` includes them). `tools/host/host_render.cpp` defines the named scenarios (screen + fixed data) and the pixel-compare `--check` mode; the compare decodes the reference PNG instead of comparing file bytes, so it doesn't depend on the zlib version. A new renderer library must be added to `build.sh` and scenarios to `host_render.cpp`, then `tools/golden.sh --update`; a new Arduino API used by shared code needs a stub in `tools/host/Arduino.h`. Adding a page also means a `Page` value and `drawPage()` case in `main.cpp` and a `lib_ignore` entry.

## Bin feed facts and gotchas

- The feed URL identifies the property and the feed embeds the home address in its calendar name, so never commit the URL, `secrets.h`, or a raw feed (tests use a hand-made fixture). Real feed shape: one all-day event per collection day, ~27 events / 10 KB / ~5 months, `Cache-Control: max-age=43200`; the first URL 301-redirects to another host, so redirects must be followed. Titles describe contents ("Food waste, plastic recycling (blue-lid bin...), and refuse..."), so one event can mean several bins. The household names the bins Blue = glass & cans, Black = general, Red = cardboard, while the feed says "plastic recycling (blue-lid bin...)", "refuse" and "paper and card (red-lid bin...)"; the keyword table accepts both wordings.
- `platformio.ini` adds `-Iinclude` because PlatformIO only puts `include/` on the path for `src/`, not for libs. Without it `lib/BinFeed` silently compiles its "not configured" stub even when `secrets.h` exists. After touching feed code, check the real path is linked, e.g. `nm .pio/build/esp32/firmware.elf | grep -c HTTPClient` (nonzero) or flash near 76%, not 23%.
- A saved copy is only usable when today's date is known (clock synced since power-up, or still running from an earlier sync through deep sleep); otherwise only the plain error message can be shown.

- WiFi names are case-sensitive (`HuttonHomeHub`). `BinFeed` logs the connection status and any similar-named networks it can see when a join fails, and the error shown is the true root cause (WiFi vs clock vs HTTP).

## CI and versions

`.github/workflows/ci.yml`: on every push/PR it runs `pio test -e native`, `tools/golden.sh` (uploads `golden-actual/` on failure), builds without `secrets.h`, then builds with `secrets.example.h` copied to `secrets.h` and greps the ELF for `HTTPClient` (guards the `-Iinclude` trap above). A monthly scheduled job (and manual dispatch) strips the platform pins and builds against the newest platform; red means don't upgrade yet. `platformio.ini` pins `espressif32 @ 7.0.1` and `native @ 1.2.1`; bump them deliberately (`pio pkg outdated`). The workflow itself can only be exercised on GitHub, but every command in it was run locally.

## Flash layout

`platformio.ini` sets `board_build.partitions = huge_app.csv` (one 3 MB app slot, no OTA; firmware ~1 MB = 32%). NVS is at 0x9000 in both this and the default layout, so the saved bin dates survive a switch. No wireless updates by design; if ever wanted, restore the default layout and add signed images plus rollback first.

## Vendored driver: `lib/WaveshareEPD/`

Copied from Waveshare's e-Paper repo (provenance table and pin map are in its README). It is locally modified, so don't overwrite it from upstream. Marked `LOCAL MODIFICATION` in source:
- `EPD_7in5_V2.cpp`: bounded busy-wait with diagnostics instead of an infinite spin.
- `DEV_Config.h`: `D_9PIN` wrapped in `#ifndef` so it can be set from `platformio.ini`.
- `GUI_Paint.cpp`: `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime` had their foreground/background arguments swapped upstream. **Waveshare demo snippets pasted here need their two colour arguments exchanged.**

Gotcha: `EPD_7IN5_V2_Display()` inverts the buffer in place, so redraw the frame buffer before reusing it rather than assuming it survived.
