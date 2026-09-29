#include "ics_bins.h"

#include <string.h>

#include "calendar_date.h"

#define LINE_MAX_LEN   320
#define SUMMARY_MAX_LEN 256

/* ------------------------------------------------------- summary keywords -- */

static char lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c; }
static int  isLetter(char c) { return (c >= 'a' && c <= 'z'); }

static int hasSubstring(const char *hay, const char *needle)
{
    return strstr(hay, needle) != NULL;
}

/* Whole-word match, so "red" does not fire inside "shredded". A '-' counts as
 * a word break, which is what makes "red-lid" match. */
static int hasWord(const char *hay, const char *word)
{
    const size_t n = strlen(word);
    for (const char *p = hay; (p = strstr(p, word)) != NULL; p++) {
        const int startOk = (p == hay) || !isLetter(p[-1]);
        const int endOk   = !isLetter(p[n]);
        if (startOk && endOk) return 1;
    }
    return 0;
}

unsigned IcsBins_FromSummary(const char *summary)
{
    if (summary == NULL) return 0;

    char s[SUMMARY_MAX_LEN];
    size_t i = 0;
    for (; summary[i] != '\0' && i + 1 < sizeof(s); i++) s[i] = lower(summary[i]);
    s[i] = '\0';

    unsigned mask = 0;
    if (hasSubstring(s, "garden"))                                   mask |= 1u << BIN_GARDEN;
    if (hasSubstring(s, "food"))                                     mask |= 1u << BIN_FOOD;
    if (hasSubstring(s, "plastic")   || hasWord(s, "blue"))          mask |= 1u << BIN_BLUE;
    if (hasSubstring(s, "refuse") || hasSubstring(s, "rubbish")
                                     || hasWord(s, "black"))         mask |= 1u << BIN_BLACK;
    if (hasSubstring(s, "paper")     || hasWord(s, "red"))           mask |= 1u << BIN_RED;
    return mask;
}

/* ------------------------------------------------------------- line reader -- */

/* Reads the next logical iCalendar line, joining folded continuations (a
 * physical line starting with a space or tab continues the previous one).
 * Returns 0 at end of input. Over-long lines are truncated, not overrun. */
static int nextLine(const char *ics, size_t len, size_t *pos, char *buf, size_t bufLen)
{
    if (*pos >= len) return 0;

    size_t n = 0;
    for (;;) {
        while (*pos < len && ics[*pos] != '\n') {
            if (ics[*pos] != '\r' && n + 1 < bufLen) buf[n++] = ics[*pos];
            (*pos)++;
        }
        if (*pos < len) (*pos)++;                           /* consume '\n' */

        const int folded = (*pos < len) && (ics[*pos] == ' ' || ics[*pos] == '\t');
        if (!folded) break;
        (*pos)++;                                           /* drop the fold marker */
    }
    buf[n] = '\0';
    return 1;
}

/* ------------------------------------------------------------ property parse -- */

static int startsWithNoCase(const char *s, const char *prefix)
{
    for (; *prefix; s++, prefix++) {
        if (lower(*s) != lower(*prefix)) return 0;
    }
    return 1;
}

/* Value of "NAME[;params]:value", or NULL if this line is not that property. */
static const char *propertyValue(const char *line, const char *name)
{
    if (!startsWithNoCase(line, name)) return NULL;
    const char *p = line + strlen(name);
    if (*p != ':' && *p != ';') return NULL;
    const char *colon = strchr(p, ':');
    return colon ? colon + 1 : NULL;
}

static int parseDate(const char *v, int *y, int *m, int *d)
{
    for (int i = 0; i < 8; i++) {
        if (v[i] < '0' || v[i] > '9') return 0;
    }
    *y = (v[0] - '0') * 1000 + (v[1] - '0') * 100 + (v[2] - '0') * 10 + (v[3] - '0');
    *m = (v[4] - '0') * 10 + (v[5] - '0');
    *d = (v[6] - '0') * 10 + (v[7] - '0');
    return *m >= 1 && *m <= 12 && *d >= 1 && *d <= Cal_DaysInMonth(*y, *m);
}

static void unescapeText(const char *in, char *out, size_t outLen)
{
    size_t n = 0;
    for (; *in && n + 1 < outLen; in++) {
        if (*in == '\\' && in[1] != '\0') {
            in++;
            out[n++] = (*in == 'n' || *in == 'N') ? ' ' : *in;
        } else {
            out[n++] = *in;
        }
    }
    out[n] = '\0';
}

/* ------------------------------------------------------------------- parse -- */

int IcsBins_Parse(const char *ics, size_t len, BinCollection *out, int maxOut)
{
    if (ics == NULL || out == NULL || maxOut <= 0) return 0;

    char line[LINE_MAX_LEN];
    char summary[SUMMARY_MAX_LEN];
    size_t pos = 0;
    int count = 0;

    int inEvent = 0, haveDate = 0;
    int y = 0, m = 0, d = 0;

    while (nextLine(ics, len, &pos, line, sizeof(line))) {
        if (startsWithNoCase(line, "BEGIN:VEVENT")) {
            inEvent = 1; haveDate = 0; summary[0] = '\0';
            continue;
        }
        if (startsWithNoCase(line, "END:VEVENT")) {
            if (inEvent && haveDate) {
                const unsigned mask = IcsBins_FromSummary(summary);
                for (int b = 0; b < BIN_TYPE_COUNT && count < maxOut; b++) {
                    if (!(mask & (1u << b))) continue;
                    out[count].year  = y;
                    out[count].month = m;
                    out[count].day   = d;
                    out[count].bin   = (BinType)b;
                    count++;
                }
            }
            inEvent = 0;
            continue;
        }
        if (!inEvent) continue;

        const char *v;
        if ((v = propertyValue(line, "DTSTART")) != NULL) {
            haveDate = parseDate(v, &y, &m, &d);
        } else if ((v = propertyValue(line, "SUMMARY")) != NULL) {
            unescapeText(v, summary, sizeof(summary));
        }
    }
    return count;
}
