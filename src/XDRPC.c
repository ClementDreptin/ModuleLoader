#include "XDRPC.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Log.h"

#define RESPONSE_SIZE 512

// Calculate the size of string rounded up to the closest multiple of 8.
static size_t SizeOfString(const char *string)
{
    size_t size = strnlen_s(string, MAX_PATH);

    while (size % sizeof(uint64_t) != 0)
        size++;

    return size;
}

// Write the characters pointed by string at the buffer pointed by ppBuffer and increment it by the
// size of string rounded up to the closest multiple of 8.
static void WriteString(void **ppBuffer, const char *string)
{
    size_t numberOfCharacters = strnlen_s(string, MAX_PATH);
    size_t sizeOfStringInBuffer = SizeOfString(string);

    memcpy(*ppBuffer, string, numberOfCharacters);

    *ppBuffer = (byte *)*ppBuffer + sizeOfStringInBuffer;
}

// Write data at the buffer pointed by ppBuffer and increment it by the size of a uint64_t.
static void WriteUInt64(void **ppBuffer, uint64_t data)
{
    *(uint64_t *)*ppBuffer = _byteswap_uint64(data);

    *ppBuffer = (uint64_t *)*ppBuffer + 1;
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

HRESULT Call(const char *moduleName, uint32_t ordinal, XdrpcArgInfo *args, size_t numberOfArgs)
{
    HRESULT hr = S_OK;
    size_t i = 0;
    BOOL hasStringArgs = FALSE;

    char command[60] = { 0 };
    const char commandFormat[] = "rpc system version=4 buf_size=%d processor=5 thread=\r\n";

    char response[RESPONSE_SIZE] = "204- buf_addr=3A174C30\r\n";
    size_t responseSize = RESPONSE_SIZE;

    size_t moduleNameSize = 0;

    uint64_t bufferAddress = 0;
    uint64_t firstBufferAddress = 0;
    uint64_t secondBufferAddress = 0;

    byte *buffer = NULL;
    byte *pBuffer = NULL;
    size_t bufferSize = 0;

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
        bufferSize += sizeof(uint64_t);

    // Increase the size of the buffer to allow all arguments to fit
    for (i = 0; i < numberOfArgs; i++)
        bufferSize += args[i].Size;

    // Increase the size of the buffer to fit the module name
    bufferSize += moduleNameSize;

    // Create the command from the format and the buffer size
    _snprintf_s(command, sizeof(command), _TRUNCATE, commandFormat, bufferSize);

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

    // Let at least 0x48 bytes between firstBufferAddress and the buffer address returned when the thread was created,
    // I don't know why...
    firstBufferAddress = bufferAddress + 0x48;

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

    PrintBuffer(buffer, bufferSize);

    free(buffer);

    return hr;
}
