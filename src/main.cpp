/**
 * Calendar and Bin Collection screens for the Waveshare 7.5" e-Paper panel
 * (800x480 B/W).
 *
 * The Bin Collection page reads the council's calendar feed over WiFi when
 * include/secrets.h is filled in (see secrets.example.h); without it, and for
 * the Calendar page, placeholder data is shown.
 *
 * One screen is drawn per boot, then the panel is slept and the ESP32 enters
 * deep sleep. E-paper holds its image unpowered, so this costs almost nothing.
 * Press the BOOT button (GPIO0) to wake and show the next screen; press RESET
 * to start again from the calendar.
 */

#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "DEV_Config.h"
#include "EPD_7in5_V2.h"
#include "GUI_Paint.h"

#include "bin_data.h"
#include "bin_feed.h"
#include "bin_render.h"
#include "calendar_data.h"
#include "calendar_render.h"
#include "dummy_data.h"

typedef enum { PAGE_CALENDAR = 0, PAGE_BINS, PAGE_COUNT } Page;

static const char *const kPageNames[PAGE_COUNT] = { "Calendar", "Bin Collection" };

static const UWORD   PANEL_WIDTH  = EPD_7IN5_V2_WIDTH;
static const UWORD   PANEL_HEIGHT = EPD_7IN5_V2_HEIGHT;
static const UDOUBLE IMAGE_BYTES  = (UDOUBLE)(PANEL_WIDTH / 8) * PANEL_HEIGHT;

static const gpio_num_t BUTTON_PIN = GPIO_NUM_0;    /* BOOT button, active low */

/* Survives deep sleep, so each button wake can step to the next page. */
static RTC_DATA_ATTR int gPage = PAGE_CALENDAR;

/* Draws the bin page from the live feed, or says why it can't. Runs before the
 * panel is initialised, so the WiFi radio and the display never work together
 * and the panel is not left powered while waiting on the network. */
static void drawBins()
{
    BinView view;
    if (!BinFeed_Configured()) {
        DummyBinData_Fill(&view);
        BinRender_Draw(&view);
        return;
    }
    const char *error = "";
    if (BinFeed_Load(&view, &error)) {
        BinRender_Draw(&view);
    } else {
        Serial.print("Feed failed: ");
        Serial.println(error);
        BinRender_DrawMessage("Can't load bin dates", error);
    }
}

static void drawPage(Page page)
{
    if (page == PAGE_BINS) {
        drawBins();
    } else {
        CalView view;
        DummyData_Fill(&view);
        CalendarRender_Draw(&view);
    }
}

/* Deep sleep with the BOOT button as the wake source. Waits for the button to
 * be released first, otherwise a still-held press would wake us straight away. */
static void sleepUntilButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    const unsigned long start = millis();
    while (digitalRead(BUTTON_PIN) == LOW && millis() - start < 5000) {
        delay(20);
    }

    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
    rtc_gpio_pullup_en(BUTTON_PIN);
    rtc_gpio_pulldown_dis(BUTTON_PIN);

    Serial.println("Deep sleep. Press BOOT for the next page, RESET to start over.");
    Serial.flush();
    esp_deep_sleep_start();
}

void setup()
{
    DEV_Module_Init();
    delay(200);
    Serial.println();

    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        gPage = (gPage + 1) % PAGE_COUNT;
    } else {
        gPage = PAGE_CALENDAR;
    }
    Serial.print("e-Paper display - page: ");
    Serial.println(kPageNames[gPage]);

    Serial.print("BUSY line (GPIO25) before init reads: ");
    Serial.println(DEV_Digital_Read(EPD_BUSY_PIN) ? "HIGH - panel present and idle"
                                                  : "LOW  - panel not responding");

    UBYTE *frameBuffer = (UBYTE *)malloc(IMAGE_BYTES);
    if (frameBuffer == NULL) {
        Serial.println("FAILED: could not allocate the frame buffer - halting.");
        return;
    }

    /* Draw first (this may fetch over WiFi), then bring the panel up. */
    Serial.println("Drawing page...");
    Paint_NewImage(frameBuffer, PANEL_WIDTH, PANEL_HEIGHT, ROTATE_0, WHITE);
    Paint_SelectImage(frameBuffer);
    drawPage((Page)gPage);

    Serial.println("Initialising panel...");
    EPD_7IN5_V2_Init();

    Serial.println("Clearing panel (this takes a few seconds)...");
    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);

    Serial.println("Refreshing panel...");
    EPD_7IN5_V2_Display(frameBuffer);
    DEV_Delay_ms(2000);

    Serial.println("Sleeping panel.");
    EPD_7IN5_V2_Sleep();

    free(frameBuffer);
    DEV_Module_Exit();

    sleepUntilButton();
}

void loop()
{
    delay(1000);
}
