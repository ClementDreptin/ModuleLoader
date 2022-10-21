# ModuleLoader
ModuleLoader is a command line tool to load/unload modules from an RGH/Jtag/Devkit Xbox 360 without having to reboot.

# Requirements
- Having the Xbox 360 Software Development Kit installed.
- Xbox 360 Neighborhood set up with your RGH/Jtag/Devkit registered as the default console.

## Installation
You can download the latest binary from the [releases](https://github.com/ClementDreptin/ModuleLoader/releases) or clone the repository and open `ModuleLoader.sln` in Visual Studio to build from source.

## Documentation
- `-h`: Show usage.
- `-s`: Show loaded modules.
- `<module_path>`: If `<module_path>` is already loaded, it will be unloaded then loaded back, otherwise it will just be loaded.
- `-l <module_path>`: Load the module located at `<module_path>` (absolute path).
- `-u <module_path>`: Unload the module located at `<module_path>` (absolute path).
