# E-Paper — Waveshare 7.5" (800×480) on an ESP32 driver board

A starter PlatformIO project that draws a test page to a Waveshare 7.5inch
e-Paper panel from a Waveshare e-Paper ESP32 Driver Board.

| | |
| --- | --- |
| **Display** | Waveshare 7.5inch e-Paper raw display, 800×480, black/white (SKU 13187, panel `WF0583CZ09`) |
| **Controller board** | Waveshare e-Paper ESP32 Driver Board (SKU 15823, ESP32-WROOM-32) |
| **Driver used** | `EPD_7IN5_V2` — Waveshare's official driver for the 800×480 B/W panel |
| **Framework** | Arduino, via PlatformIO |

The `.ino`-style sketch lives in [`src/main.cpp`](src/main.cpp). Waveshare's
driver code is vendored into [`lib/WaveshareEPD/`](lib/WaveshareEPD/) — see the
README there for exactly which upstream files were copied and why.

---

## What the test page draws

* A border around the full 800×480 area, plus corner registration marks — if any
  edge or corner is missing, the geometry is wrong, not the code.
* The headline **“Me and Luna Love Jo”**.
* Two caption lines confirming the panel and resolution.
* A row of shapes (outlined circle, filled circle, box with dotted diagonals).
* A ramp of thin vertical lines, which is the quickest way to spot a panel that
  isn't refreshing cleanly.

It draws once in `setup()` and then puts the panel to sleep. E-paper holds its
image with no power, so `loop()` does nothing — **press the RESET button on the
board to draw it again.**

---

## Part 1 — What you need to do on your computer

### 1. Install PlatformIO

PlatformIO is the build tool. It downloads the ESP32 compiler, builds your code
and flashes it to the board. The easiest way in is the VS Code extension.

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open VS Code → Extensions panel (the blocks icon in the left bar) → search for
   **PlatformIO IDE** → Install.
3. Let it finish. The first install pulls down a bundled Python and takes several
   minutes. VS Code will ask to reload when it's done.
4. Open this folder in VS Code (`File → Open Folder…`, pick the `E-Paper` folder).
   PlatformIO recognises it as a project because of `platformio.ini`.

Prefer the command line? `pip install platformio` gets you the same thing, and
every `pio` command below works from a terminal in this folder.

The first build downloads the ESP32 toolchain (a few hundred MB, one time only).

### 2. Install the USB serial driver

The ESP32 doesn't talk USB directly — there's a USB-to-serial chip on the driver
board, and your computer needs a driver for it before the board shows up as a
serial port.

**Check which chip your board has** — Waveshare changed it. Look at the small
chip next to the USB socket:

* **CH343** — boards from revision 20220728 onwards. Install the
  [WCH CH343 driver](https://www.wch-ic.com/downloads/CH343SER_ZIP.html).
* **CP2102** — earlier boards. Install Silicon Labs'
  [CP210x VCP driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers).

Then, for either chip:

* **Windows** — download the driver zip, unzip, run the installer. Reboot if asked.
* **macOS** — run the installer. macOS blocks it on first run: go to
  *System Settings → Privacy & Security*, scroll down, click **Allow**, reboot.
* **Linux** — both chips are supported by the in-tree kernel drivers, so there's
  nothing to install. You do need permission to use the port:
  `sudo usermod -a -G dialout $USER`, then log out and back in.

**Check it worked:** plug the board in with a USB **data** cable — a charge-only
cable is a very common cause of "the board doesn't show up".

* Windows — Device Manager → *Ports (COM & LPT)* → a `USB-SERIAL CH343` or
  `Silicon Labs CP210x…` entry with a COM number, e.g. `COM5`.
* macOS — `ls /dev/cu.*` in Terminal → e.g. `/dev/cu.usbserial-0001`.
* Linux — `ls /dev/ttyUSB*` (CP2102) or `ls /dev/ttyACM*` (CH343).

### 3. Board selection

Already done for you — `platformio.ini` sets `board = esp32dev`, the generic
ESP32-WROOM-32 profile, which is what's on the driver board. You don't need to
pick anything from a menu the way you would in the Arduino IDE.

You also don't normally need to set the serial port: PlatformIO auto-detects it.
If you have several USB serial devices attached and it picks the wrong one,
uncomment `upload_port` / `monitor_port` in `platformio.ini` and put your port
there.

---

## Part 2 — Wiring the display

**Good news: there is nothing to wire by hand.** The ESP32 driver board exists
precisely so you don't have to. The 7.5" panel ends in a flat ribbon cable (FPC),
and the board has a matching FPC connector.

### Connecting the ribbon cable

1. **Power off / unplug the board first.**
2. Find the black or white FPC connector on the driver board. It has a small
   hinged flap along one edge.
3. Gently flip that flap **up** — it pivots, it does not pull out. Use a
   fingernail; it takes very little force.
4. Slide the panel's ribbon cable in and push squarely until it stops.

   **Which way up?** Don't trust a rule of thumb here — it varies by board and
   cable revision. On the board this project was tested with, contacts face
   **up**, away from the board. If the cable is in the wrong way the code runs
   perfectly and hangs at `Initialising panel...`, because the panel never
   responds. So: if you get that symptom, power down and flip the cable. That
   is a normal part of first setup, not a mistake.
5. Flip the flap back **down** to clamp it. Tug the cable very lightly — it
   shouldn't move.

The e-paper panel itself is glass and the ribbon cable is the fragile part. Don't
fold the ribbon sharply, and don't insert or remove it with power applied.

### The config switch (not on every board)

Some revisions of the driver board carry a small 2-position DIP switch — a tiny
plastic block with two sliders, usually near the USB socket:

* **Switch 1** — display mode select (**A** / **B**), selecting between two panel
  wiring variants. Waveshare's advice when unsure is to start on **A**.
* **Switch 2** — powers the USB-to-UART chip. Must be **ON** or you cannot
  upload at all.

**Several revisions have no switch at all**, including the one this project was
tested on. If you can't find it, that's fine — there's nothing to set. Plug in
and carry on; if a COM port appears in Device Manager, the USB-UART side is
powered either way.

### Pin mapping (for reference)

You don't need to connect these yourself; the FPC connector routes them. This is
just what the code expects, and it's what you'd wire up if you ever drove the
panel from a bare ESP32 with the Waveshare adapter instead:

| Panel signal | ESP32 GPIO |
| --- | --- |
| BUSY | 25 |
| RST | 26 |
| DC | 27 |
| CS | 15 |
| CLK / SCK | 13 |
| DIN / MOSI | 14 |
| GND | GND |
| VCC | 3.3 V |

These are defined in [`lib/WaveshareEPD/DEV_Config.h`](lib/WaveshareEPD/DEV_Config.h)
if you ever need to change them.

### Powering it

USB from your computer is enough to drive the 7.5" panel — you don't need a
separate supply for this test.

---

## Part 3 — Build, upload, watch

With the board plugged in and the panel connected, in VS Code use the small icons
in the blue status bar along the bottom:

| Icon | What it does | Command line |
| --- | --- | --- |
| ✓ (tick) | **Build** — compile only, doesn't touch the board | `pio run` |
| → (arrow) | **Upload** — compile and flash to the board | `pio run -t upload` |
| 🔌 (plug) | **Monitor** — open the serial output | `pio device monitor` |

Do them in that order the first time. Build on its own proves your toolchain
works before you involve the hardware at all.

Once uploaded you should see, in the serial monitor at 115200 baud:

```
Waveshare 7.5in e-Paper (800x480 B/W) - hello world
Allocating frame buffer...
Initialising panel...
Clearing panel (this takes a few seconds)...
Drawing test page...
Refreshing panel...
Sleeping panel.
Done. Press RESET on the board to draw it again.
```

**The panel is slow, and that's normal.** A full refresh of a 7.5" e-paper takes
roughly 4–5 seconds, and the code clears the screen first, so allow ~10–15
seconds from reset to finished image. You'll see it flash black and white a few
times on the way — that's the panel's normal refresh cycle, not a fault.

---

## Troubleshooting

**Upload fails with "Failed to connect to ESP32: Timed out waiting for packet
header"** — the board isn't in bootloader mode. Most ESP32 boards enter it
automatically; if this one doesn't, hold the **BOOT** button down, tap **RESET**,
release BOOT, and start the upload. Also try a different USB cable and a port
directly on the machine rather than through a hub.

**Upload starts then fails partway, or the serial monitor is full of garbage** —
lower `upload_speed` in `platformio.ini` from `921600` to `115200`.

**"Could not open port" / "Access denied"** — the serial monitor is still holding
the port. Close it before uploading. On Linux, check you're in the `dialout` group.

**The log stops at `Initialising panel...` / `e-Paper busy`, or reports
`BUSY TIMEOUT`** — the panel never released its BUSY line, so the ESP32 is
getting no electrical response from it. In order:

1. **Flip the ribbon cable over.** Power down first. This is the most common
   cause by a wide margin, and the orientation is not what most guides claim.
2. Re-seat it fully and make sure the latch is properly closed — a cable that's
   inserted but unclamped behaves exactly like no cable.
3. If your board has the A/B switch, flip it.
4. Try `-D D_9PIN=1` in `platformio.ini`. Some revisions gate the panel's power
   through GPIO33, in which case the panel is never switched on at all.

The `BUSY line (GPIO25) before init reads:` line in the log narrows this down:
`LOW` before any command is sent means the fault is physical, not in the code.

**The serial log runs all the way through to `Done.` but the screen stays blank**
— same physical checks as above, starting with the ribbon cable.

**The image appears but is scrambled, doubled or shifted** — first flip the
Display Config switch (A ↔ B) with the power off and try again. If that doesn't
fix it, it's the wrong panel driver for your hardware revision. This project uses `EPD_7IN5_V2` (800×480). If your
panel is an older 7.5" it may be 640×384 and need `EPD_7in5` instead, or an HD
variant at 880×528 needing `EPD_7in5_HD`. All of those are in
[Waveshare's repository](https://github.com/waveshareteam/e-Paper).

**Ghosting — a faint version of the previous image remains** — normal for e-paper.
A full `EPD_7IN5_V2_Clear()` before drawing (which this sketch does) removes most
of it. Panels that have sat displaying one image for a long time may need two or
three clear cycles.

**Don't leave the panel powered and idle for hours.** It can damage the display.
The sketch calls `EPD_7IN5_V2_Sleep()` when it's done drawing, which is exactly
why — keep that habit in anything you build on top of this.

---

## The calendar screen

The default build (`env:calendar`) draws a month grid with an agenda column:

* **Header** — current month and year, today's full date, and a status line.
* **Month grid** — Monday-first, today's date knocked out white on a black
  block, and up to three dots under any day that has events. Days either side
  of the month are drawn smaller so they recede.
* **Agenda** — what's coming up, grouped under `TODAY` / `TOMORROW` / `WED 26
  Aug`, with over-long titles truncated. Overflow shows as `+ N more`.

**It currently renders placeholder data.** There is no WiFi and no calendar
feed in this build — the layout is being settled first. Everything the screen
draws comes from `DummyData_Fill()` in `src/dummy_data.cpp`; swapping that for
a live source is the only change needed later.

### Previewing without hardware

The renderer is deliberately free of Arduino and network code, so the exact
screen the ESP32 draws can be produced on a normal computer:

```
tools/render.sh calendar.png
```

That compiles the real `GUI_Paint` and the real renderer against a small
Arduino shim and writes a PNG. Much faster than a 15-second panel refresh when
you're nudging a layout.

```
tools/test.sh
```

runs the date checks (weekday calculation, leap years, month and year
rollovers) — the parts that fail silently and wrongly rather than loudly.

### Switching between the two builds

```
pio run -e calendar -t upload    # the calendar screen (default)
pio run -e hello    -t upload    # the original hardware test page
```

In VS Code, the PlatformIO sidebar lists both under **Project Tasks**, each
with its own Build and Upload. Plain `pio run -t upload` builds the calendar.

## Where to go next

The drawing API is Waveshare's `GUI_Paint`, declared in
[`lib/WaveshareEPD/GUI_Paint.h`](lib/WaveshareEPD/GUI_Paint.h). The useful calls:

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
(see `lib/WaveshareEPD/README.md`), so **swap the two colour arguments in any
snippet you copy from Waveshare's demos.**

Fonts available: `Font8`, `Font12`, `Font16`, `Font20`, `Font24` (the number is
the pixel height). Rotate the whole canvas by passing `ROTATE_90` / `ROTATE_180` /
`ROTATE_270` to `Paint_NewImage()`.

For images, Waveshare's own image2lcd workflow converts a 800×480 monochrome BMP
into a C array you pass to `Paint_DrawBitMap()`.

## Credits

Driver code is Waveshare's, from <https://github.com/waveshareteam/e-Paper>,
MIT licensed. See [`lib/WaveshareEPD/README.md`](lib/WaveshareEPD/README.md) for
the file-by-file provenance.
