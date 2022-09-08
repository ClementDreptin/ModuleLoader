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
    char szErrorMsg[200] = { 0 };

    DmTranslateError(hr, szErrorMsg, sizeof(szErrorMsg));

    LogError(szErrorMsg);
}

HRESULT ShowLoadedModuleNames(void)
{
    HRESULT hr = S_OK;
    PDM_WALK_MODULES pWalkModule = NULL;
    DMN_MODLOAD LoadedModule = { 0 };

    // Go through the loaded modules and print their names
    while ((hr = DmWalkLoadedModules(&pWalkModule, &LoadedModule)) == XBDM_NOERR)
        puts(LoadedModule.Name);

    // Error handling
    if (hr != XBDM_ENDOFLIST)
    {
        LogXbdmError(hr);
        DmCloseLoadedModules(pWalkModule);

        return hr;
    }

    // Free the memory allocated by DmWalkLoadedModules
    DmCloseLoadedModules(pWalkModule);

    return S_OK;
}
