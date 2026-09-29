/**
 * Placeholder contents, so screen layouts can be built and judged without a live
 * feed. The calendar page has no live source yet; the bin page uses
 * DummyBinData_Fill() only when include/secrets.h is not filled in.
 */
#ifndef DUMMY_DATA_H
#define DUMMY_DATA_H

#include "bin_data.h"
#include "calendar_data.h"

void DummyData_Fill(CalView *view);
void DummyBinData_Fill(BinView *view);

#endif
