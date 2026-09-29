/**
 * Compact encoding of a bin collection for the on-flash cache, so the last good
 * schedule survives a failed refresh (and a power cut). One collection packs
 * into 32 bits: ((year * 100 + month) * 100 + day) * 10 + bin.
 */
#ifndef BIN_CACHE_H
#define BIN_CACHE_H

#include <stdint.h>

#include "bin_data.h"

uint32_t BinCache_Pack(const BinCollection *c);

/* Returns 1 and fills *c if `packed` is a valid collection, else 0 (so a
 * corrupt or foreign cache entry is rejected rather than trusted). */
int BinCache_Unpack(uint32_t packed, BinCollection *c);

/* A date as a single number, e.g. 20260929, for storing "last updated". */
uint32_t BinCache_DateKey(int year, int month, int day);

#endif
