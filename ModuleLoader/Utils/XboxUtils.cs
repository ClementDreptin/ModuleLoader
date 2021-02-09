using System;
using XDevkit;
using Microsoft.Test.Xbox.XDRPC;
using System.Text;
using System.Collections.Generic;

public static class XboxUtils
{
	private static bool m_ActiveConnection = false;
	private static uint m_XboxConnection = 0;
	
	private static IXboxManager m_XboxManager = null;
	private static IXboxConsole m_XboxConsole = null;

	public enum TitleID : uint
	{
		COD4 = 0x415607E6,
		MW2 = 0x41560817,
		MW3 = 0x415608CB
	}

	public enum MPStringAddr : uint
	{
		COD4 = 0x82032AC4,
		MW2 = 0x82001270,
		MW3 = 0x82001458,
		COD4_ALPHA = 0x820019EC,
		MW2_ALPHA = 0x82001D38
	}

	public static void Connect()
	{
		m_XboxManager = new XboxManager();
		m_XboxConsole = m_XboxManager.OpenConsole(m_XboxManager.DefaultConsole);

		try
		{
			m_XboxConnection = m_XboxConsole.OpenConnection(null);
		}
		catch (Exception)
		{
			throw new Exception("Couldn't connect to console: " + m_XboxManager.DefaultConsole);
		}

		if (!m_XboxConsole.DebugTarget.IsDebuggerConnected(out string debuggerName, out string userName))
		{
			m_XboxConsole.DebugTarget.ConnectAsDebugger("XboxTool", XboxDebugConnectFlags.Force);

			if (!m_XboxConsole.DebugTarget.IsDebuggerConnected(out debuggerName, out userName))
				throw new Exception("Couldn't connect a debugger to console: " + m_XboxConsole.Name);
		}

		m_ActiveConnection = true;
	}

	public static void Disconnect()
	{
		if (!m_ActiveConnection)
			return;

		try
		{
			if (m_XboxConsole.DebugTarget.IsDebuggerConnected(out string debuggerName, out string userName))
				m_XboxConsole.DebugTarget.DisconnectAsDebugger();

			if (m_XboxConnection != 0)
			{
				m_XboxConsole.CloseConnection(m_XboxConnection);
				m_ActiveConnection = false;
			}
		}
		catch (Exception)
		{
			throw new Exception("Could not disconnect from console: " + m_XboxConsole.Name);
		}
	}

	public static void ErrorMessage(string message)
	{
		Console.WriteLine("[ModuleLoader - ERROR]: " + message);
	}

	public static void ConfirmMessage(string message)
	{
		Console.WriteLine("[ModuleLoader - SUCCESS]: " + message);
	}

	public static string GetConsoleName()
	{
		if (!IsConnected())
			throw new Exception("Not connected to a console!");

		return m_XboxConsole.Name;
	}

	public static List<string> GetModuleNames()
	{
		if (!IsConnected())
			throw new Exception("Not connected to a console!");

		List<string> moduleNames = new List<string>();

		foreach (IXboxModule module in m_XboxConsole.DebugTarget.Modules)
			moduleNames.Add(module.ModuleInfo.Name);

		return moduleNames;
	}

	public static IXboxFile GetFile(string filePath)
	{
		if (!IsConnected())
			throw new Exception("Not connected to a console!");

		IXboxFile file = null;

		try
		{
			file = m_XboxConsole.GetFileObject(filePath);
		}
		catch (Exception)
		{
			throw new Exception(filePath + " does not exist!");
		}

		return file;
	}

	public static T Call<T>(uint address, params object[] args) where T : struct
	{
		T returnValue = default;

		if (!IsConnected())
			return returnValue;

		try
		{
			returnValue = XDRPCMarshaler.ExecuteRPC<T>(m_XboxConsole, new XDRPCExecutionOptions(XDRPCMode.Title, address), args);
		}
		catch (Exception)
		{
			throw new Exception("Error while calling the function!");
		}

		return returnValue;
	}

	public static T Call<T>(string moduleName, int ordinal, params object[] args) where T : struct
	{
		T returnValue = default;

		if (!IsConnected())
			return returnValue;

		try
		{
			returnValue = XDRPCMarshaler.ExecuteRPC<T>(m_XboxConsole, XDRPCMode.System, moduleName, ordinal, args);
		}
		catch (Exception)
		{
			throw new Exception("Error while calling the function!");
		}

		return returnValue;
	}

	public static uint GetCurrentTitleID()
	{
		uint titleID = 0;

		if (!IsConnected())
			return titleID;

		try
		{
			titleID = Call<uint>("xam.xex", 463, new object[] { });
		}
		catch (Exception)
		{
			throw new Exception("Couldn't get current title ID");
		}

		return titleID;
	}

	public static void XNotify(string text)
	{
		if (!IsConnected())
			return;

		try
		{
			Call<uint>("xam.xex", 656, Convert.ToUInt32(14), 0, 2, StringToWideCharArray(text), 0);
		}
		catch (Exception)
		{
			throw new Exception("Couldn't call XNotify!");
		}
	}

	public static int GetHostIndex(uint sessionIsHostAddr, uint sessionDataPtr)
	{
		int hostIndex = -1;

		if (!IsConnected())
			return hostIndex;

		try
		{
			for (int i = 0; i < 18; i++)
				if (Call<bool>(sessionIsHostAddr, sessionDataPtr, i))
					hostIndex = i;
		}
		catch (Exception)
		{
			throw new Exception("Couldn't find the host!");
		}

		return hostIndex;
	}

	public static string GetXenonUserGamertag(uint xenonUserDataPtr)
	{
		string gamertag;

		try
		{
			gamertag = ReadString(xenonUserDataPtr + 0x04);
		}
		catch (Exception)
		{
			throw new Exception("Couldn't get current user gamertag!");
		}

		return gamertag;
	}

	public static bool IsOnGame(TitleID titleID, MPStringAddr mpStringAddr)
	{
		bool isOnGame = false;

		if (!IsConnected())
			return isOnGame;

		isOnGame = GetCurrentTitleID() == (uint)titleID && ReadString((uint)mpStringAddr, 11) == "multiplayer";

		return isOnGame;
	}

	public static bool IsConnected()
	{
		return m_ActiveConnection;
	}

	public static uint ReadUInt32(uint address)
	{
		uint result = 0;

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[4];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			result = BitConverter.ToUInt32(memoryBuffer, 0);
		}
		catch (Exception)
		{
			throw new Exception("Could not read a uint32 at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteUInt32(uint address, uint input)
	{
		if (!IsConnected())
			return;

		try
		{
			byte[] memoryBuffer = new byte[4];
			BitConverter.GetBytes(input).CopyTo(memoryBuffer, 0);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write a uint32 at 0x" + address.ToString("X"));
		}
	}

	public static float ReadFloat(uint address)
	{
		float result = 0.0f;

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[4];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			result = BitConverter.ToSingle(memoryBuffer, 0);
		}
		catch (Exception)
		{
			throw new Exception("Could not read a float at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteFloat(uint address, float input)
	{
		if (!IsConnected())
			return;

		try
		{
			byte[] memoryBuffer = new byte[4];
			BitConverter.GetBytes(input).CopyTo(memoryBuffer, 0);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write a float at 0x" + address.ToString("X"));
		}
	}

	public static int ReadInt(uint address)
	{
		int result = 0;

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[4];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			result = BitConverter.ToInt32(memoryBuffer, 0);
		}
		catch (Exception)
		{
			throw new Exception("Could not read an int at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteInt(uint address, int input)
	{
		if (!IsConnected())
			return;

		try
		{
			byte[] memoryBuffer = new byte[4];
			BitConverter.GetBytes(input).CopyTo(memoryBuffer, 0);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write an int at 0x" + address.ToString("X"));
		}
	}

	public static float[] ReadVec3(uint address)
	{
		float[] result = { 0.0f, 0.0f, 0.0f };

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[12];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			Buffer.BlockCopy(memoryBuffer, 0, result, 0, memoryBuffer.Length);
			Array.Reverse(result);
		}
		catch (Exception)
		{
			throw new Exception("Could not read a vec3 at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteVec3(uint address, float[] input)
	{
		if (!IsConnected())
			return;

		if (input.Length != 3)
		{
			throw new Exception("The input needs to be an array of exactly 3 floats");
		}

		try
		{
			byte[] memoryBuffer = new byte[12];
			Array.Reverse(input);
			Buffer.BlockCopy(input, 0, memoryBuffer, 0, memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write a vec3 at 0x" + address.ToString("X"));
		}
	}

	public static short ReadShort(uint address)
	{
		short result = 0; ;

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[2];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			result = BitConverter.ToInt16(memoryBuffer, 0);
		}
		catch (Exception)
		{
			throw new Exception("Could not read a short at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteShort(uint address, short input)
	{
		if (!IsConnected())
			return;

		try
		{
			byte[] memoryBuffer = new byte[2];
			BitConverter.GetBytes(input).CopyTo(memoryBuffer, 0);
			Array.Reverse(memoryBuffer, 0, memoryBuffer.Length);
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write a float at 0x" + address.ToString("X"));
		}
	}

	public static string ReadString(uint address, int length = 32)
	{
		string result = null;

		if (!IsConnected())
			return result;

		try
		{
			byte[] memoryBuffer = new byte[length];
			m_XboxConsole.DebugTarget.GetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesRead);
			m_XboxConsole.DebugTarget.InvalidateMemoryCache(true, address, (uint)memoryBuffer.Length);
			result = new string(Encoding.ASCII.GetChars(memoryBuffer));
			char[] separator = new char[1];
			result = result.Split(separator)[0];
		}
		catch (Exception)
		{
			throw new Exception("Could not read a string at 0x" + address.ToString("X"));
		}

		return result;
	}

	public static void WriteString(uint address, string input)
	{
		// HAS NOT BEEN TESTED YET!!!
		if (!IsConnected())
			return;

		try
		{
			byte[] memoryBuffer = new byte[input.Length + 1];
			Encoding.ASCII.GetBytes(input).CopyTo(memoryBuffer, 0);
			memoryBuffer[memoryBuffer.Length] = 0;
			m_XboxConsole.DebugTarget.SetMemory(address, (uint)memoryBuffer.Length, memoryBuffer, out uint bytesWritten);
		}
		catch (Exception)
		{
			throw new Exception("Could not write a string at 0x" + address.ToString("X"));
		}
	}

	private static byte[] StringToWideCharArray(string text)
	{
		byte[] buffer = new byte[(text.Length * 2) + 2];
		int index = 1;
		buffer[0] = 0;
		foreach (char ch in text)
		{
			buffer[index] = Convert.ToByte(ch);
			index += 2;
		}
		return buffer;
	}
}