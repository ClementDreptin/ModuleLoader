#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Log.h"

void ShowUsage(void);

int main(int argc, char **argv)
{
    // The first char * of argv is the name of the program so the number of arguments is argc - 1
    size_t nNumberOfArguments = argc - 1;

    // Case of using ModuleLoader without providing any arguments
    if (nNumberOfArguments == 0)
    {
        ShowUsage();
        return EXIT_SUCCESS;
    }

    // Check to make sure not more than 2 arguments are passed
    if (nNumberOfArguments > 2)
    {
        LogError("Maximum number of arguments exceeded. ModuleLoader -h to see the usage.");
        return EXIT_FAILURE;
    }

    // Case of using ModuleLoader by providing a module path
    if (argv[1][0] != '-')
    {
        LogInfo("Loading %s...", argv[1]);
        return EXIT_SUCCESS;
    }

    // Cases of using ModuleLoader with a flag

    // Usage
    if (!strcmp(argv[1], "-h"))
    {
        ShowUsage();
        return EXIT_SUCCESS;
    }

    // Module list
    if (!strcmp(argv[1], "-s"))
    {
        puts("Module1\nModule2\nModule3");
        return EXIT_SUCCESS;
    }

    // Loading
    if (!strcmp(argv[1], "-l"))
    {
        if (nNumberOfArguments < 2)
        {
            LogError("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
            return EXIT_FAILURE;
        }

        LogInfo("Loading %s...", argv[2]);

        return EXIT_SUCCESS;
    }

    // Unloading
    if (!strcmp(argv[1], "-u"))
    {
        if (nNumberOfArguments < 2)
        {
            LogError("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
            return EXIT_FAILURE;
        }

        LogInfo("Unloading %s...", argv[2]);

        return EXIT_SUCCESS;
    }

    // Invalid flag
    LogError("%s is not a valid argument. ModuleLoader -h to see the usage.", argv[1]);

    return EXIT_FAILURE;
}

void ShowUsage(void)
{
    const char *szUsage =
        "Usage:\n"
        "    -h:               Show usage.\n"
        "\n"
        "    -s:               Show loaded modules (NOT YET IMPLEMENTED).\n"
        "\n"
        "    <module_path>:    If <module_path> is already loaded, it will be unloaded then\n"
        "                      loaded back, otherwise it will just be loaded (NOT YET IMPLEMENTED).\n"
        "\n"
        "    -l <module_path>: Load the module located at <module_path> (absolute path) (NOT YET IMPLEMENTED).\n"
        "\n"
        "    -u <module_path>: Unload the module located at <module_path> (absolute path) (NOT YET IMPLEMENTED).";

    puts(szUsage);
}
