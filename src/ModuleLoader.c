#include <stdio.h>
#include <Windows.h>

// XBDM uses bit field types other than int which triggers a warning at warning level 4
// so we just disable it for XBDM
#pragma warning(push)
#pragma warning(disable : 4214)
#include <xbdm.h>
#pragma warning(pop)

int main()
{
    HRESULT hr = S_OK;
    DWORD dwXboxNameSize = MAX_PATH;
    char szXboxName[MAX_PATH];

    // Get the name of the default Xbox 360 set up in Neighborhood
    hr = DmGetNameOfXbox(szXboxName, &dwXboxNameSize, TRUE);
    if (FAILED(hr))
    {
        fputs("Could not connect to console\n", stderr);
        return EXIT_FAILURE;
    }

    printf("Successfully connected to console: \"%s\"\n", szXboxName);

    return EXIT_SUCCESS;
}
