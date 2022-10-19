#pragma once

#include <Windows.h>

// Check if the module located at modulePath is loaded on the console.
HRESULT IsModuleLoaded(const char *modulePath, BOOL *pIsLoaded);

// Print a list of the loaded modules on the console.
HRESULT ShowLoadedModuleNames(void);

// Load the module located at modulePath.
HRESULT Load(const char *modulePath);

// Unload the module located at modulePath.
HRESULT Unload(const char *modulePath);
