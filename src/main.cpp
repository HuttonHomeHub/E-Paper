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
 * Two things wake it:
 *   - the BOOT button (GPIO0): show the next screen;
 *   - a timer set for just after midnight: redraw the same screen, because the
 *     pages show dates (never the time), so midnight is when they go stale.
 * RESET starts again from the calendar.
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
#include "uk_clock.h"

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

/* Deep sleep until the BOOT button is pressed, or - if the clock is known - just
 * after the next midnight. Waits for the button to be released first, otherwise
 * a still-held press would wake us straight away. */
static void sleepUntilWake()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    const unsigned long start = millis();
    while (digitalRead(BUTTON_PIN) == LOW && millis() - start < 5000) {
        delay(20);
    }

    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);
    rtc_gpio_pullup_en(BUTTON_PIN);
    rtc_gpio_pulldown_dis(BUTTON_PIN);

    const long untilRefresh = UkClock_SecondsUntilRefresh();
    if (untilRefresh > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)untilRefresh * 1000000ULL);
        Serial.print("Automatic refresh in ");
        Serial.print(untilRefresh / 60);
        Serial.println(" min (just after midnight).");
    } else {
        Serial.println("Clock not set - no automatic refresh until the bin page has been shown.");
    }

    Serial.println("Deep sleep. Press BOOT for the next page, RESET to start over.");
    Serial.flush();
    esp_deep_sleep_start();
}

void setup()
{
    DEV_Module_Init();
    delay(200);
    Serial.println();

    UkClock_ApplyTimezone();    /* the setting is lost in deep sleep */

    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0:                 /* BOOT pressed: next page */
            gPage = (gPage + 1) % PAGE_COUNT;
            break;
        case ESP_SLEEP_WAKEUP_TIMER:                /* midnight: same page, new date */
            Serial.println("Scheduled midnight refresh.");
            break;
        default:                                    /* power-up or RESET */
            gPage = PAGE_CALENDAR;
            break;
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

    sleepUntilWake();
}

void loop()
{
    delay(1000);
}
