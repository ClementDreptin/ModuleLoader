#pragma once

#include <Windows.h>

// Print a list of the loaded modules on the console.
HRESULT ShowLoadedModuleNames(void);

// Debug function to try XDRPC communication
HRESULT TestXNotify(void);
