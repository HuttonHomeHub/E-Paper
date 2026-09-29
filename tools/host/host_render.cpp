/**
 * Host-side render harness. Builds the same screens the ESP32 draws, using the
 * real GUI_Paint code and the real renderers, on a normal computer.
 *
 *   host_render --list                          print the scenario names
 *   host_render <scenario> <out.png>            render a scenario to a PNG
 *   host_render --check <scenario> <golden.png> <actual.png>
 *                                               render and compare with a reference
 *                                               image pixel for pixel; on a
 *                                               difference write <actual.png>
 *
 * A "scenario" is a screen plus a fixed set of data, so it always renders the
 * same. They cover the states worth eyeballing: normal, extremes, and the
 * warning banners. Build and run through tools/preview.sh or tools/golden.sh.
 */
#include "GUI_Paint.h"
#include "bin_render.h"
#include "bin_schedule.h"
#include "calendar_render.h"
#include "dummy_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define W 800
#define H 480

/* ------------------------------------------------------------------ scenarios -- */

/* All scenarios use the placeholder "today" (Mon 24 Aug 2026) unless they say so. */
static BinCollection gCol[16];
static char          gWarning[80];

static void bins(int n, const BinCollection *src, const char *warning, BinView *v)
{
    DummyBinData_Fill(v);
    for (int i = 0; i < n; i++) gCol[i] = src[i];
    v->collections     = gCol;
    v->collectionCount = n;
    v->warning         = warning;
}

static void drawBins(int n, const BinCollection *src, const char *warning)
{
    BinView v;
    bins(n, src, warning, &v);
    BinRender_Draw(&v);
}

typedef struct {
    const char *name;
    void (*draw)(void);
} Scenario;

static void scCalendar(void)   { CalView v; DummyData_Fill(&v); CalendarRender_Draw(&v); }
static void scBins(void)       { BinView v; DummyBinData_Fill(&v); BinRender_Draw(&v); }   /* "sample data" banner */

static void scOne(void)
{
    const BinCollection c[] = { { 2026, 8, 24, BIN_FOOD } };
    drawBins(1, c, NULL);
}

static void scFive(void)
{
    const BinCollection c[] = { { 2026, 8, 26, BIN_BLUE }, { 2026, 8, 26, BIN_BLACK },
                                { 2026, 8, 26, BIN_RED },  { 2026, 8, 26, BIN_FOOD },
                                { 2026, 8, 26, BIN_GARDEN }, { 2026, 9, 2, BIN_RED } };
    drawBins(6, c, NULL);
}

static void scToday(void)
{
    const BinCollection c[] = { { 2026, 8, 24, BIN_BLUE }, { 2026, 8, 24, BIN_GARDEN },
                                { 2026, 8, 24, BIN_RED } };
    drawBins(3, c, NULL);
}

static void scFar(void)
{
    const BinCollection c[] = { { 2026, 8, 31, BIN_BLACK }, { 2026, 8, 31, BIN_BLUE } };
    drawBins(2, c, NULL);
}

static void scEmpty(void)      { drawBins(0, NULL, NULL); }

static void scStale(void)
{
    const BinCollection c[] = { { 2026, 8, 25, BIN_BLACK }, { 2026, 8, 25, BIN_FOOD },
                                { 2026, 9, 1, BIN_BLUE }, { 2026, 9, 1, BIN_FOOD },
                                { 2026, 9, 1, BIN_GARDEN } };
    Bin_StaleWarning(gWarning, sizeof(gWarning), 2026, 8, 20);
    drawBins(5, c, gWarning);
}

static void scEndsSoon(void)
{
    const BinCollection c[] = { { 2026, 8, 25, BIN_BLACK }, { 2026, 8, 25, BIN_FOOD },
                                { 2026, 9, 1, BIN_BLUE }, { 2026, 9, 1, BIN_FOOD },
                                { 2026, 9, 4, BIN_RED }, { 2026, 9, 4, BIN_FOOD } };
    BinView v;
    bins(6, c, NULL, &v);
    if (BinSchedule_CalendarEnd(&v, gWarning, sizeof(gWarning))) v.warning = gWarning;
    BinRender_Draw(&v);
}

static void scMessage(void)    { BinRender_DrawMessage("Can't load bin dates", "Could not join WiFi"); }

static const Scenario kScenarios[] = {
    { "calendar",       scCalendar },
    { "bins",           scBins },
    { "bins-one",       scOne },
    { "bins-five",      scFive },
    { "bins-today",     scToday },
    { "bins-far",       scFar },
    { "bins-empty",     scEmpty },
    { "bins-stale",     scStale },
    { "bins-ends-soon", scEndsSoon },
    { "bins-message",   scMessage },
};
static const int kScenarioCount = (int)(sizeof(kScenarios) / sizeof(kScenarios[0]));

static const Scenario *findScenario(const char *name)
{
    for (int i = 0; i < kScenarioCount; i++) {
        if (strcmp(kScenarios[i].name, name) == 0) return &kScenarios[i];
    }
    return NULL;
}

/* ------------------------------------------------------------------- pixels -- */

/* Paint_Clear(WHITE) fills 0xFF, so bit 1 is white and bit 0 is black ink. */
static void renderToGray(const Scenario *s, unsigned char *gray)
{
    static UBYTE buf[(W / 8) * H];
    Paint_NewImage(buf, W, H, ROTATE_0, WHITE);
    Paint_SelectImage(buf);
    s->draw();

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            const int bit = (buf[y * (W / 8) + (x / 8)] >> (7 - (x % 8))) & 1;
            gray[y * W + x] = bit ? 255 : 0;
        }
    }
}

/* ---------------------------------------------------------------------- PNG -- */

static void put32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);  p[3] = (unsigned char)v;
}

static unsigned long get32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8)  |  (unsigned long)p[3];
}

static void writeChunk(FILE *f, const char *type, const unsigned char *data, unsigned long len)
{
    unsigned char hdr[4];
    put32(hdr, len);
    fwrite(hdr, 1, 4, f);
    fwrite(type, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    unsigned long crc = crc32(0L, (const Bytef *)type, 4);
    if (len) crc = crc32(crc, data, (unsigned)len);
    put32(hdr, crc);
    fwrite(hdr, 1, 4, f);
}

/* 8-bit greyscale, no filtering, one IDAT chunk. */
static int writePng(const char *path, const unsigned char *gray)
{
    unsigned char *raw = (unsigned char *)malloc((size_t)(W + 1) * H);
    for (int y = 0; y < H; y++) {
        raw[(size_t)y * (W + 1)] = 0;                       /* filter: none */
        memcpy(raw + (size_t)y * (W + 1) + 1, gray + (size_t)y * W, W);
    }
    unsigned long clen = compressBound((unsigned long)(W + 1) * H);
    unsigned char *comp = (unsigned char *)malloc(clen);
    compress(comp, &clen, raw, (unsigned long)(W + 1) * H);

    FILE *f = fopen(path, "wb");
    if (!f) { free(raw); free(comp); return 0; }
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    unsigned char ihdr[13];
    put32(ihdr, W); put32(ihdr + 4, H);
    ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    writeChunk(f, "IHDR", ihdr, 13);
    writeChunk(f, "IDAT", comp, clen);
    writeChunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw); free(comp);
    return 1;
}

/* Reads back a PNG that writePng() produced. Comparing decoded pixels, not file
 * bytes, keeps the check independent of which zlib compressed the reference. */
static int readPng(const char *path, unsigned char *gray)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *file = (unsigned char *)malloc((size_t)size);
    if (fread(file, 1, (size_t)size, f) != (size_t)size) { fclose(f); free(file); return 0; }
    fclose(f);

    int ok = size > 8 && memcmp(file, "\x89PNG\r\n\x1a\n", 8) == 0;
    unsigned char *comp = (unsigned char *)malloc((size_t)size);
    unsigned long compLen = 0;
    int sizeOk = 0;

    for (long pos = 8; ok && pos + 12 <= size; ) {
        const unsigned long len = get32(file + pos);
        const char *type = (const char *)(file + pos + 4);
        const unsigned char *data = file + pos + 8;
        if (pos + 12 + (long)len > size) { ok = 0; break; }

        if (memcmp(type, "IHDR", 4) == 0) {
            sizeOk = get32(data) == W && get32(data + 4) == H &&
                     data[8] == 8 && data[9] == 0 && data[12] == 0;
        } else if (memcmp(type, "IDAT", 4) == 0) {
            memcpy(comp + compLen, data, len);
            compLen += len;
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        pos += 12 + (long)len;
    }
    ok = ok && sizeOk && compLen > 0;

    if (ok) {
        unsigned long rawLen = (unsigned long)(W + 1) * H;
        unsigned char *raw = (unsigned char *)malloc(rawLen);
        ok = uncompress(raw, &rawLen, comp, compLen) == Z_OK && rawLen == (unsigned long)(W + 1) * H;
        for (int y = 0; ok && y < H; y++) {
            ok = raw[(size_t)y * (W + 1)] == 0;             /* only unfiltered rows */
            if (ok) memcpy(gray + (size_t)y * W, raw + (size_t)y * (W + 1) + 1, W);
        }
        free(raw);
    }
    free(file); free(comp);
    return ok;
}

/* --------------------------------------------------------------------- main -- */

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        for (int i = 0; i < kScenarioCount; i++) printf("%s\n", kScenarios[i].name);
        return 0;
    }

    static unsigned char gray[W * H], golden[W * H];

    if (argc == 5 && strcmp(argv[1], "--check") == 0) {
        const Scenario *s = findScenario(argv[2]);
        if (!s) { fprintf(stderr, "unknown scenario '%s'\n", argv[2]); return 2; }
        renderToGray(s, gray);

        if (!readPng(argv[3], golden)) {
            fprintf(stderr, "DIFF  %s: reference image %s is missing or unreadable\n", argv[2], argv[3]);
            writePng(argv[4], gray);
            return 1;
        }
        long diff = 0;
        for (int i = 0; i < W * H; i++) diff += gray[i] != golden[i];
        if (diff == 0) { printf("match %s\n", argv[2]); return 0; }

        printf("DIFF  %s: %ld pixels differ\n", argv[2], diff);
        writePng(argv[4], gray);
        return 1;
    }

    if (argc == 3) {
        const Scenario *s = findScenario(argv[1]);
        if (!s) {
            fprintf(stderr, "unknown scenario '%s'. Try --list.\n", argv[1]);
            return 1;
        }
        renderToGray(s, gray);
        if (!writePng(argv[2], gray)) { fprintf(stderr, "cannot write %s\n", argv[2]); return 1; }
        printf("wrote %s\n", argv[2]);
        return 0;
    }

    fprintf(stderr, "usage: host_render --list | <scenario> <out.png> | --check <scenario> <golden.png> <actual.png>\n");
    return 2;
}
