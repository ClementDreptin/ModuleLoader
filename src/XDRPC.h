#pragma once

#include <Windows.h>
#include <stdint.h>

typedef enum _XdrpcArgType
{
    XdrpcArgType_Integer,
    XdrpcArgType_String,
} XdrpcArgType;

typedef struct _XdrpcArgInfo
{
    const void *pData;
    XdrpcArgType Type;
} XdrpcArgInfo;

// Call the function at ordinal in moduleName and pass it numberOfArgs arguments coming from args.
HRESULT XdrpcCall(const char *moduleName, uint32_t ordinal, XdrpcArgInfo *args, size_t numberOfArgs, uint64_t *pReturnValue);
