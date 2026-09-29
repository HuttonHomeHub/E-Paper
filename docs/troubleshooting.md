# Troubleshooting

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
A full `EPD_7IN5_V2_Clear()` before drawing (which the firmware does) removes most
of it. Panels that have sat displaying one image for a long time may need two or
three clear cycles.

**Don't leave the panel powered and idle for hours.** It can damage the display.
The firmware calls `EPD_7IN5_V2_Sleep()` when it's done drawing, which is exactly
why — keep that habit in anything you build on top of this.
