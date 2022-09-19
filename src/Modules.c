#include "Modules.h"

#include <stdio.h>
#include <stdint.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

#include "Log.h"

#define RESPONSE_SIZE 512

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

HRESULT TestXNotify(void)
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
