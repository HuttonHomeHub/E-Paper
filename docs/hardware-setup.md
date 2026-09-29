# Hardware setup

Everything needed to get from a fresh checkout to a panel you can flash.

## 1. Install PlatformIO

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

## 2. Install the USB serial driver

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

## 3. Board selection

Already done for you — `platformio.ini` sets `board = esp32dev`, the generic
ESP32-WROOM-32 profile, which is what's on the driver board. You don't need to
pick anything from a menu the way you would in the Arduino IDE.

You also don't normally need to set the serial port: PlatformIO auto-detects it.
If you have several USB serial devices attached and it picks the wrong one,
uncomment `upload_port` / `monitor_port` in `platformio.ini` and put your port
there.

## Connecting the ribbon cable

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

## The config switch (not on every board)

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

## Pin mapping (for reference)

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

These are defined in [`lib/WaveshareEPD/DEV_Config.h`](../lib/WaveshareEPD/DEV_Config.h)
if you ever need to change them.

## Powering it

USB from your computer is enough to drive the 7.5" panel — you don't need a
separate supply.

## Build, flash, watch

With the board plugged in and the panel connected, use the icons in the VS Code
status bar, or the command line:

| Icon | What it does | Command line |
| --- | --- | --- |
| ✓ (tick) | **Build** — compile only, doesn't touch the board | `pio run` |
| → (arrow) | **Upload** — compile and flash to the board | `pio run -t upload` |
| 🔌 (plug) | **Monitor** — open the serial output | `pio device monitor` |

Build first on its own: it proves the toolchain works before the hardware is
involved. Once uploaded, the serial monitor (115200 baud) shows:

```
e-Paper display - page: Calendar
BUSY line (GPIO25) before init reads: HIGH - panel present and idle
Initialising panel...
Clearing panel (this takes a few seconds)...
Drawing page...
Refreshing panel...
Sleeping panel.
Deep sleep. Press BOOT for the next page, RESET to start over.
```

**The panel is slow, and that's normal.** A full refresh of a 7.5" e-paper takes
roughly 4–5 seconds and the firmware clears the screen first, so allow ~10–15
seconds from reset to finished image. The panel flashes black and white a few
times on the way; that is its normal refresh cycle, not a fault.

The Bin Collection page needs your WiFi details first (a 2.4 GHz network); see
[Live bin data](development.md#live-bin-data). Without them it shows placeholder
data.

If the log stops early or the screen stays blank, see
[troubleshooting.md](troubleshooting.md).
