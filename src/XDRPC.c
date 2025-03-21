#include "XDRPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

#include "Log.h"
#include "Utils.h"

#define RESPONSE_SIZE 512

static size_t SizeOfString(const char *string)
{
    // Get the amount of characters in the string
    size_t size = strnlen_s(string, MAX_PATH);

    // Round up the size to the closest multiple of 8
    while (size % sizeof(uint64_t) != 0)
        size++;

    return size;
}

static void WriteString(void **ppBuffer, const char *string)
{
    size_t numberOfCharacters = strnlen_s(string, MAX_PATH);
    size_t sizeOfStringInBuffer = SizeOfString(string);

    memcpy(*ppBuffer, string, numberOfCharacters);

    *ppBuffer = (byte *)*ppBuffer + sizeOfStringInBuffer;
}

static void WriteUInt64(void **ppBuffer, uint64_t data)
{
    // The bytes of data need to be swapped because they are in little-endian on the PC but need to
    // be sent in big-endian to the console.
    *(uint64_t *)*ppBuffer = _byteswap_uint64(data);

    *ppBuffer = (uint64_t *)*ppBuffer + 1;
}

HRESULT XdrpcCall(const char *moduleName, uint32_t ordinal, XdrpcArgInfo *args, size_t numberOfArgs, uint64_t *pReturnValue)
{
    HRESULT hr = S_OK;

    size_t i = 0;
    BOOL hasStringArgs = FALSE;

    char command[60] = { 0 };
    const char commandFormat[] = "rpc system version=4 buf_size=%d processor=5 thread=\r\n";

    char response[RESPONSE_SIZE] = { 0 };
    size_t responseSize = RESPONSE_SIZE;

    size_t moduleNameSize = 0;

    uint64_t bufferAddress = 0;
    uint64_t firstBufferAddress = 0;
    uint64_t secondBufferAddress = 0;

    byte *buffer = NULL;
    byte *pBuffer = NULL;
    size_t bufferSize = 0;

    PDM_CONNECTION connection = NULL;

    moduleNameSize = SizeOfString(moduleName);

    // The buffer size needs to be 0x40, I don't know why...
    bufferSize = 0x40;

    // Check if any string arguments are passed
    for (i = 0; i < numberOfArgs; i++)
    {
        if (args[i].Type == XdrpcArgType_String)
        {
            hasStringArgs = TRUE;
            break;
        }
    }

    // It seems like a second buffer address needs to be in the buffer only when string arguments are passed,
    // I'm not really sure about this assumption...
    if (hasStringArgs == TRUE)
        bufferSize += sizeof(secondBufferAddress);

    // Increase the size of the buffer to allow all arguments to fit
    for (i = 0; i < numberOfArgs; i++)
        if (args[i].Type == XdrpcArgType_String)
            bufferSize += SizeOfString(args[i].pData);
        else if (args[i].Type == XdrpcArgType_Integer)
            bufferSize += sizeof(uint64_t);

    // Increase the size of the buffer to fit the module name
    bufferSize += moduleNameSize;

    // Create the command from the format and the buffer size
    _snprintf_s(command, sizeof(command), _TRUNCATE, commandFormat, bufferSize);

    // Open the XBDM connection
    hr = DmOpenConnection(&connection);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    // Send the command
    hr = DmSendCommand(connection, command, response, (DWORD *)&responseSize);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        return E_FAIL;
    }

    // Extract the buffer address from the response
    if (sscanf_s(response, "204- buf_addr=%x\r\n", &bufferAddress) != 1)
    {
        LogError("Unexpected response received: %s", response);
        return E_FAIL;
    }

    // Reset the response buffer for later
    ZeroMemory(response, RESPONSE_SIZE);
    responseSize = RESPONSE_SIZE;

    // Allocate enough memory to hold the main RPC buffer
    buffer = malloc(bufferSize);
    ZeroMemory(buffer, bufferSize);
    pBuffer = buffer;

    // Let at least 0x40 bytes between firstBufferAddress and the buffer address returned when the thread was created,
    // I don't know why...
    firstBufferAddress = bufferAddress + 0x40;

    // For each argument, we need to shift firstBufferAddress by 8 bytes
    for (i = 0; i < numberOfArgs; i++)
        firstBufferAddress += sizeof(uint64_t);

    // The buffer needs to have 32 zeros at first, so we just move the pointer 32 bytes forwards because the entire buffer
    // is already filled with zeros
    pBuffer += sizeof(uint64_t) * 4;

    // Write the number of arguments passed on the next 8 bytes
    WriteUInt64(&pBuffer, numberOfArgs);

    // Leave 8 zeros
    pBuffer += sizeof(uint64_t);

    // Write firstBufferAddress on the next 8 bytes
    WriteUInt64(&pBuffer, firstBufferAddress);

    // Write the ordinal on the next 8 bytes
    WriteUInt64(&pBuffer, ordinal);

    // Calculate and write the second buffer address in the buffer only if needed
    if (hasStringArgs == TRUE)
    {
        secondBufferAddress = firstBufferAddress + moduleNameSize;
        WriteUInt64(&pBuffer, secondBufferAddress);
    }

    // Append the integer arguments to the buffer
    for (i = 0; i < numberOfArgs; i++)
        if (args[i].Type == XdrpcArgType_Integer)
            WriteUInt64(&pBuffer, *(uint64_t *)(args[i].pData));

    // Copy the module name
    memcpy(pBuffer, moduleName, strnlen_s(moduleName, MAX_PATH));
    pBuffer += moduleNameSize;

    // Append the string arguments to the buffer
    for (i = 0; i < numberOfArgs; i++)
        if (args[i].Type == XdrpcArgType_String)
            WriteString(&pBuffer, args[i].pData);

    // Send the buffer
    hr = DmSendBinary(connection, buffer, bufferSize);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        free(buffer);

        return E_FAIL;
    }

    // Receive the response status
    hr = DmReceiveStatusResponse(connection, response, (DWORD *)&responseSize);
    if (FAILED(hr))
    {
        LogXbdmError(hr);
        free(buffer);

        return E_FAIL;
    }

    // Check if an error code was returned
    if (response[0] != '2')
    {
        LogError("Unexpected response received: %s", response);
        free(buffer);

        return E_FAIL;
    }

    // Fetch the response only if a return value is expected from the function
    if (pReturnValue != NULL)
    {
        // The return value is the second uint64_t in the buffer
        void *returnValueLocationInBuffer = buffer + sizeof(*pReturnValue);

        ZeroMemory(buffer, bufferSize);

        // 2 8-byte packets are sent before the actual response buffer, I don't know what information they
        // are supposed to hold...
        hr = DmReceiveBinary(connection, buffer, sizeof(uint64_t) * 2, NULL);
        if (FAILED(hr))
        {
            LogXbdmError(hr);
            free(buffer);

            return E_FAIL;
        }

        // Receive the actual response buffer (which is the buffer that was sent but with the return value in the second uint64_t)
        hr = DmReceiveBinary(connection, buffer, bufferSize, NULL);
        if (FAILED(hr))
        {
            LogXbdmError(hr);
            free(buffer);

            return E_FAIL;
        }

        // The bytes in the buffer need to be swapped because they are in big-endian and the PC's CPU has (most likely)
        // a little-endian architecture.
        *pReturnValue = _byteswap_uint64(*(uint64_t *)returnValueLocationInBuffer);
    }

    // Cleanup the heap allocated buffer
    free(buffer);

    // Close the XBDM connection
    DmCloseConnection(connection);

    return S_OK;
}

// ----------------------------------------------------------------
// Examples of buffer to construct to call different functions
// ----------------------------------------------------------------
//
//
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
//
//
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
//
//
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
