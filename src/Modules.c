#include "Modules.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

#include "Log.h"
#include "Utils.h"
#include "XDRPC.h"

static HRESULT FileExists(const char *filePath, BOOL *pFileExists)
{
    HRESULT hr = S_OK;

    DM_FILE_ATTRIBUTES fileAttributes = { 0 };

    hr = DmGetFileAttributes(filePath, &fileAttributes);

    if (hr == XBDM_NOERR)
    {
        *pFileExists = TRUE;
        return S_OK;
    }

    *pFileExists = FALSE;

    return hr;
}

static HRESULT GetFileNameFromPath(const char *filePath, char *fileName, size_t fileNameSize)
{
    errno_t err = 0;

    char baseName[MAX_PATH] = { 0 };
    char extension[MAX_PATH] = { 0 };

    // Isolate the base name and the extension of modulePath
    err = _splitpath_s(filePath, NULL, 0, NULL, 0, baseName, sizeof(baseName), extension, sizeof(extension));
    if (err != 0)
    {
        LogError("Could not split path: %s.", filePath);
        return E_FAIL;
    }

    // Build the file name (base name + extension)
    strncpy_s(fileName, fileNameSize, baseName, _TRUNCATE);
    strncat_s(fileName, fileNameSize, extension, _TRUNCATE);

    return S_OK;
}

static HRESULT IsModuleLoaded(const char *modulePath, BOOL *pIsLoaded)
{
    HRESULT hr = S_OK;

    PDM_WALK_MODULES pModuleWalker = NULL;
    DMN_MODLOAD loadedModule = { 0 };

    char fileName[MAX_PATH] = { 0 };

    // Get the file name from modulePath (base name + extension)
    hr = GetFileNameFromPath(modulePath, fileName, sizeof(fileName));
    if (FAILED(hr))
        return E_FAIL;

    // Go through the loaded modules and check if modulePath is in them
    while ((hr = DmWalkLoadedModules(&pModuleWalker, &loadedModule)) == XBDM_NOERR)
    {
        if (!strncmp(fileName, loadedModule.Name, sizeof(fileName)))
            *pIsLoaded = TRUE;
    }

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

static HRESULT XGetModuleHandleA(const char *modulePath, uint64_t *pHandle)
{
    XdrpcArgInfo args[1] = { { 0 } };

    args[0].pData = modulePath;
    args[0].Type = XdrpcArgType_String;

    return XdrpcCall("xam.xex", 1102, args, 1, pHandle);
}

static HRESULT XexLoadImage(const char *modulePath)
{
    XdrpcArgInfo args[4] = { { 0 }, { 0 }, { 0 }, { 0 } };
    uint64_t eight = 8;
    uint64_t zero = 0;

    args[0].pData = modulePath;
    args[0].Type = XdrpcArgType_String;
    args[1].pData = &eight;
    args[1].Type = XdrpcArgType_Integer;
    args[2].pData = &zero;
    args[2].Type = XdrpcArgType_Integer;
    args[3].pData = &zero;
    args[3].Type = XdrpcArgType_Integer;

    return XdrpcCall("xboxkrnl.exe", 409, args, 4, NULL);
}

static HRESULT XexUnloadImage(uint64_t moduleHandle)
{
    XdrpcArgInfo args[1] = { { 0 } };

    args[0].pData = &moduleHandle;
    args[0].Type = XdrpcArgType_Integer;

    return XdrpcCall("xboxkrnl.exe", 417, args, 1, NULL);
}

HRESULT Load(const char *modulePath)
{
    HRESULT hr = S_OK;

    BOOL moduleExists = FALSE;
    BOOL isModuleLoaded = FALSE;

    hr = FileExists(modulePath, &moduleExists);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    if (moduleExists == FALSE)
    {
        LogError("%s does not exist.", modulePath);
        return XBDM_NOSUCHFILE;
    }

    hr = IsModuleLoaded(modulePath, &isModuleLoaded);
    if (FAILED(hr))
        return E_FAIL;

    if (isModuleLoaded == TRUE)
    {
        LogError("%s is already loaded.", modulePath);
        return E_FAIL;
    }

    hr = XexLoadImage(modulePath);
    if (FAILED(hr))
        return E_FAIL;

    LogSuccess("%s has been loaded.", modulePath);

    return hr;
}

HRESULT Unload(const char *modulePath)
{
    HRESULT hr = S_OK;

    BOOL moduleExists = FALSE;
    BOOL isModuleLoaded = FALSE;

    uint64_t moduleHandle = 0;
    void *moduleHandlePatchAddress = NULL;
    uint16_t moduleHandlePatchValue = 1;
    size_t bytesWritten = 0;

    hr = FileExists(modulePath, &moduleExists);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    if (moduleExists == FALSE)
    {
        LogError("%s does not exist.", modulePath);
        return XBDM_NOSUCHFILE;
    }

    hr = IsModuleLoaded(modulePath, &isModuleLoaded);
    if (FAILED(hr))
        return E_FAIL;

    if (isModuleLoaded == FALSE)
    {
        LogError("%s is not loaded.", modulePath);
        return E_FAIL;
    }

    hr = XGetModuleHandleA(modulePath, &moduleHandle);
    if (FAILED(hr))
        return E_FAIL;

    if (moduleHandle == 0)
    {
        LogError("Handle of %s is invalid", modulePath);
        return E_FAIL;
    }

    moduleHandlePatchAddress = (void*)((uint32_t)moduleHandle + 0x40);

    // The Xbox 360 is in big-endian so we need to swap the bytes of the patch value before sending it
    moduleHandlePatchValue = _byteswap_ushort(moduleHandlePatchValue);

    // Before unloading a module, the short 0x0001 needs to be written at moduleHandle + 0x40, the console crashes otherwise
    hr = DmSetMemory(moduleHandlePatchAddress, sizeof(moduleHandlePatchValue), &moduleHandlePatchValue, (DWORD *)&bytesWritten);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    if (bytesWritten != sizeof(moduleHandlePatchValue))
    {
        LogError("Expected to write %d bytes at %p but only wrote %d.", sizeof(moduleHandlePatchValue), moduleHandlePatchAddress, bytesWritten);
        return E_FAIL;
    }

    hr = XexUnloadImage(moduleHandle);
    if (FAILED(hr))
        return E_FAIL;

    LogSuccess("%s has been unloaded.", modulePath);

    return hr;
}

HRESULT UnloadThenLoad(const char *modulePath)
{
    HRESULT hr = S_OK;

    hr = Unload(modulePath);
    if (FAILED(hr))
        return E_FAIL;

    hr = Load(modulePath);
    if (FAILED(hr))
        return E_FAIL;

    return hr;
}
