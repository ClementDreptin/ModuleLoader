#include "Utils.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>

void ShowUsage(void)
{
    const char *usage =
        "Usage:\n"
        "    -h:               Show usage.\n"
        "\n"
        "    -s:               Show loaded modules.\n"
        "\n"
        "    <module_path>:    If <module_path> is already loaded, it will be unloaded then\n"
        "                      loaded back, otherwise it will just be loaded.\n"
        "\n"
        "    -l <module_path>: Load the module located at <module_path> (absolute path).\n"
        "\n"
        "    -u <module_path>: Unload the module located at <module_path> (absolute path).";

    puts(usage);
}

HRESULT AddXdkBinDirToPath(void)
{
    char *originalPath = NULL;
    size_t originalPathSize = 0;
    char *xdkDir = NULL;
    size_t xdkDirSize = 0;
    char *newPath = NULL;
    size_t newPathSize = 0;
    char newPathFormat[] = "%s\\bin\\win32;%s";
    size_t newPathFormatSize = 0;
    errno_t err = 0;

    // Get the value of %PATH%
    err = _dupenv_s(&originalPath, &originalPathSize, "PATH");
    if (err)
    {
        LogError("Could not get the value of %PATH%.");
        return E_FAIL;
    }

    // Get the value of %XEDK%
    err = _dupenv_s(&xdkDir, &xdkDirSize, "XEDK");
    if (err)
    {
        LogError("Could not get the value of %XEDK%. Make sure the Xbox 360 Software Development Kit is properly installed.");
        return E_FAIL;
    }

    // Calculate the size of the new value of %PATH%
    newPathFormatSize = sizeof(newPathFormat);
    newPathSize = originalPathSize + xdkDirSize + newPathFormatSize;
    newPath = malloc(newPathSize);

    // Create the new value of %PATH% from the format
    _snprintf_s(newPath, newPathSize, _TRUNCATE, newPathFormat, xdkDir, originalPath);

    // Overwrite the value of %PATH%
    err = _putenv_s("PATH", newPath);
    if (err)
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
