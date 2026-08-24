/**
 * Host-side render harness. Builds the same calendar screen the ESP32 draws,
 * using the real GUI_Paint code and the real renderer, and writes it out as a
 * PNG. Lets the layout be reviewed without flashing hardware.
 *
 * Build and run with tools/render.sh
 */
#include "GUI_Paint.h"
#include "calendar_render.h"
#include "dummy_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define W 800
#define H 480

static void put32(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);  p[3] = (unsigned char)v;
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

int main(int argc, char **argv)
{
    const char *out = (argc > 1) ? argv[1] : "calendar.png";

    UBYTE *buf = (UBYTE *)malloc((size_t)(W / 8) * H);
    Paint_NewImage(buf, W, H, ROTATE_0, WHITE);
    Paint_SelectImage(buf);

    CalView view;
    DummyData_Fill(&view);
    CalendarRender_Draw(&view);

    /* Paint_Clear(WHITE) fills 0xFF, so bit 1 is white and bit 0 is black ink. */
    unsigned char *raw = (unsigned char *)malloc((size_t)(W + 1) * H);
    for (int y = 0; y < H; y++) {
        raw[(size_t)y * (W + 1)] = 0;                       /* filter: none */
        for (int x = 0; x < W; x++) {
            int bit = (buf[y * (W / 8) + (x / 8)] >> (7 - (x % 8))) & 1;
            raw[(size_t)y * (W + 1) + 1 + x] = bit ? 255 : 0;
        }
    }

    unsigned long clen = compressBound((unsigned long)(W + 1) * H);
    unsigned char *comp = (unsigned char *)malloc(clen);
    compress(comp, &clen, raw, (unsigned long)(W + 1) * H);

    FILE *f = fopen(out, "wb");
    fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
    unsigned char ihdr[13];
    put32(ihdr, W); put32(ihdr + 4, H);
    ihdr[8] = 8; ihdr[9] = 0; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    writeChunk(f, "IHDR", ihdr, 13);
    writeChunk(f, "IDAT", comp, clen);
    writeChunk(f, "IEND", NULL, 0);
    fclose(f);

    printf("wrote %s\n", out);
    return 0;
}
