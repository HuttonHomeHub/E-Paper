# Development

## Layout

```
platformio.ini             envs: esp32 (firmware, default) and native (unit tests)
src/                       main.cpp (page switching, panel I/O) + placeholder data
lib/CalendarCore/          data models and date/bin-schedule logic (no hardware deps)
lib/UiKit/                 text helpers shared by the screens (fit, centre, scaled text)
lib/CalendarRender/        the calendar screen
lib/BinRender/             the bin collection screen and its bin icons
lib/WaveshareEPD/          vendored Waveshare driver (locally modified, see its README)
test/                      Unity tests: test_calendar_date, test_bin_schedule
tools/                     host-side preview: preview.sh + stubs in tools/host/
docs/                      hardware setup, troubleshooting, this file
```

Dependencies point one way: `src/main.cpp` → `CalendarRender` / `BinRender` →
`UiKit` → `WaveshareEPD`, with everything that decides *what* to show in
`CalendarCore`. `CalendarCore` knows nothing about the display, which is why
its logic can be unit tested on a PC.

## Pages

The firmware shows one page per boot, then puts the panel and the ESP32 into
deep sleep. **Press BOOT (GPIO0) to wake and show the next page; press RESET to
start again from the calendar.** The current page is kept in RTC memory across
deep sleep. Both pages currently render placeholder data (`src/dummy_data.cpp`);
there is no WiFi or live feed yet. Swapping `DummyData_Fill()` /
`DummyBinData_Fill()` for a real source is the only change the renderers need.

The button wake has not yet been exercised on hardware.

### Calendar

A month grid with an agenda column:

* **Header** — current month and year, today's full date, and a status line.
* **Month grid** — Monday-first, today's date knocked out white on a black
  block, and up to three dots under any day that has events. Days either side
  of the month are drawn smaller so they recede.
* **Agenda** — what's coming up, grouped under `TODAY` / `TOMORROW` / `WED 26
  Aug`, with over-long titles truncated. Overflow shows as `+ N more`.

Data model: [`calendar_data.h`](../lib/CalendarCore/calendar_data.h). Times are
minutes from midnight (already time-zone resolved), `CAL_ALL_DAY` marks all-day
events, `nowMin = -1` hides the clock, and `events` must be sorted by date then
time.

### Bin collection

A hero panel for the next collection day, with the next five days listed beside
it:

* **Hero** — a headline (`TODAY`, `TOMORROW`, a weekday, or `IN 9 DAYS` from a
  week out), the full date, a pill saying what to do ("Put bins out tonight"),
  and one icon per bin being collected. Icons grow when fewer bins are due.
* **Following** — each later collection day with its countdown, the bin names,
  and mini icons.

The panel has no colour, so each bin is an icon plus its name. The five bins
(Blue, Black, Red, Food, Garden) share a wheelie-bin silhouette and are told
apart by a mark on the body (Black is solid, Blue a triangle, Red a band,
Garden a sprig); Food is a caddy. The marks are placeholders and easy to change
in [`bin_icon.cpp`](../lib/BinRender/bin_icon.cpp).

Data model: [`bin_data.h`](../lib/CalendarCore/bin_data.h), a flat list of
(date, bin) entries in any order. `BinSchedule_Build()` sorts them, merges bins
that share a day and drops past dates. It is the tested part; the renderer only
lays out what it returns. The live source will read a shared Google Calendar
(ICS link); bins are to be recognised by event title. That feed is not
implemented yet.

## Commands

```
pio run                     # build the firmware
pio run -t upload           # flash it
pio device monitor          # serial output, 115200 baud
pio test -e native          # unit tests, run on the host (no board needed)
tools/preview.sh bins       # render a page to preview-bins.png (or: calendar)
                            # needs g++ and zlib; optional 2nd arg is the output path
```

`tools/preview.sh` compiles the real renderers and the real `GUI_Paint` against
small Arduino stand-ins in `tools/host/`, so it is much faster than a 15-second
panel refresh when nudging a layout. If shared code starts using a new Arduino
API, add a stub for it in `tools/host/Arduino.h`; if a renderer library is added,
add it to the compile line in `preview.sh`.

New tests go in `test/test_<name>/` and only cover code that builds without the
Arduino framework (the `native` env ignores `CalendarRender`, `BinRender`,
`UiKit` and `WaveshareEPD`), so keep decision logic in `CalendarCore`.

### Adding a page

1. Model and logic in `lib/CalendarCore/` (with tests), drawing in a new
   `lib/<Name>Render/` that depends on `UiKit`.
2. Add it to `lib/*` in `tools/preview.sh`, and a case in `tools/host/host_render.cpp`.
3. Add a `Page` value, a name and a `drawPage()` case in `src/main.cpp`.
4. Add the library to `lib_ignore` under `[env:native]` in `platformio.ini`.

## Drawing with GUI_Paint

The drawing API is Waveshare's `GUI_Paint`, declared in
[`lib/WaveshareEPD/GUI_Paint.h`](../lib/WaveshareEPD/GUI_Paint.h). The useful calls:

```c
Paint_Clear(WHITE);
Paint_DrawString_EN(x, y, "text", &Font24, BLACK, WHITE);   // foreground, background
Paint_DrawNum(x, y, 42, &Font16, BLACK, WHITE);
Paint_DrawLine(x0, y0, x1, y1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
Paint_DrawRectangle(x0, y0, x1, y1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
Paint_DrawCircle(x, y, r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
Paint_DrawBitMap(imageArray);                                // full-screen image
```

Colour arguments are always **foreground first, then background**. Note that
this differs from Waveshare's own examples: their `GUI_Paint` passed the pair
backwards in `Paint_DrawString_EN`, `Paint_DrawNum` and `Paint_DrawTime`, which
turned black-on-white text into white text on a black block. That is fixed here
(see [`lib/WaveshareEPD/README.md`](../lib/WaveshareEPD/README.md)), so **swap the two colour arguments in any
snippet you copy from Waveshare's demos.**

Fonts available: `Font8`, `Font12`, `Font16`, `Font20`, `Font24` (the number is
the pixel height). Rotate the whole canvas by passing `ROTATE_90` / `ROTATE_180` /
`ROTATE_270` to `Paint_NewImage()`.

For images, Waveshare's own image2lcd workflow converts a 800×480 monochrome BMP
into a C array you pass to `Paint_DrawBitMap()`.
