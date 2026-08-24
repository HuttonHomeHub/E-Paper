/**
 * Hello-world test page for a Waveshare 7.5" e-Paper raw panel (800x480, B/W)
 * driven by a Waveshare e-Paper ESP32 Driver Board.
 *
 * Panel : SKU 13187  - 7.5inch e-Paper raw display, 800x480, black/white (WF0583CZ09)
 * Board : SKU 15823  - e-Paper ESP32 Driver Board (ESP32-WROOM-32)
 *
 * Wiring is fixed by the driver board's FPC connector - see README.md. The pin
 * numbers themselves live in lib/WaveshareEPD/DEV_Config.h.
 *
 * The panel is written once in setup() and then put to sleep. E-paper holds its
 * image with no power, so there is nothing to do in loop() - press the RESET
 * button on the board (or power-cycle it) to draw the page again.
 */

#include <Arduino.h>

#include "DEV_Config.h"
#include "EPD_7in5_V2.h"
#include "GUI_Paint.h"
#include "fonts.h"

// 800 x 480 pixels, 1 bit per pixel => 48000 bytes of RAM for the frame buffer.
static const UWORD PANEL_WIDTH  = EPD_7IN5_V2_WIDTH;
static const UWORD PANEL_HEIGHT = EPD_7IN5_V2_HEIGHT;
static const UDOUBLE IMAGE_BYTES = (UDOUBLE)(PANEL_WIDTH / 8) * PANEL_HEIGHT;

static UBYTE *frameBuffer = NULL;

// Draws a string centred horizontally on the panel.
static void drawCentred(UWORD y, const char *text, sFONT *font)
{
    const UWORD textWidth = (UWORD)(strlen(text) * font->Width);
    const UWORD x = (textWidth >= PANEL_WIDTH) ? 0 : (UWORD)((PANEL_WIDTH - textWidth) / 2);
    Paint_DrawString_EN(x, y, text, font, BLACK, WHITE);
}

// A pattern that makes it obvious the whole panel is being addressed correctly:
// a border, corner registration marks, and some shapes of varying weight.
static void drawTestPage(void)
{
    Paint_Clear(WHITE);

    // Outer border - if any edge is missing, the geometry or rotation is wrong.
    Paint_DrawRectangle(4, 4, PANEL_WIDTH - 5, PANEL_HEIGHT - 5,
                        BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    // Corner registration marks, so a cropped or shifted image is easy to spot.
    const UWORD tick = 40;
    Paint_DrawLine(4, 4, 4 + tick, 4, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(4, 4, 4, 4 + tick, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(PANEL_WIDTH - 5, 4, PANEL_WIDTH - 5 - tick, 4, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(PANEL_WIDTH - 5, 4, PANEL_WIDTH - 5, 4 + tick, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(4, PANEL_HEIGHT - 5, 4 + tick, PANEL_HEIGHT - 5, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(4, PANEL_HEIGHT - 5, 4, PANEL_HEIGHT - 5 - tick, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(PANEL_WIDTH - 5, PANEL_HEIGHT - 5, PANEL_WIDTH - 5 - tick, PANEL_HEIGHT - 5, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(PANEL_WIDTH - 5, PANEL_HEIGHT - 5, PANEL_WIDTH - 5, PANEL_HEIGHT - 5 - tick, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

    // Headline.
    drawCentred(150, "Me and Luna Love Jo", &Font24);

    // Rule under the headline.
    Paint_DrawLine(200, 195, PANEL_WIDTH - 200, 195, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    drawCentred(215, "Waveshare 7.5\" e-Paper - 800x480 - ESP32 driver board", &Font16);
    drawCentred(240, "If you can read this, the wiring and the build are good.", &Font16);

    // Shape row - checks fills, outlines and line styles all render.
    Paint_DrawCircle(250, 340, 45, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawCircle(400, 340, 45, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(505, 295, 595, 385, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawLine(505, 295, 595, 385, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(505, 385, 595, 295, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    // Resolution ramp - vertical lines at 1px spacing. On a healthy panel these
    // stay distinct all the way across; smearing means a refresh or power issue.
    for (UWORD x = 60; x < PANEL_WIDTH - 60; x += 8) {
        Paint_DrawLine(x, 410, x, 440, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }

    drawCentred(450, "hello, world", &Font12);
}

void setup()
{
    // DEV_Module_Init() also calls Serial.begin(115200) for us.
    DEV_Module_Init();
    delay(200);
    Serial.println();
    Serial.println("Waveshare 7.5in e-Paper (800x480 B/W) - hello world");

    Serial.println("Allocating frame buffer...");
    frameBuffer = (UBYTE *)malloc(IMAGE_BYTES);
    if (frameBuffer == NULL) {
        Serial.println("FAILED: could not allocate the frame buffer - halting.");
        return;
    }

    Serial.println("Initialising panel...");
    if (EPD_7IN5_V2_Init() != 0) {
        Serial.println("FAILED: panel did not initialise - check the ribbon cable.");
        return;
    }

    // A full clear first. Skipping this can leave ghosting from the last image.
    Serial.println("Clearing panel (this takes a few seconds)...");
    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);

    Serial.println("Drawing test page...");
    Paint_NewImage(frameBuffer, PANEL_WIDTH, PANEL_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(frameBuffer);
    drawTestPage();

    Serial.println("Refreshing panel...");
    EPD_7IN5_V2_Display(frameBuffer);
    DEV_Delay_ms(2000);

    // Put the panel into deep sleep. Leaving a 7.5" panel powered and idle for
    // long periods can damage it, so always sleep it when you are done drawing.
    Serial.println("Sleeping panel.");
    EPD_7IN5_V2_Sleep();

    free(frameBuffer);
    frameBuffer = NULL;

    DEV_Module_Exit();
    Serial.println("Done. Press RESET on the board to draw it again.");
}

void loop()
{
    // Nothing to do - the image stays on the panel with no power.
    delay(1000);
}
