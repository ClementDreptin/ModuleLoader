#pragma once

#include <Windows.h>

HRESULT ShowLoadedModules(void);

HRESULT Load(const char *modulePath);

HRESULT Unload(const char *modulePath);

HRESULT UnloadThenLoad(const char *modulePath);
