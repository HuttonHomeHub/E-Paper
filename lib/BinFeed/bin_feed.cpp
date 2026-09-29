#include "bin_feed.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>

#include "ics_bins.h"

#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

#define WIFI_TIMEOUT_MS   20000
#define NTP_TIMEOUT_MS    10000
#define HTTP_TIMEOUT_MS   15000
#define FEED_MAX_BYTES    (64 * 1024)
#define FEED_MAX_ENTRIES  160

/* United Kingdom: GMT, BST from the last Sunday of March to the last Sunday of
 * October. */
#define TIMEZONE "GMT0BST,M3.5.0/1,M10.5.0"

/* Mozilla-style root CA bundle built into the Arduino ESP32 core; lets the
 * TLS connection be verified without shipping our own certificates. */
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

static BinCollection gCollections[FEED_MAX_ENTRIES];
static char          gStatus[40];

bool BinFeed_Configured()
{
#if defined(WIFI_SSID) && defined(WIFI_PASSWORD) && defined(BIN_FEED_URL)
    return sizeof(WIFI_SSID) > 1;
#else
    return false;
#endif
}

#if defined(WIFI_SSID) && defined(WIFI_PASSWORD) && defined(BIN_FEED_URL)

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

static bool syncClock(struct tm *now)
{
    configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");

    const unsigned long start = millis();
    while (millis() - start < NTP_TIMEOUT_MS) {
        if (getLocalTime(now, 500) && now->tm_year >= 125) return true;   /* >= 2025 */
    }
    return false;
}

static void wifiOff()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

bool BinFeed_Load(BinView *view, const char **error)
{
    static const char *kWifiFail  = "Could not join WiFi";
    static const char *kClockFail = "Could not get the time";
    static const char *kHttpFail  = "Calendar feed unreachable";
    static const char *kBadFeed   = "Calendar feed unreadable";
    static const char *kEmptyFeed = "No bin dates in the feed";

    Serial.println("Feed: joining WiFi...");
    if (!connectWifi()) { wifiOff(); *error = kWifiFail; return false; }

    Serial.println("Feed: syncing clock...");
    struct tm now;
    if (!syncClock(&now)) { wifiOff(); *error = kClockFail; return false; }

    Serial.println("Feed: downloading calendar...");
    WiFiClientSecure client;
    client.setCACertBundle(rootca_crt_bundle_start);

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(HTTP_TIMEOUT_MS);

    if (!http.begin(client, BIN_FEED_URL)) { wifiOff(); *error = kHttpFail; return false; }

    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.print("Feed: HTTP status ");
        Serial.println(code);
        http.end();
        wifiOff();
        *error = kHttpFail;
        return false;
    }

    const int size = http.getSize();
    if (size > FEED_MAX_BYTES) {
        http.end();
        wifiOff();
        *error = kBadFeed;
        return false;
    }

    String body = http.getString();
    http.end();
    wifiOff();

    const int count = IcsBins_Parse(body.c_str(), body.length(), gCollections, FEED_MAX_ENTRIES);
    Serial.print("Feed: ");
    Serial.print(body.length());
    Serial.print(" bytes, ");
    Serial.print(count);
    Serial.println(" bin entries");
    if (count <= 0) { *error = (body.length() > 0) ? kEmptyFeed : kBadFeed; return false; }

    snprintf(gStatus, sizeof(gStatus), "Updated %02d:%02d", now.tm_hour, now.tm_min);

    view->todayYear       = now.tm_year + 1900;
    view->todayMonth      = now.tm_mon + 1;
    view->todayDay        = now.tm_mday;
    view->collections     = gCollections;
    view->collectionCount = count;
    view->statusLine      = gStatus;
    return true;
}

#else   /* no secrets.h */

bool BinFeed_Load(BinView *, const char **error)
{
    *error = "Feed not configured";
    return false;
}

#endif
