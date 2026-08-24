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
4. Slide the panel's ribbon cable in, **metal contacts facing down** toward the
   board. This matters — inserted upside down, nothing will happen. Push it in
   squarely until it stops; the cable's contacts should be fully hidden.
5. Flip the flap back **down** to clamp it. Tug the cable very lightly — it
   shouldn't move.

The e-paper panel itself is glass and the ribbon cable is the fragile part. Don't
fold the ribbon sharply, and don't insert or remove it with power applied.

### The switches on the board

The driver board carries two small config switches, there so that one board can
drive Waveshare's whole range of panels:

* **Interface Config** — set to **0**. This selects 4-wire SPI, which is the mode
  this code uses. (`1` is 3-wire SPI.)
* **Display Config** — has an **A** and a **B** position, selecting between two
  panel wiring variants. Waveshare's own advice when you're unsure is to **start
  on A**; if the panel won't drive at all, or refreshes but shows a scrambled
  image, power down and flip it to **B**. There's a per-panel table on the
  [board's wiki page](https://www.waveshare.com/wiki/E-Paper_ESP32_Driver_Board)
  if you want to confirm the intended setting for the 7.5" panel rather than
  trying both.

Some board revisions also have a power switch for the USB-to-UART section — if
the board doesn't enumerate at all, check that it's set to **ON**.

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

**The serial log runs all the way through but the screen stays blank** — power the
board off and re-seat the ribbon cable, contacts down and fully inserted. That's
by far the most common cause. Then check the display config switch is on **B**.

**"FAILED: panel did not initialise"** in the log — the BUSY line never responded.
Same checks: ribbon cable seating and orientation.

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

Fonts available: `Font8`, `Font12`, `Font16`, `Font20`, `Font24` (the number is
the pixel height). Rotate the whole canvas by passing `ROTATE_90` / `ROTATE_180` /
`ROTATE_270` to `Paint_NewImage()`.

For images, Waveshare's own image2lcd workflow converts a 800×480 monochrome BMP
into a C array you pass to `Paint_DrawBitMap()`.

## Credits

Driver code is Waveshare's, from <https://github.com/waveshareteam/e-Paper>,
MIT licensed. See [`lib/WaveshareEPD/README.md`](lib/WaveshareEPD/README.md) for
the file-by-file provenance.
