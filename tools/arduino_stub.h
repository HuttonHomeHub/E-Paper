/* Minimal Arduino shim so the drawing code (which pulls in DEV_Config.h)
 * compiles on a normal desktop for the render harness. */
#ifndef ARDUINO_H_STUB
#define ARDUINO_H_STUB
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define HIGH 1
#define LOW  0
#define INPUT  0
#define OUTPUT 1
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int  digitalRead(int) { return 1; }
inline void delay(unsigned long) {}
struct SerialStub {
    void begin(unsigned long) {}
    void print(const char *s) { printf("%s", s); }
    void print(int v) { printf("%d", v); }
    void println(const char *s) { printf("%s\n", s); }
    void println() { printf("\n"); }
};
static SerialStub Serial;
#endif
