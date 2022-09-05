#include "Log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOG_BUFFER_SIZE 2048

static void Print(FILE *pOutputStream, const char *szPrefix, const char *szFormat, const va_list pArgList)
{
    char szFinalFormat[MAX_LOG_BUFFER_SIZE] = { 0 };

    // Build the final form of the format ([<prefix>]: <format>)
    _snprintf_s(szFinalFormat, MAX_LOG_BUFFER_SIZE, _TRUNCATE, "[%s]: %s", szPrefix, szFormat);

    // Format the full message and write it to pOutputStream
    vfprintf_s(pOutputStream, szFinalFormat, pArgList);
}

void LogSuccess(const char *szMessage, ...)
{
    // Get the variadic arguments
    va_list pArgList = NULL;
    va_start(pArgList, szMessage);

    // Print
    Print(stdout, "Success", szMessage, pArgList);

    // Free the variadic arguments
    va_end(pArgList);
}

void LogError(const char *szMessage, ...)
{
    // Get the variadic arguments
    va_list pArgList = NULL;
    va_start(pArgList, szMessage);

    // Print
    Print(stderr, "Error", szMessage, pArgList);

    // Free the variadic arguments
    va_end(pArgList);
}

void LogInfo(const char *szMessage, ...)
{
    // Get the variadic arguments
    va_list pArgList = NULL;
    va_start(pArgList, szMessage);

    // Print
    Print(stdout, "Info", szMessage, pArgList);

    // Free the variadic arguments
    va_end(pArgList);
}
