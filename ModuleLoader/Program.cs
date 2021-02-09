using System;

namespace ModuleLoader
{
	class Program
	{
		static ModuleLoader ModuleLoader = new ModuleLoader();
		delegate void ModuleCallBack(string moduleName);

		static void Main(string[] args)
		{
			if (args.Length == 0)
			{
				ModuleLoader.ShowHelp();
				return;
			}

			if (args.Length > 2)
			{
				XboxUtils.ErrorMessage("Maximum number of arguments exceeded. ModuleLoader -h to see the usage.");
				return;
			}

			string firstArg = (args.Length > 0 && args[0] != null) ? args[0] : null;
			string secondArg = (args.Length > 1 && args[1] != null) ? args[1] : null;

			if (firstArg.StartsWith("-"))
			{
				switch (firstArg)
				{
					case "-h":
						ModuleLoader.ShowHelp();
						break;
					case "-s":
						ModuleAction(Show, null);
						break;
					case "-l":
						if (secondArg != null)
							ModuleAction(Load, secondArg);
						else
							XboxUtils.ErrorMessage("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
						break;
					case "-u":
						if (secondArg != null)
							ModuleAction(Unload, secondArg);
						else
							XboxUtils.ErrorMessage("You need to specify an absolute module path. ModuleLoader -h to see the usage.");
						break;
					default:
						XboxUtils.ErrorMessage(firstArg + " is not a valid argument. ModuleLoader -h to see the usage.");
						break;
				}
			}
			else
			{
				if (secondArg != null)
					XboxUtils.ErrorMessage("You can not specify argument after the module path. ModuleLoader -h to see the usage.");
				else
					ModuleAction(UnloadThenLoad, firstArg);
			}
		}

		static void Show(string moduleName)
		{
			ModuleLoader.ShowModuleNames();
		}

		static void Load(string moduleName)
		{
			ModuleLoader.LoadModule(moduleName);
		}

		static void Unload(string moduleName)
		{
			ModuleLoader.UnloadModule(moduleName);
		}

		static void UnloadThenLoad(string moduleName)
		{
			if (ModuleLoader.IsModuleLoaded(moduleName))
				ModuleLoader.UnloadModule(moduleName);

			ModuleLoader.LoadModule(moduleName);
		}

		static void ModuleAction(ModuleCallBack moduleCallBack, string moduleName)
		{
			if (ModuleLoader.Connect())
			{
				try
				{
					moduleCallBack(moduleName);
				}
				catch (Exception exception)
				{
					XboxUtils.ErrorMessage("An error occured. Full message: " + exception.Message);
				}

				ModuleLoader.Disconnect();
			}
		}
	}
}
