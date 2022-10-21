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
#include "XDRPC.h"

#define RESPONSE_SIZE 512

static void LogXbdmError(HRESULT hr)
{
    char errorMsg[200] = { 0 };

    DmTranslateError(hr, errorMsg, sizeof(errorMsg));

    LogError(errorMsg);
}

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

HRESULT IsModuleLoaded(const char *modulePath, BOOL *pIsLoaded)
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

static void PrintBuffer(void *pBuffer, size_t bufferSize)
{
    size_t i = 0;

    for (i = 0; i < bufferSize; i++)
    {
        printf("%02hhX ", ((byte *)pBuffer)[i]);

        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf("  ");
    }

    printf("\n");
}

static void WriteUInt64(byte **ppBuffer, uint64_t data)
{
    *(uint64_t *)(*ppBuffer) = _byteswap_uint64(data);
    (*ppBuffer) += sizeof(uint64_t);
}

static void WriteWideChar(byte **ppBuffer, wchar_t data)
{
    *(wchar_t *)(*ppBuffer) = _byteswap_ushort((uint16_t)data);
    (*ppBuffer) += sizeof(wchar_t);
}

#define XBDM_ERR_CHECK(hr) \
    if (FAILED((hr))) \
    { \
        LogXbdmError((hr)); \
        return (hr); \
    }

// Call to XNotify
//
// - module: "xam.xex"
// - ordinal: 656
// - number of args: 5
// - 1st arg: 14
// - 2nd arg: 0
// - 3rd arg: 2
// - 4th arg: "hello"
// - 5th arg: 0
//
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00    <-- 16 empty bytes
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00    <-- 16 empty bytes
// 00 00 00 00 00 00 00 05   00 00 00 00 00 00 00 00    <-- num of args (5) | 8 empty bytes
// 00 00 00 00 3A 13 3C D8   00 00 00 00 00 00 02 90    <-- 1st address     | ordinal (656)
// 00 00 00 00 00 00 00 0E   00 00 00 00 00 00 00 00    <-- 1st arg (14)    | 2nd arg (0)
// 00 00 00 00 00 00 00 02   00 00 00 00 3A 13 3C E0    <-- 3rd arg (2)     | 2nd address
// 00 00 00 00 00 00 00 00   78 61 6D 2E 78 65 78 00    <-- last arg (0)    | module name ("xam.xex", 1 byte per character)
// 00 68 00 65 00 6C 00 6C   00 6F 00 00 00 00 00 00    <-- string argument ("hello", 2 bytes per character)

static HRESULT TestXNotify(void)
{
    HRESULT hr = S_OK;
    char response[RESPONSE_SIZE] = { 0 };
    DWORD responseSize = RESPONSE_SIZE;
    int i = 0;
    const char *command = "rpc system version=4 buf_size=128 processor=5 thread=\r\n";
    const char *moduleName = "xam.xex";
    const wchar_t *text = L"hello";
    size_t moduleNameSize = 0;
    uint32_t ordinal = 656;
    uint64_t bufferAddress = 0;
    uint64_t firstBufferAddress = 0;
    uint64_t secondBufferAddress = 0;
    uint32_t numberOfParams = 5;
    byte buffer[128] = { 0 };
    byte *pBuffer = buffer;
    size_t bufferSize = 128;
    PDM_CONNECTION conn = NULL;

    hr = DmOpenConnection(&conn);
    XBDM_ERR_CHECK(hr);

    hr = DmSendCommand(conn, command, response, &responseSize);
    XBDM_ERR_CHECK(hr);

    if (sscanf_s(response, "204- buf_addr=%x\r\n", &bufferAddress) != 1)
    {
        LogError("Unexpected response received: %s", response);
        return E_FAIL;
    }

    // Reset the response buffer for later
    ZeroMemory(response, RESPONSE_SIZE);
    responseSize = RESPONSE_SIZE;

    // Let at least 0x48 bytes between firstBufferAddress and the buffer address returned when the thread was created,
    // I don't know why...
    firstBufferAddress = bufferAddress + 0x48;

    // For each argument that is not a string, we need to shift firstBufferAddress by the amount of space taken by the argument (8 bytes),
    // in our case, there are 4 arguments that are not strings
    firstBufferAddress += sizeof(uint64_t) * 4;

    // The buffer needs to have 32 zeros at first, so we just move the pointer 32 bytes forwards because the entire buffer
    // is already filled with zeros
    pBuffer += 32;

    // Write the number of parameters passed on the next 8 bytes
    WriteUInt64(&pBuffer, numberOfParams);

    // Leave 8 zeros
    pBuffer += sizeof(uint64_t);

    // Write firstBufferAddress on the next 8 bytes
    WriteUInt64(&pBuffer, firstBufferAddress);

    // Write the ordinal on the next 8 bytes
    WriteUInt64(&pBuffer, ordinal);

    // First integer argument
    WriteUInt64(&pBuffer, 14ULL);

    // Second integer argument
    WriteUInt64(&pBuffer, 0ULL);

    // Third integer argument
    WriteUInt64(&pBuffer, 2ULL);

    // The amount of bytes between secondBufferAddress and firstBufferAddress needs to be a multiple of 8 and enough
    // to contain the module name, which is xam.xex
    secondBufferAddress = firstBufferAddress + 8;

    // Write secondBufferAddress
    WriteUInt64(&pBuffer, secondBufferAddress);

    // Last integer argument
    WriteUInt64(&pBuffer, 0ULL);

    // Copy the module name (1 byte per character)
    moduleNameSize = strnlen_s(moduleName, 10);
    memcpy(pBuffer, moduleName, moduleNameSize);
    pBuffer += moduleNameSize;

    // "xax.xex" is only 7 characters so we need to add an extra 0 to round up to 8
    *pBuffer = 0;
    pBuffer++;

    // The string argument (2 bytes per character)
    for (i = 0; i < (int)wcsnlen_s(text, 10); i++)
        WriteWideChar(&pBuffer, text[i]);

    // Send the buffer
    hr = DmSendBinary(conn, buffer, bufferSize);
    XBDM_ERR_CHECK(hr);

    hr = DmReceiveStatusResponse(conn, response, &responseSize);
    XBDM_ERR_CHECK(hr);

    if (response[0] != '2')
    {
        LogError("Unexpected response received: %s", response);
        return E_FAIL;
    }

    ZeroMemory(response, RESPONSE_SIZE);
    responseSize = RESPONSE_SIZE;

    hr = DmReceiveBinary(conn, response, 16, NULL);
    XBDM_ERR_CHECK(hr);

    ZeroMemory(buffer, bufferSize);

    hr = DmReceiveBinary(conn, buffer, bufferSize, NULL);
    XBDM_ERR_CHECK(hr);

    DmCloseConnection(conn);

    return hr;
}

// Call to GetModuleHandleA
//
// - module: "xam.xex"
// - ordinal: 1102
// - number of args: 1
// - 1st arg: modulePath
//
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 01   00 00 00 00 00 00 00 00   <-- num of args (1) | 8 empty bytes
// 00 00 00 00 3A 15 CC 58   00 00 00 00 00 00 04 4E   <-- 1st address     | ordinal (1102)
// 00 00 00 00 3A 15 CC 60   78 61 6D 2E 78 65 78 00   <-- 2nd address     | module name ("xam.xex")
// 68 64 64 3A 5C 50 6C 75   67 69 6E 73 5C 48 61 79   <-- string argument ("hdd:\Plugins\Hayzen.xex")
// 7A 65 6E 2E 78 65 78 00

static HRESULT XGetModuleHandleA(const char *modulePath, uint64_t *pHandle)
{
    XdrpcArgInfo args[1] = { { 0 } };

    args[0].pData = modulePath;
    args[0].Type = XdrpcArgType_String;

    return XdrpcCall("xam.xex", 1102, args, 1, pHandle);
}

// Call to XexLoadImage
//
// - module: "xboxkrnl.exe"
// - ordinal: 409
// - number of args: 4
// - 1st arg: modulePath
// - 2nd arg: 8
// - 3rd arg: 0
// - 4th arg: 0
//
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 04   00 00 00 00 00 00 00 00   <-- num of args (4) | 8 empty bytes
// 00 00 00 00 3A 12 D3 B0   00 00 00 00 00 00 01 99   <-- 1st address     | ordinal (409)
// 00 00 00 00 3A 12 D3 C0   00 00 00 00 00 00 00 08   <-- 2nd address     | 2nd arg (8)
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 3rd arg (0)     | 4th arg (0)
// 78 62 6F 78 6B 72 6E 6C   2E 65 78 65 00 00 00 00   <-- module name ("xboxkrnl.exe")
// 68 64 64 3A 5C 50 6C 75   67 69 6E 73 5C 48 61 79   <-- string argument ("hdd:\Plugins\Hayzen.xex")
// 7A 65 6E 2E 78 65 78 00

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

// Call to XexUnloadImage
//
// - module: "xboxkrnl.exe"
// - ordinal: 417
// - number of args: 1
// - 1st arg: moduleHandle
//
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   <-- 16 empty bytes
// 00 00 00 00 00 00 00 01   00 00 00 00 00 00 00 00   <-- num of args (1)  | 8 empty bytes
// 00 00 00 00 3A 17 4C 78   00 00 00 00 00 00 01 A1   <-- 1st address      | ordinal (417)
// 00 00 00 00 3A 14 FE 08   78 62 6F 78 6B 72 6E 6C   <-- 1st arg (handle) | module name ("xboxkrnl.exe")
// 2E 65 78 65 00 00 00 00

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
    uint16_t moduleHandlePatchValue = 1;
    DWORD bytesWritten = 0;

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

    moduleHandlePatchValue = _byteswap_ushort(moduleHandlePatchValue);
    hr = DmSetMemory((void *)((uint32_t)moduleHandle + 0x40), sizeof(uint16_t), &moduleHandlePatchValue, &bytesWritten);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    if (bytesWritten != sizeof(uint16_t))
    {
        LogError("Expected to write %d bytes at %x but only wrote %d.", sizeof(uint16_t), (uint32_t)moduleHandle + 0x40);
        return E_FAIL;
    }

    hr = XexUnloadImage(moduleHandle);
    if (FAILED(hr))
        return E_FAIL;

    LogSuccess("%s has been unloaded.", modulePath);

    return hr;
}
