#pragma once

#include <Windows.h>

void ShowUsage(void);

HRESULT AddXdkBinDirToPath(void);

void LogXbdmError(HRESULT hr);

void TimestampToDateString(time_t timestamp, char *date, size_t dateSize);
