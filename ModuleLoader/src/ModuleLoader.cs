using System;
using System.Collections.Generic;
using XDevkit;
using Microsoft.Test.Xbox.XDRPC;

namespace ModuleLoader
{
	class ModuleLoader
	{
		private bool activeConnection = false;
		private uint xboxConnection = 0;

		private IXboxManager xbManager = null;
		private IXboxConsole xbConsole = null;

		private string debuggerName = null;
		private string userName = null;
		private byte[] memoryBuffer = new byte[32];
		private uint outInt = 0;

		public bool Connect()
		{
			xbManager = new XboxManager();
			xbConsole = xbManager.OpenConsole(xbManager.DefaultConsole);

			try
			{
				xboxConnection = xbConsole.OpenConnection(null);
			}
			catch (Exception)
			{
				Print("error", "Could not connect to console: " + xbManager.DefaultConsole);
				return false;
			}

			if (xbConsole.DebugTarget.IsDebuggerConnected(out debuggerName, out userName))
			{
				activeConnection = true;
				Print("success", "Successfully connected to console: " + xbConsole.Name);
				return true;
			}
			else
			{
				xbConsole.DebugTarget.ConnectAsDebugger("PluginLoader", XboxDebugConnectFlags.Force);
				if (!xbConsole.DebugTarget.IsDebuggerConnected(out debuggerName, out userName))
				{
					Print("error", "Could not connect a debugger to console: " + xbConsole.Name);
					return false;
				}
				else
				{
					activeConnection = true;
					Print("success", "Successfully connected to console: " + xbConsole.Name);
					return true;
				}
			}
		}

		public bool Disconnect()
		{
			if (xbConsole.DebugTarget.IsDebuggerConnected(out debuggerName, out userName))
				xbConsole.DebugTarget.DisconnectAsDebugger();

			if (xboxConnection != 0)
			{
				xbConsole.CloseConnection(xboxConnection);
				Print("success", "Successfully disconnected from console: " + xbConsole.Name);
				return true;
			}

			Print("error", "Could not disconnect from console: " + xbConsole.Name);
			return false;
		}

		public List<string> GetModuleNames()
		{
			List<string> moduleNames = new List<string>();

			foreach (IXboxModule module in xbConsole.DebugTarget.Modules)
				moduleNames.Add(module.ModuleInfo.Name);

			return moduleNames;
		}

		public void LoadModule(string moduleName)
		{
			if (moduleName.Contains("\\") == false)
				moduleName = "Hdd:\\" + moduleName;

			if (!FileExists(moduleName))
				return;

			if (IsModuleLoaded(moduleName))
			{
				Print("error", moduleName + " is already loaded.");
				return;
			}

			try
			{
				xbConsole.ExecuteRPC<uint>(XDRPCMode.System, "xboxkrnl.exe", 409, new object[] { moduleName, 8, 0, 0 });
				Print("success", moduleName + " has been loaded.");
			}
			catch (Exception)
			{
				Print("error", "Could not load module: " + moduleName);
			}
		}

		public void UnloadModule(string moduleName)
		{
			if (!FileExists(moduleName))
				return;

			if (!IsModuleLoaded(moduleName))
			{
				Print("error", moduleName + " is not loaded.");
				return;
			}

			try
			{
				uint handle = xbConsole.ExecuteRPC<uint>(XDRPCMode.System, "xam.xex", 1102, new object[] { moduleName });
				if (handle != 0u)
				{
					WriteInt16(handle + 0x40, 1);
					xbConsole.ExecuteRPC<uint>(XDRPCMode.System, "xboxkrnl.exe", 417, new object[] { handle });
					Print("success", moduleName + " has been unloaded.");
				}
				else
					Print("error", "Could not unload module: " + moduleName);
			}
			catch (Exception)
			{
				Print("error", "Could not unload module: " + moduleName);
			}
		}

		public bool IsModuleLoaded(string moduleName)
		{
			List<string> modules = GetModuleNames();
			foreach (string module in modules)
				if (moduleName.Contains(module))
					return true;

			return false;
		}

		public void Print(string type, string message)
		{
			Console.WriteLine("[ModuleLoader - " + type.ToUpper() + "]: " + message);
		}

		public void ShowHelp()
		{
			Console.WriteLine("-h:               Show help.");
			Console.WriteLine("");

			Console.WriteLine("<module_path>:    If <module_path> is already loaded, it will be unloaded then");
			Console.WriteLine("                  loaded back, otherwise it will just be loaded.");
			Console.WriteLine("");

			Console.WriteLine("-l <module_path>: Load the module located at <module_path> (absolute path).");
			Console.WriteLine("");

			Console.WriteLine("-u <module_path>: Unload the module located at <module_path> (absolute path).");
		}

		private void WriteInt16(uint offset, short input)
		{
			BitConverter.GetBytes(input).CopyTo(memoryBuffer, 0);
			Array.Reverse(memoryBuffer, 0, 2);
			xbConsole.DebugTarget.SetMemory(offset, 2, memoryBuffer, out outInt);
		}

		public bool FileExists(string filePath)
		{
			try
			{
				IXboxFile file = xbConsole.GetFileObject(filePath);
				return true;
			}
			catch (Exception)
			{
				Print("error", filePath + " does not exist.");
				return false;
			}
		}
	}
}
