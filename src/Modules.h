#pragma once

#include <Windows.h>

// Print a list of the loaded modules on the console.
HRESULT ShowLoadedModuleNames(void);

// Load the module located at modulePath.
HRESULT Load(const char *modulePath);

// Unload the module located at modulePath.
HRESULT Unload(const char *modulePath);

// Unload then load back the module located at modulePath.
HRESULT UnloadThenLoad(const char *modulePath);
