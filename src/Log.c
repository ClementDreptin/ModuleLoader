#include "Log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOG_BUFFER_SIZE 2048

static void Print(FILE *pOutputStream, const char *prefix, const char *format, const va_list args)
{
    char finalFormat[MAX_LOG_BUFFER_SIZE] = { 0 };

    // Build the final form of the format ([<prefix>]: <format>)
    _snprintf_s(finalFormat, MAX_LOG_BUFFER_SIZE, _TRUNCATE, "[%s]: %s", prefix, format);

    // Format the full message and write it to pOutputStream
    vfprintf_s(pOutputStream, finalFormat, args);
}

void LogSuccess(const char *message, ...)
{
    // Get the variadic arguments
    va_list args = NULL;
    va_start(args, message);

    // Print
    Print(stdout, "Success", message, args);

    // Free the variadic arguments
    va_end(args);
}

void LogError(const char *message, ...)
{
    // Get the variadic arguments
    va_list args = NULL;
    va_start(args, message);

    // Print
    Print(stderr, "Error", message, args);

    // Free the variadic arguments
    va_end(args);
}

void LogInfo(const char *message, ...)
{
    // Get the variadic arguments
    va_list args = NULL;
    va_start(args, message);

    // Print
    Print(stdout, "Info", message, args);

    // Free the variadic arguments
    va_end(args);
}
