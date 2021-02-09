using System;
using System.Collections.Generic;
using XDevkit;
using Microsoft.Test.Xbox.XDRPC;

namespace ModuleLoader
{
	class ModuleLoader
	{
		public bool Connect()
		{
			try
			{
				XboxUtils.Connect();
				XboxUtils.ConfirmMessage("Successfully connected to console: " + XboxUtils.GetConsoleName());
				return true;
			}
			catch (Exception exception)
			{
				XboxUtils.ErrorMessage(exception.Message);
				return false;
			}
		}

		public bool Disconnect()
		{
			if (!XboxUtils.IsConnected())
				return true;

			try
			{
				string consoleName = XboxUtils.GetConsoleName();
				XboxUtils.Disconnect();
				XboxUtils.ConfirmMessage("Successfully disconnected from console: " + consoleName);
				return true;
			}
			catch (Exception exception)
			{
				XboxUtils.ErrorMessage(exception.Message);
				return false;
			}
		}

		public void ShowModuleNames()
		{
			List<string> moduleNames = XboxUtils.GetModuleNames();

			foreach (string module in moduleNames)
				Console.WriteLine(module);
		}

		public void LoadModule(string moduleName)
		{
			if (moduleName.Contains("\\") == false)
				moduleName = "Hdd:\\" + moduleName;

			if (!FileExists(moduleName))
				return;

			if (IsModuleLoaded(moduleName))
			{
				XboxUtils.ErrorMessage(moduleName + " is already loaded.");
				return;
			}

			try
			{
				XboxUtils.Call<uint>("xboxkrnl.exe", 409, new object[] { moduleName, 8, 0, 0 });
				XboxUtils.ConfirmMessage(moduleName + " has been loaded.");
			}
			catch (Exception)
			{
				XboxUtils.ErrorMessage("Could not load module: " + moduleName);
			}
		}

		public void UnloadModule(string moduleName)
		{
			if (!FileExists(moduleName))
				return;

			if (!IsModuleLoaded(moduleName))
			{
				XboxUtils.ErrorMessage(moduleName + " is not loaded.");
				return;
			}

			try
			{
				uint handle = XboxUtils.Call<uint>("xam.xex", 1102, new object[] { moduleName });
				if (handle != 0u)
				{
					XboxUtils.WriteShort(handle + 0x40, 1);
					XboxUtils.Call<uint>("xboxkrnl.exe", 417, new object[] { handle });
					XboxUtils.ConfirmMessage(moduleName + " has been unloaded.");
				}
				else
					XboxUtils.ErrorMessage("Could not unload module: " + moduleName);
			}
			catch (Exception)
			{
				XboxUtils.ErrorMessage("Could not unload module: " + moduleName);
			}
		}

		public bool IsModuleLoaded(string moduleName)
		{
			try
			{
				List<string> modules = XboxUtils.GetModuleNames();
				foreach (string module in modules)
					if (moduleName.Contains(module))
						return true;
			}
			catch (Exception exception)
			{
				XboxUtils.ErrorMessage(exception.Message);
			}

			return false;
		}

		public void ShowHelp()
		{
			Console.WriteLine("-h:               Show help.");
			Console.WriteLine("");

			Console.WriteLine("-s:               Show loaded modules.");
			Console.WriteLine("");

			Console.WriteLine("<module_path>:    If <module_path> is already loaded, it will be unloaded then");
			Console.WriteLine("                  loaded back, otherwise it will just be loaded.");
			Console.WriteLine("");

			Console.WriteLine("-l <module_path>: Load the module located at <module_path> (absolute path).");
			Console.WriteLine("");

			Console.WriteLine("-u <module_path>: Unload the module located at <module_path> (absolute path).");
		}

		public bool FileExists(string filePath)
		{
			try
			{
				IXboxFile file = XboxUtils.GetFile(filePath);
				return true;
			}
			catch (Exception)
			{
				XboxUtils.ErrorMessage(filePath + " does not exist.");
				return false;
			}
		}
	}
}
