#include <stdio.h>
#include <string.h>

#include "Utils.h"
#include "Log.h"
#include "Modules.h"

int main(int argc, char **argv)
{
    HRESULT hr = S_OK;

    // The first char * of argv is the name of the program so the number of arguments is argc - 1
    size_t numberOfArguments = argc - 1;

    // Add the XDK bin directory to the path to successfully delay load xbdm.dll
    hr = AddXdkBinDirToPath();
    if (FAILED(hr))
        return EXIT_FAILURE;

    // Case of using ModuleLoader without providing any arguments
    if (numberOfArguments == 0)
    {
        ShowUsage();
        return EXIT_SUCCESS;
    }

    // Check to make sure not more than 2 arguments are passed
    if (numberOfArguments > 2)
    {
        LogError("Maximum number of arguments exceeded. ModuleLoader -h to see the usage.");
        return EXIT_FAILURE;
    }

    // Case of using ModuleLoader by just providing a module path
    if (argv[1][0] != '-')
        return UnloadThenLoad(argv[1]);

    // Cases of using ModuleLoader with a flag

    // Usage
    if (!strcmp(argv[1], "-h"))
    {
        ShowUsage();
        return EXIT_SUCCESS;
    }

    // Module list
    if (!strcmp(argv[1], "-s"))
        return ShowLoadedModules();

    // Loading
    if (!strcmp(argv[1], "-l"))
    {
        if (numberOfArguments < 2)
        {
            LogError("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
            return EXIT_FAILURE;
        }

        return Load(argv[2]);
    }

    // Unloading
    if (!strcmp(argv[1], "-u"))
    {
        if (numberOfArguments < 2)
        {
            LogError("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
            return EXIT_FAILURE;
        }

        return Unload(argv[2]);
    }

    // Invalid flag
    LogError("%s is not a valid argument. ModuleLoader -h to see the usage.", argv[1]);

    return EXIT_FAILURE;
}
