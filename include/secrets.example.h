/**
 * Copy this file to secrets.h (same folder) and fill it in. secrets.h is
 * gitignored, so your WiFi password and calendar link never reach the repo.
 *
 * Without a secrets.h, or with an empty WIFI_SSID, the firmware still builds
 * and shows placeholder data on the Bin Collection page.
 */
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

/* The calendar's .ics link. A webcal:// link works if you change the scheme to
 * https://. The link identifies your property, so treat it as private. */
#define BIN_FEED_URL  "https://example.invalid/events.ics"

#endif
