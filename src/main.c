#include <stdio.h>

void ShowHelp(void);

int main()
{
    // Just display the help message all the time for now
    ShowHelp();

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
