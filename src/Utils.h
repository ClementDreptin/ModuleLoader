#pragma once

#include <Windows.h>

// Print the different command line options with their descriptions.
void ShowUsage(void);

// Append %XEDK%/bin/win32 to %PATH% so that xbdm.dll can be found and delay loaded.
HRESULT AddXdkBinDirToPath(void);

// Translate the HRESULT hr into an error message and write it to stderr.
void LogXbdmError(HRESULT hr);
