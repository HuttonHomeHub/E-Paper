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
 * Three things wake it:
 *   - the BOOT button (GPIO0): show the next screen;
 *   - a timer set for just after midnight: redraw the same screen, because the
 *     pages show dates (never the time), so midnight is when they go stale;
 *   - a retry timer, only while the bin dates are out of date: try the download
 *     again WITHOUT touching the panel, and redraw only if it now works.
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
#include "refresh_schedule.h"
#include "uk_clock.h"

typedef enum { PAGE_CALENDAR = 0, PAGE_BINS, PAGE_COUNT } Page;

static const char *const kPageNames[PAGE_COUNT] = { "Calendar", "Bin Collection" };

static const UWORD   PANEL_WIDTH  = EPD_7IN5_V2_WIDTH;
static const UWORD   PANEL_HEIGHT = EPD_7IN5_V2_HEIGHT;
static const UDOUBLE IMAGE_BYTES  = (UDOUBLE)(PANEL_WIDTH / 8) * PANEL_HEIGHT;

static const gpio_num_t BUTTON_PIN = GPIO_NUM_0;    /* BOOT button, active low */

/* State that survives deep sleep. */
static RTC_DATA_ATTR int  gPage        = PAGE_CALENDAR;
static RTC_DATA_ATTR int  gRetryStep   = -1;        /* -1: nothing to retry; else next retry (0 = first) */
static RTC_DATA_ATTR bool gWakeIsRetry = false;     /* the timer we slept on was a retry, not midnight */

/* The bin page's data, loaded before the frame buffer exists (see setup). */
static BinView     gBinView;
static bool        gBinHaveView = false;    /* gBinView is usable (fresh or the saved copy) */
static bool        gBinSettled  = true;     /* nothing to retry: fresh, or placeholder data */
static const char *gBinError    = "";

static void logHeap(const char *when)
{
    Serial.print("Heap ");
    Serial.print(when);
    Serial.print(": ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" free, largest block ");
    Serial.println(ESP.getMaxAllocHeap());
}

/* Loads the bin page's data. Done BEFORE the 48 KB frame buffer is allocated:
 * the secure connection needs a large contiguous block of RAM, and the display
 * buffer must not be sitting in the way. */
static void loadBins()
{
    gBinHaveView = false;
    gBinSettled  = true;

    if (!BinFeed_Configured()) {
        DummyBinData_Fill(&gBinView);       /* placeholder data: nothing to retry */
        gBinHaveView = true;
        return;
    }

    logHeap("before feed");
    if (BinFeed_Load(&gBinView, &gBinError)) {
        gBinHaveView = true;
        gBinSettled  = BinFeed_LastLoadWasFresh();
    } else {
        Serial.print("Feed failed: ");
        Serial.println(gBinError);
        gBinSettled = false;
    }
    logHeap("after feed");
}

static void drawPage(Page page)
{
    if (page == PAGE_BINS) {
        if (gBinHaveView) BinRender_Draw(&gBinView);
        else              BinRender_DrawMessage("Can't load bin dates", gBinError);
    } else {
        CalView view;
        DummyData_Fill(&view);
        CalendarRender_Draw(&view);
    }
}

/* Deep sleep until the BOOT button is pressed, or the next timer wake (a retry
 * if one is due, else just after midnight, if the clock is known). Waits for the
 * button to be released first, otherwise a still-held press would wake us
 * straight away. */
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

    int isRetry = 0;
    const long seconds = Refresh_NextWake(UkClock_SecondsUntilRefresh(), gRetryStep, &isRetry);
    gWakeIsRetry = isRetry != 0;

    if (seconds > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
        Serial.print(isRetry ? "Retry in " : "Automatic refresh in ");
        Serial.print(seconds / 60);
        Serial.println(isRetry ? " min (quiet: the panel is only redrawn if it works)."
                               : " min (just after midnight).");
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

    bool retryWake = false;
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0:                 /* BOOT pressed: next page */
            gPage = (gPage + 1) % PAGE_COUNT;
            gRetryStep = -1;
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            if (gWakeIsRetry && gPage == PAGE_BINS) {
                retryWake = true;                   /* quiet retry: same page, panel untouched unless it works */
                Serial.println("Retry wake.");
            } else {
                gRetryStep = -1;                    /* midnight: same page, new date */
                Serial.println("Scheduled midnight refresh.");
            }
            break;
        default:                                    /* power-up or RESET */
            gPage = PAGE_CALENDAR;
            gRetryStep = -1;
            break;
    }
    Serial.print("e-Paper display - page: ");
    Serial.println(kPageNames[gPage]);

    /* Network first, panel and frame buffer afterwards. */
    if (gPage == PAGE_BINS) {
        loadBins();

        if (retryWake && !gBinSettled) {
            /* Still can't refresh: back off and go back to sleep without ever
             * powering the panel, so an outage costs no flashing at all. */
            gRetryStep++;
            Serial.println("Retry failed - the panel is left as it is.");
            DEV_Module_Exit();
            sleepUntilWake();
            return;
        }
        gRetryStep = gBinSettled ? -1 : 0;          /* not settled: first retry is due */
        if (retryWake) Serial.println("Retry succeeded - redrawing.");
    }

    Serial.print("BUSY line (GPIO25) before init reads: ");
    Serial.println(DEV_Digital_Read(EPD_BUSY_PIN) ? "HIGH - panel present and idle"
                                                  : "LOW  - panel not responding");

    UBYTE *frameBuffer = (UBYTE *)malloc(IMAGE_BYTES);
    if (frameBuffer == NULL) {
        Serial.println("FAILED: could not allocate the frame buffer - halting.");
        return;
    }

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
