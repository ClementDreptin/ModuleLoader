# ModuleLoader

ModuleLoader is a command line tool to load/unload modules from an RGH/Jtag/Devkit Xbox 360 without having to reboot.

## Requirements

-   Having the Xbox 360 Software Development Kit installed.
-   Xbox 360 Neighborhood set up with your RGH/Jtag/Devkit registered as the default console.
-   `XDRPC.xex` as a loaded plugin (not needed on devkit).

## Installation

You can download the latest binary from the [releases](https://github.com/ClementDreptin/ModuleLoader/releases) or clone the repository and open `ModuleLoader.sln` in Visual Studio to build from source.

## Documentation

-   `-h`: Show usage.
-   `-s`: Show loaded modules.
-   `-S`: Show loaded modules and their metadata (verbose).
-   `<module_path>`: If `<module_path>` is already loaded, it will be unloaded then loaded back, otherwise it will just be loaded.
-   `-l <module_path>`: Load the module at an absolute drive path or native `\Device\` path.
-   `-u <module_name>`: Unload the module named `<module_name>`. An absolute path is also accepted; its file name is used for lookup.

## Native device paths

Use a native path if a drive alias such as `USB0:` does not resolve when loading:

```powershell
.\ModuleLoader.exe -l '\Device\Mass0\Plugins\Example.xex'
```

`Mass0` is the first USB storage device. Native paths are validated by the console
kernel instead of XBDM's drive-alias file check.
