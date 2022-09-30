#include "Modules.h"

#include <stdio.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

#include "Log.h"

static void LogXbdmError(HRESULT hr)
{
    char errorMsg[200] = { 0 };

    DmTranslateError(hr, errorMsg, sizeof(errorMsg));

    LogError(errorMsg);
}

HRESULT ShowLoadedModuleNames(void)
{
    HRESULT hr = S_OK;
    PDM_WALK_MODULES pModuleWalker = NULL;
    DMN_MODLOAD loadedModule = { 0 };

    // Go through the loaded modules and print their names
    while ((hr = DmWalkLoadedModules(&pModuleWalker, &loadedModule)) == XBDM_NOERR)
        puts(loadedModule.Name);

    // Error handling
    if (hr != XBDM_ENDOFLIST)
    {
        LogXbdmError(hr);
        DmCloseLoadedModules(pModuleWalker);

        return hr;
    }

    // Free the memory allocated by DmWalkLoadedModules
    DmCloseLoadedModules(pModuleWalker);

    return S_OK;
}
