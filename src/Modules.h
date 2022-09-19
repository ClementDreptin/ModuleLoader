#pragma once

#include <Windows.h>

// Print a list of the loaded modules on the console.
HRESULT ShowLoadedModuleNames(void);

// Load the module located at module path.
HRESULT Load(const char *modulePath);

// Debug function to try XDRPC communication
HRESULT TestXNotify(void);
