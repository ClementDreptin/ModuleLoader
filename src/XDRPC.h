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
    void *pData;
    XdrpcArgType Type;
    size_t Size;
} XdrpcArgInfo;

HRESULT Call(const char *moduleName, uint32_t ordinal, XdrpcArgInfo *args, size_t numberOfArgs);
