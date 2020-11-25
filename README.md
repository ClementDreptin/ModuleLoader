# ModuleLoader
ModuleLoader is a command line tool to load/unload modules from an RGH/Jtag/Devkit Xbox 360 without having to reboot.

## Requirements
You must have the Xbox 360 SDK (XDK) installed. If you have another version than 2.0.21256.0 you will need to replace the dll files from the `lib` folder with the ones from your SDK (located at `\path\to\XDK\bin\win32\`).

## Documentation
- `-h`: Show help.
- `<module_path>`: If `<module_path>` is already loaded, it will be unloaded then loaded back, otherwise it will just be loaded.
- `-l <module_path>`: Load the module located at `<module_path>` (absolute path).
- `-u <module_path>`: Unload the module located at `<module_path>` (absolute path).