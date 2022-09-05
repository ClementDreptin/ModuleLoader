#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Log.h"

void ShowHelp(void);

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        ShowHelp();
        return EXIT_SUCCESS;
    }

    if (argc > 3)
    {
        LogError("Maximum number of arguments exceeded. ModuleLoader -h to see the usage.");
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "-h"))
    {
        ShowHelp();
        return EXIT_SUCCESS;
    }

    return 0;
}

void ShowHelp(void)
{
    const char *szHelp =
        "-h:               Show help (NOT YET IMPLEMENTED).\n"
        "\n"
        "-s:               Show loaded modules (NOT YET IMPLEMENTED).\n"
        "\n"
        "<module_path>:    If <module_path> is already loaded, it will be unloaded then\n"
        "                  loaded back, otherwise it will just be loaded (NOT YET IMPLEMENTED).\n"
        "\n"
        "-l <module_path>: Load the module located at <module_path> (absolute path) (NOT YET IMPLEMENTED).\n"
        "\n"
        "-u <module_path>: Unload the module located at <module_path> (absolute path) (NOT YET IMPLEMENTED).\n";

    puts(szHelp);
}
