#pragma once

#include <stdint.h>
#include <Windows.h>

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

HRESULT XdrpcCall(const char *moduleName, uint32_t ordinal, XdrpcArgInfo *args, size_t numberOfArgs, uint64_t *pReturnValue);
