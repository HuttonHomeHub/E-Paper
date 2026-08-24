/**
 * Calendar screen for the Waveshare 7.5" e-Paper panel (800x480 B/W).
 *
 * Currently renders placeholder data - the layout is being built first, and a
 * live feed will replace DummyData_Fill() once the design is settled. There is
 * no WiFi in this build.
 *
 * Drawn once in setup(), then the panel is slept. Press RESET to redraw.
 */

#include <Arduino.h>

#include "DEV_Config.h"
#include "EPD_7in5_V2.h"
#include "GUI_Paint.h"

#include "calendar_data.h"
#include "calendar_render.h"
#include "dummy_data.h"

static const UWORD   PANEL_WIDTH  = EPD_7IN5_V2_WIDTH;
static const UWORD   PANEL_HEIGHT = EPD_7IN5_V2_HEIGHT;
static const UDOUBLE IMAGE_BYTES  = (UDOUBLE)(PANEL_WIDTH / 8) * PANEL_HEIGHT;

void setup()
{
    DEV_Module_Init();
    delay(200);
    Serial.println();
    Serial.println("e-Paper calendar - placeholder data");

    Serial.print("BUSY line (GPIO25) before init reads: ");
    Serial.println(DEV_Digital_Read(EPD_BUSY_PIN) ? "HIGH - panel present and idle"
                                                  : "LOW  - panel not responding");

    UBYTE *frameBuffer = (UBYTE *)malloc(IMAGE_BYTES);
    if (frameBuffer == NULL) {
        Serial.println("FAILED: could not allocate the frame buffer - halting.");
        return;
    }

    Serial.println("Initialising panel...");
    EPD_7IN5_V2_Init();

    Serial.println("Clearing panel (this takes a few seconds)...");
    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);

    Serial.println("Drawing calendar...");
    Paint_NewImage(frameBuffer, PANEL_WIDTH, PANEL_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(frameBuffer);

    CalView view;
    DummyData_Fill(&view);
    CalendarRender_Draw(&view);

    Serial.println("Refreshing panel...");
    EPD_7IN5_V2_Display(frameBuffer);
    DEV_Delay_ms(2000);

    Serial.println("Sleeping panel.");
    EPD_7IN5_V2_Sleep();

    free(frameBuffer);
    DEV_Module_Exit();
    Serial.println("Done. Press RESET on the board to draw it again.");
}

void loop()
{
    delay(1000);
}
