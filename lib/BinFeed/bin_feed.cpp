#include "bin_feed.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <string.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "bin_cache.h"
#include "bin_schedule.h"
#include "ics_bins.h"
#include "uk_clock.h"

#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

#define WIFI_TIMEOUT_MS   12000
#define NTP_TIMEOUT_MS     8000
#define HTTP_TIMEOUT_MS   12000
#define FEED_ATTEMPTS         2
#define FEED_RETRY_DELAY_MS 2000
#define FEED_MAX_BYTES    (64 * 1024)
#define FEED_MAX_ENTRIES  160

#define CACHE_NAMESPACE   "binfeed"
#define CACHE_KEY_DATA    "coll"
#define CACHE_KEY_DATE    "date"

/* Mozilla-style root CA bundle built into the Arduino ESP32 core; lets the
 * TLS connection be verified without shipping our own certificates. */
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

static BinCollection gCollections[FEED_MAX_ENTRIES];
static char          gWarning[64];

bool BinFeed_Configured()
{
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD) && defined(BIN_FEED_URL)
    return sizeof(WIFI_SSID) > 1;
#else
    return false;
#endif
}

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD) && defined(BIN_FEED_URL)

static const char *const kWifiFail  = "Could not join WiFi";
static const char *const kClockFail = "Could not get the time";
static const char *const kHttpFail  = "Calendar feed unreachable";
static const char *const kBadFeed   = "Calendar feed unreadable";
static const char *const kEmptyFeed = "No bin dates in the feed";

/* ------------------------------------------------------------- flash cache -- */

static void saveCache(int count, uint32_t dateKey)
{
    uint32_t packed[FEED_MAX_ENTRIES];
    for (int i = 0; i < count; i++) packed[i] = BinCache_Pack(&gCollections[i]);

    Preferences prefs;
    if (!prefs.begin(CACHE_NAMESPACE, false)) return;

    /* Skip the flash write when nothing changed, to spare the flash. */
    uint32_t old[FEED_MAX_ENTRIES];
    const size_t oldLen = prefs.getBytesLength(CACHE_KEY_DATA);
    const bool same = oldLen == (size_t)count * sizeof(uint32_t)
        && prefs.getBytes(CACHE_KEY_DATA, old, oldLen) == oldLen
        && memcmp(old, packed, oldLen) == 0
        && prefs.getUInt(CACHE_KEY_DATE, 0) == dateKey;
    if (!same) {
        prefs.putBytes(CACHE_KEY_DATA, packed, (size_t)count * sizeof(uint32_t));
        prefs.putUInt(CACHE_KEY_DATE, dateKey);
    }
    prefs.end();
}

/* Loads the saved copy into gCollections; returns how many entries were valid. */
static int loadCache(uint32_t *dateKey)
{
    Preferences prefs;
    if (!prefs.begin(CACHE_NAMESPACE, true)) return 0;

    uint32_t packed[FEED_MAX_ENTRIES];
    size_t len = prefs.getBytesLength(CACHE_KEY_DATA);
    if (len > sizeof(packed)) len = sizeof(packed);
    len = prefs.getBytes(CACHE_KEY_DATA, packed, len);
    *dateKey = prefs.getUInt(CACHE_KEY_DATE, 0);
    prefs.end();

    int n = 0;
    for (size_t i = 0; i < len / sizeof(uint32_t); i++) {
        if (BinCache_Unpack(packed[i], &gCollections[n])) n++;
    }
    return n;
}

/* --------------------------------------------------------------- download -- */

static bool connectWifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) return false;
        delay(250);
    }
    return true;
}

static void wifiOff()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

/* Downloads and parses the feed into gCollections. WiFi must be up. */
static bool downloadFeed(int *count, const char **error)
{
    WiFiClientSecure client;
    client.setCACertBundle(rootca_crt_bundle_start);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(HTTP_TIMEOUT_MS);

    if (!http.begin(client, BIN_FEED_URL)) { *error = kHttpFail; return false; }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.print("Feed: HTTP status ");
        Serial.println(code);
        http.end();
        *error = kHttpFail;
        return false;
    }
    if (http.getSize() > FEED_MAX_BYTES) { http.end(); *error = kBadFeed; return false; }

    String body = http.getString();
    http.end();

    *count = IcsBins_Parse(body.c_str(), body.length(), gCollections, FEED_MAX_ENTRIES);
    Serial.print("Feed: ");
    Serial.print(body.length());
    Serial.print(" bytes, ");
    Serial.print(*count);
    Serial.println(" bin entries");
    if (*count <= 0) { *error = body.length() > 0 ? kEmptyFeed : kBadFeed; return false; }
    return true;
}

/* ------------------------------------------------------------------- load -- */

bool BinFeed_Load(BinView *view, const char **error)
{
    const char *reason = kWifiFail;
    int count = 0;
    bool fresh = false;

    for (int attempt = 0; attempt < FEED_ATTEMPTS && !fresh; attempt++) {
        if (attempt > 0) {
            Serial.println("Feed: retrying...");
            delay(FEED_RETRY_DELAY_MS);
        }
        Serial.println("Feed: joining WiFi...");
        if (!connectWifi()) { reason = kWifiFail; wifiOff(); continue; }

        Serial.println("Feed: syncing clock...");
        UkClock_SyncNtp(NTP_TIMEOUT_MS);
        struct tm probe;
        if (!UkClock_Now(&probe)) { reason = kClockFail; wifiOff(); continue; }

        Serial.println("Feed: downloading calendar...");
        fresh = downloadFeed(&count, &reason);
        wifiOff();
    }

    /* Whatever happened, today's date comes from the clock: freshly synced, or
     * still running from an earlier sync through deep sleep. */
    struct tm now;
    if (!UkClock_Now(&now)) { *error = kClockFail; return false; }

    view->todayYear  = now.tm_year + 1900;
    view->todayMonth = now.tm_mon + 1;
    view->todayDay   = now.tm_mday;

    if (fresh) {
        saveCache(count, BinCache_DateKey(view->todayYear, view->todayMonth, view->todayDay));
        view->collections     = gCollections;
        view->collectionCount = count;
        view->warning         = NULL;
        return true;
    }

    /* Refresh failed: fall back to the last good copy, and say so loudly. */
    Serial.print("Feed failed: ");
    Serial.println(reason);
    uint32_t savedOn = 0;
    const int cached = loadCache(&savedOn);
    if (cached <= 0) { *error = reason; return false; }

    Serial.println("Feed: showing the saved copy");
    if (savedOn >= 20000101u) {
        Bin_StaleWarning(gWarning, sizeof(gWarning),
                         (int)(savedOn / 10000), (int)(savedOn / 100 % 100), (int)(savedOn % 100));
    } else {
        snprintf(gWarning, sizeof(gWarning), "OUT OF DATE - could not refresh");
    }
    view->collections     = gCollections;
    view->collectionCount = cached;
    view->warning         = gWarning;
    return true;
}

#else   /* no secrets.h */

bool BinFeed_Load(BinView *, const char **error)
{
    *error = "Feed not configured";
    return false;
}

#endif
