// CTXorDLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <detours.h>
#include <iostream>
#include <fstream>
#include "xorgen.h"
#pragma comment(lib, "detours.lib")

bool SkipNextHook = false;
wchar_t WorkingDirectory[MAX_PATH];
std::wstring ParseXORContents(std::wstring &TempLine)
{
	/*Check if the line has an XOR in it, accounts for multiple
	XORs on the same line*/
	std::size_t StartXOR = 0;
	std::size_t EndQuote = 0;
	std::size_t StartQuote = 0;
	std::size_t EndXOR = 0;
	int StartSearch = 0;
	bool NeedsEnd = false;

	int EraseFixup = 0;
	int InsertFixup = 0;
	std::wstring TotalLine = TempLine;

	if (TempLine.length() <= 0)
		return TempLine;

	//BS Loop SHOULD exit before this
	while (EndQuote<TempLine.length())
	{
		//Do We need beginning of XOR or End
		if (!NeedsEnd)
		{
			//Find Start of XOR Macro
			StartXOR = TempLine.find(L"XOR(", StartSearch);
			if (StartXOR == std::string::npos)
				return TotalLine;

			NeedsEnd = true;

			//Move to End of XOR( character Sequence
			StartXOR += 4;

			//Find Start quote after macro
			StartQuote = TempLine.find(L"\"", StartXOR);
			if (StartQuote == std::string::npos)
				return TempLine;

			StartQuote += 1; //Move to End of Quote
		} else
		{
			EndQuote = TempLine.find(L"\"", StartQuote);
			if (EndQuote == std::string::npos)
				return TempLine;

			bool FoundUnEscapedEnd = false;
			int EscapedFixup = 0;
			while (!FoundUnEscapedEnd)
			{
				if (TempLine.at(EndQuote - 1) != '\\' || (TempLine.at(EndQuote - 2) == '\\' && TempLine.at(EndQuote - 1) == '\\'))
					FoundUnEscapedEnd = true;
				else
				{
					EscapedFixup += 1;
					EndQuote = TempLine.find(L"\"", StartQuote + EscapedFixup);
					if (EndQuote == std::string::npos)
						return TempLine;
				}
			}

			NeedsEnd = false;

			//For Next Iteration ignore previous
			StartSearch = EndQuote;

			std::wstring XORContents = TempLine.substr(StartQuote, EndQuote - StartQuote);

			BOOL Success;
			std::wstring EncryptedSubString = StringToWString(blub(WStringToString(XORContents), Success));
			if (!Success)
				return TempLine;

			EndXOR = TempLine.find(L")", EndQuote);
			if (EndXOR == std::string::npos)
				return TempLine;

			//Move to end of ) and go back to beginning of XOR Macro
			EndXOR += 1;
			StartXOR -= 4;

			//Erase Raw text and replace with encrypted
			TotalLine.erase(StartXOR - EraseFixup + InsertFixup, EndXOR - StartXOR);
			TotalLine.insert(StartXOR - EraseFixup + InsertFixup, EncryptedSubString);

			//Next Time around we need to account for the changed length of the string
			InsertFixup += EncryptedSubString.length();
			EraseFixup += (EndXOR - StartXOR);
		}
	}
	return TempLine;
}

std::wstring ParseFile(std::wstring& str)
{
	std::string LowerWorkingDir = lower_string(WStringToString((std::wstring(WorkingDirectory))));

	bool InDirectory = str.find(StringToWString(LowerWorkingDir)) != std::string::npos;
	if (str.find(L"\\") == std::string::npos && str.find(L":") == std::string::npos || InDirectory && str.find(L"resource.h")==std::string::npos)
	{
		if (str.find(L".cpp") != std::string::npos || str.find(L".h") != std::string::npos)
		{
			std::wstring InputFileName;
			std::wstring OutputFileName;
			if (InDirectory)
			{
				/*If we are a file such as D:\\Path\\Project\\Main.cpp
				  We need to strip Main.cpp off the end, and append it to 
				  the temp directory*/
				InputFileName = str;
				size_t IndexOfSlash = str.find_last_of(L"\\");
				if (IndexOfSlash == std::string::npos)
					return str;

				std::wstring FileDirRemoved = str.substr(IndexOfSlash + 1, str.length() - IndexOfSlash + 1);
				str.erase(IndexOfSlash + 1, str.length() - IndexOfSlash + 1);

				std::wstringstream ssTempFile;
				ssTempFile << WorkingDirectory << L"\\Temp\\" << FileDirRemoved;
				OutputFileName = ssTempFile.str();
			} else{
				/*If we are a file such as main.cpp we need to append it to the
				temp directory, and build up the input directory based on our 
				working directory (the project folder)*/
				std::wstringstream ssInFile;
				ssInFile << WorkingDirectory << L"\\" << str;
				InputFileName = ssInFile.str();

				std::wstringstream ssTempFile;
				ssTempFile << WorkingDirectory << L"\\Temp\\" << str;
				OutputFileName = ssTempFile.str();
			}
			std::wifstream InputFile;
			std::wofstream OutputFile;
			SkipNextHook = true;
			InputFile.open(InputFileName.c_str());
			OutputFile.open(OutputFileName.c_str());
			if (InputFile.is_open())
			{
				while (!InputFile.eof())
				{
					//Read Input File, Replace with Xor Contents, Write to output
					wchar_t Buffer[2048];
					InputFile.getline(Buffer, 2048);
					OutputFile << ParseXORContents(std::wstring(Buffer)) << L"\n";
				}
			}
			InputFile.close();
			OutputFile.close();
			SkipNextHook = false;
			return OutputFileName;
		}
		return str;
	}
	return str;
}

typedef errno_t(__cdecl* twsopen_s)(int* pfh, const wchar_t *filename, int oflag, int shflag, int pmode);
twsopen_s owsopen_s;
errno_t hkwsopen_s(int* pfh, wchar_t *filename, int oflag, int shflag, int pmode)
{
	//This Prevents and infinite loop, it appears std::wifstream, uses this open file function
	//Skipping one itteration fixes the recursion
	if (SkipNextHook)
		return owsopen_s(pfh, filename, oflag, shflag, pmode);
	
	std::wstring NewFile=ParseFile(std::wstring(filename));
	return owsopen_s(pfh, const_cast<wchar_t*>(NewFile.c_str()), oflag, shflag, pmode);
}

void DoTasks()
{
	HMODULE hModule;
	FARPROC targetFunction;

	//We can't determine exactly which runtime they use so try em all
	hModule = GetModuleHandle(L"MSVCR100.dll");
	if (hModule == NULL)
		hModule = GetModuleHandle(L"MSVCR110.dll");
	if (hModule == NULL)
		hModule = GetModuleHandle(L"MSVCR120.dll");

	if (hModule ==NULL)
		MessageBox(NULL, L"Failed to Find MSVCR For CT String Crypt", L"Compile Time String XOR Failed", MB_OK);

	//Get Project Directory
	_wgetcwd(WorkingDirectory, MAX_PATH);
	std::wstringstream ssTempDirPath;
	ssTempDirPath << WorkingDirectory << L"\\Temp";
	if (!DirectoryExists(ssTempDirPath.str().c_str()))
	{
		CreateDirectory(ssTempDirPath.str().c_str(), NULL);
	}

	targetFunction = GetProcAddress(hModule, "_wsopen_s");
	owsopen_s = (twsopen_s)DetourFunction((PBYTE)targetFunction, (PBYTE)hkwsopen_s);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DoTasks();
	
	return TRUE;
}


