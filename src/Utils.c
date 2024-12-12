#include "Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

#include "Log.h"

void ShowUsage(void)
{
    const char usage[] =
        "Usage:\n"
        "    -h:               Show usage.\n"
        "\n"
        "    -s:               Show loaded modules.\n"
        "\n"
        "    -S:               Show loaded modules and their metadata (verbose).\n"
        "\n"
        "    <module_path>:    If <module_path> is already loaded, it will be unloaded then\n"
        "                      loaded back, otherwise it will just be loaded.\n"
        "\n"
        "    -l <module_path>: Load the module located at <module_path> (absolute path).\n"
        "\n"
        "    -u <module_name>: Unload the module named <module_name>. <module_name> can also be an absolute path.";

    puts(usage);
}

HRESULT AddXdkBinDirToPath(void)
{
    errno_t err = 0;

    char *originalPath = NULL;
    size_t originalPathSize = 0;
    char *xdkDir = NULL;
    size_t xdkDirSize = 0;
    char *newPath = NULL;
    size_t newPathSize = 0;
    char newPathFormat[] = "%s\\bin\\win32;%s";

    // Get the value of %PATH%
    err = _dupenv_s(&originalPath, &originalPathSize, "PATH");
    if (err != 0)
    {
        LogError("Could not get the value of %PATH%.");
        return E_FAIL;
    }

    // Get the value of %XEDK%
    err = _dupenv_s(&xdkDir, &xdkDirSize, "XEDK");
    if (err != 0)
    {
        LogError("Could not get the value of %XEDK%. Make sure the Xbox 360 Software Development Kit is properly installed.");
        return E_FAIL;
    }

    // Calculate the size of the new value of %PATH%
    newPathSize = originalPathSize + xdkDirSize + sizeof(newPathFormat);
    newPath = malloc(newPathSize);

    // Create the new value of %PATH% from the format
    _snprintf_s(newPath, newPathSize, _TRUNCATE, newPathFormat, xdkDir, originalPath);

    // Overwrite the value of %PATH%
    err = _putenv_s("PATH", newPath);
    if (err != 0)
    {
        LogError("Could not change the value of %PATH%.");
        return E_FAIL;
    }

    // Cleanup
    free(newPath);
    free(originalPath);
    free(xdkDir);

    return S_OK;
}

void LogXbdmError(HRESULT hr)
{
    char errorMsg[200] = { 0 };

    DmTranslateError(hr, errorMsg, sizeof(errorMsg));

    LogError(errorMsg);
}

void TimestampToDateString(time_t timestamp, char *date, size_t dateSize)
{
    struct tm dateTime;
    localtime_s(&dateTime, &timestamp);
    strftime(date, dateSize, "%B %d, %Y (%H:%M:%S)", &dateTime);
}
