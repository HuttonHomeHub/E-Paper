# Development

## Layout

```
platformio.ini             envs: esp32 (firmware, default) and native (unit tests)
src/                       main.cpp (page switching, panel I/O) + placeholder data
include/                   secrets.example.h (copy to secrets.h, which is gitignored)
lib/CalendarCore/          data models and date/bin-schedule logic (no hardware deps)
lib/UiKit/                 text helpers shared by the screens (fit, centre, scaled text)
lib/CalendarRender/        the calendar screen
lib/BinRender/             the bin collection screen and its bin icons
lib/BinFeed/               WiFi + HTTPS download of the bin calendar, flash cache (Arduino only)
lib/UkClock/               UK time, NTP sync, countdown to the midnight refresh (Arduino only)
lib/WaveshareEPD/          vendored Waveshare driver (locally modified, see its README)
test/                      Unity tests: date maths, bin schedule, ICS parsing, refresh + cache
.github/workflows/ci.yml   tests + builds on every push; monthly check against the latest platform
tools/                     host-side preview: preview.sh + stubs in tools/host/
docs/                      hardware setup, troubleshooting, this file
```

Dependencies point one way: `src/main.cpp` → `CalendarRender` / `BinRender` →
`UiKit` → `WaveshareEPD`, with everything that decides *what* to show in
`CalendarCore` (and `BinFeed` / `UkClock` doing the network and clock work). `CalendarCore` knows nothing about the display, which is why
its logic can be unit tested on a PC.

## Pages

The firmware shows one page per boot, then puts the panel and the ESP32 into
deep sleep. Two things wake it:

* **BOOT (GPIO0)** shows the next page. **RESET** starts again from the
  calendar. The current page is kept in RTC memory across deep sleep.
* **A timer set for 00:05 local time** redraws the *same* page, so "TOMORROW"
  becomes "TODAY" without anyone touching it. It is only set once the clock is
  known, which happens the first time the bin page is shown after a power-up or
  RESET (the serial log says "Clock not set..." until then). The 5 minutes past
  midnight leaves room for the sleep timer, which is not a precision clock, to
  wake slightly early. The timing logic is `lib/CalendarCore/refresh_schedule.*`.

**Design rule: pages show dates, never the time of day.** A page is only redrawn
at midnight (or on a button press), so any clock on it would be wrong within
minutes. Keep it that way; event times in the calendar agenda are fine, they are
data, not "now".

The Bin Collection page reads the live calendar feed (see below); the Calendar
page still shows placeholder data (`src/dummy_data.cpp`) until it gets a source
of its own.

**Verified on the real board (29 Sep 2026):** panel output, deep sleep, the BOOT
wake, WiFi join, NTP sync, and the HTTPS download and parse of the live feed
(10 KB, 59 bin entries). **Not yet verified:** the midnight timer wake, and the
"OUT OF DATE" fallback to the saved copy.

### Calendar

A month grid with an agenda column:

* **Header** — current month and year, today's full date, and a small status
  note ("Sample data" for now).
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
* **Warning banner** — a solid black strip under the title, shown only when the
  data can't be fully trusted: `SAMPLE DATA - not your real dates` (no WiFi
  details configured) or `OUT OF DATE - last updated Tue 27 Sep` (a refresh
  failed and the last saved copy is being shown). Normal pages have no banner.

The panel has no colour, so each bin is an icon plus its name, and the hero
panel captions each with what goes in it. The bins share a wheelie-bin
silhouette and each carries a picture of its contents:

| Bin | Contents | Icon |
| --- | --- | --- |
| Blue | Glass & cans | a bottle and a can on an outlined bin |
| Black | General | a tied rubbish sack, knocked out of a solid bin |
| Red | Cardboard | a box with a flap and tape |
| Food | Food waste | a caddy with a handle and vent slits |
| Garden | Garden waste | a sprig |

The icons are drawn from GUI_Paint primitives in
[`bin_icon.cpp`](../lib/BinRender/bin_icon.cpp) and the captions come from
`Bin_Contents()`.

Data model: [`bin_data.h`](../lib/CalendarCore/bin_data.h), a flat list of
(date, bin) entries in any order. `BinSchedule_Build()` sorts them, merges bins
that share a day and drops past dates. It is the tested part; the renderer only
lays out what it returns.

### Live bin data

`lib/BinFeed` joins WiFi, sets the clock over NTP (UK time), downloads the
council's ReCollect calendar (`.ics`) over HTTPS with certificate checking, and
turns it into bin entries with `IcsBins_Parse()` (`lib/CalendarCore/ics_bins.*`,
unit tested). The bin page is drawn *before* the panel is powered, so the radio
and the display never run together.

If a download fails it is retried once. If it still fails, the page shows the
**last good copy**, which every successful download saves to flash
(`bin_cache.*` packs each entry into 32 bits), under the `OUT OF DATE` banner.
Only when there is no saved copy, or the date is unknown, does the page show a
plain error with the reason ("Could not join WiFi", "Calendar feed
unreachable", ...) rather than made-up dates. Today's date comes from the clock,
which keeps running through deep sleep, so a failed refresh still shows correct
"TODAY / TOMORROW" labels.

Set it up once:

1. Copy `include/secrets.example.h` to `include/secrets.h` (gitignored).
2. Fill in `WIFI_SSID`, `WIFI_PASSWORD` and `BIN_FEED_URL` (use `https://`; a
   `webcal://` link works with the scheme changed). **The WiFi name is
   case-sensitive**: `HuttonHomeHub` is not `HUTTONHOMEHUB`. The link identifies your
   property, so keep it out of the repo.
3. Rebuild and flash. With no `secrets.h`, or an empty `WIFI_SSID`, the page
   shows placeholder data and the build still works.

The feed has one all-day event per collection day, and its title names *what*
is collected, not a bin colour, so one event can mean several bins. The keyword
table is `IcsBins_FromSummary()`:

| Wording in the event title | Bin |
| --- | --- |
| `plastic`, `glass`, the word `cans`, or the word `blue` | Blue |
| `refuse`, `rubbish`, the word `general`, or the word `black` | Black |
| `paper`, `cardboard`, or the word `red` | Red |
| `food` | Food |
| `garden` | Garden |

For example "Food waste, plastic recycling (blue-lid bin...), and refuse
(household rubbish)" is Blue + Black + Food. Note the council's own wording
calls the blue bin "plastic recycling" while the household describes it as glass
and cans; both map to Blue, so either wording works. If a different feed words
things differently, this table is the one place to change. Events matching
nothing are ignored.

On the bin page the serial log also shows the fetch (`Feed: joining WiFi...`,
`syncing clock...`, `downloading calendar...`, then the byte and entry counts),
which is the first place to look if the page shows an error.

The feed is cached for 12 hours by its server and holds roughly the next five
months. It is downloaded fresh every time the bin page is drawn: on each BOOT
wake onto it, and at the midnight refresh.

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
`BinFeed`, `UkClock`, `UiKit` and `WaveshareEPD`), so keep decision logic in
`CalendarCore`.

### Adding a page

1. Model and logic in `lib/CalendarCore/` (with tests), drawing in a new
   `lib/<Name>Render/` that depends on `UiKit`. Give the new library a
   `library.json` like the existing ones.
2. Add it to `lib/*` in `tools/preview.sh`, and a case in `tools/host/host_render.cpp`.
3. Add a `Page` value, a name and a `drawPage()` case in `src/main.cpp`.
4. Add the library to `lib_ignore` under `[env:native]` in `platformio.ini`.

## Continuous integration and versions

`.github/workflows/ci.yml` runs on every push and pull request: the unit tests,
a firmware build with no `secrets.h` (placeholder data), and a firmware build
with the live-feed path compiled in (it fails if the HTTP client isn't linked,
which guards the `-Iinclude` setting). The same commands work locally, so a
green local run predicts a green CI run.

`platformio.ini` **pins** the ESP32 and native platform versions, so a new
release can't change the build unannounced. GitHub's dependency bot doesn't
cover PlatformIO, so CI checks instead: a monthly job (also runnable by hand
from the Actions tab) removes the pins and builds against the newest platform.

* **Green:** the newest platform works; upgrade whenever you like by changing
  the two versions in `platformio.ini` (`pio pkg outdated` shows what is newer).
* **Red:** a newer release breaks the build. Stay on the pin and look into it
  before upgrading.
* In between, upgrade when you need a fix or feature from a release, or roughly
  once a quarter.

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
