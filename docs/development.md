# Development

## Layout

```
platformio.ini             envs: esp32 (firmware, default) and native (unit tests)
src/                       firmware entry point + placeholder data source
lib/CalendarCore/          calendar data model and date arithmetic (no hardware deps)
lib/CalendarRender/        draws the screen onto a GUI_Paint canvas
lib/WaveshareEPD/          vendored Waveshare driver (locally modified, see its README)
test/test_calendar_date/   Unity tests for the date arithmetic
tools/                     host-side preview: preview.sh + stubs in tools/host/
docs/                      hardware setup, troubleshooting, this file
```

The dependency direction is one-way: `src/main.cpp` → `CalendarRender` →
`CalendarCore` and `WaveshareEPD`. `CalendarCore` knows nothing about the
display, which is why it can be unit tested on a PC.

## The calendar screen

The default build (`env:esp32`) draws a month grid with an agenda column:

* **Header** — current month and year, today's full date, and a status line.
* **Month grid** — Monday-first, today's date knocked out white on a black
  block, and up to three dots under any day that has events. Days either side
  of the month are drawn smaller so they recede.
* **Agenda** — what's coming up, grouped under `TODAY` / `TOMORROW` / `WED 26
  Aug`, with over-long titles truncated. Overflow shows as `+ N more`.

**It currently renders placeholder data.** There is no WiFi and no calendar
feed in this build — the layout is being settled first. Everything the screen
draws comes from `DummyData_Fill()` in [`src/dummy_data.cpp`](../src/dummy_data.cpp); swapping that for
a live source is the only change needed later.

The data model is in [`calendar_data.h`](../lib/CalendarCore/calendar_data.h):
times are minutes from midnight (already time-zone resolved), `CAL_ALL_DAY`
marks all-day events, `nowMin = -1` hides the clock, and `events` must be sorted
by date then time.

## Commands

```
pio run                     # build the firmware
pio run -t upload           # flash it
pio device monitor          # serial output, 115200 baud
pio test -e native          # unit tests, run on the host (no board needed)
tools/preview.sh out.png    # render the screen to a PNG (needs g++ and zlib)
```

`tools/preview.sh` compiles the real renderer and the real `GUI_Paint` against
small Arduino stand-ins in `tools/host/`, so it is much faster than a 15-second
panel refresh when nudging a layout. If shared code starts using a new Arduino
API, add a stub for it in `tools/host/Arduino.h`; if the renderer gains a source
file, add it to the compile line in `preview.sh`.

New tests go in `test/test_<name>/` and only cover code that builds without the
Arduino framework (the `native` env ignores `CalendarRender` and `WaveshareEPD`).

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
