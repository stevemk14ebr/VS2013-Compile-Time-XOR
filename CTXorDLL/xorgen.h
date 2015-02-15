
// XorGen from d4rc converted to C++
#include <Windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <random>
#include <stdint.h>
#include <algorithm>
using namespace std;

std::string upper_string(const std::string& str)
{
	std::string upper;
	transform(str.begin(), str.end(), std::back_inserter(upper), toupper);
	return upper;
}

std::string WStringToString(std::wstring& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.to_bytes(str);
}

std::wstring StringToWString(std::string& str)
{
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.from_bytes(str);
}

__forceinline std::mt19937 InitialzeRandTrueRand()
{
	uint_least32_t seed[std::mt19937::state_size];
	std::random_device randDevice;
	std::generate_n(seed, std::mt19937::state_size, std::ref(randDevice));
	std::seed_seq seedseq(std::begin(seed), std::end(seed));
	std::mt19937 Gen(seedseq);
	return Gen;
}

BYTE randByte()
{
	std::mt19937 RandGen = InitialzeRandTrueRand();
	std::uniform_int_distribution<uint_least32_t> RandDist(0, 256);
	return static_cast<uint8_t>(RandDist(RandGen));
}

string Dec2Hex(BYTE value)
{
	stringstream sstream;

	sstream << setw(2) << std::setfill('0') << std::hex << (int)value;

	string wut = sstream.str();


	return sstream.str();
}

BYTE randByte2()
{
	return rand() % 256;
}
 
string blub(string s1, BOOL& dwSuccess)
{
	string result;
	result = "";

	dwSuccess = TRUE;

	BYTE xvaluestart = randByte();
	string xrefkill = "0x" + Dec2Hex(randByte()) + Dec2Hex(randByte()) + Dec2Hex(randByte()) + Dec2Hex(randByte());
	int finallen = s1.length() + 1;
	string hexsequence = "\"";
	BYTE xvalue = xvaluestart;
	for (int i = 0; i < s1.length(); i++)
	{
		BYTE ch = s1.substr(i, 1).c_str()[0];
		BYTE chval;
		if (ch == '\\')
		{
			i++;
			ch = s1.substr(i, 1).c_str()[0];
			if (ch == '0')
			{
				chval = 0;
			} else if (ch == '\\')
			{
				chval = '\\';
			} else if (ch == 'a')
			{
				chval = 7;
			} else if (ch == 'b')
			{
				chval = 8;
			} else if (ch == 't')
			{
				chval = 9;
			} else if (ch == 'n')
			{
				chval = 10;
			} else if (ch == 'v')
			{
				chval = 11;
			} else if (ch == 'f')
			{
				chval = 12;
			} else if (ch == 'r')
			{
				chval = 13;
			} else if (ch == '\"')
			{
				chval = '\"';
			} else
			{
				dwSuccess = FALSE;
				result = string("XorGen: invalid control sequence: \\") + (char)ch;
				return result;
			}
			--finallen;
		} else if (ch == '|')
		{
			chval = 0;
		} else
		{
			chval = ch;
			if (!(chval >= 32 && chval <= 126))
			{
				dwSuccess = FALSE;
				result = string("XorGen: invalid control sequence: \\") + (char)ch;

				return result;
			}
		}
		chval ^= xvalue;
		xvalue += 1;
		xvalue %= 256;
		hexsequence += "\\x" + Dec2Hex(chval);
	}
	hexsequence += '"';

	stringstream sstream;

	sstream << "XorStr<0x" << Dec2Hex(xvaluestart) << "," << finallen << "," << xrefkill << ">(";
	string s2 = sstream.str();
	s2 += hexsequence + "+" + xrefkill + ").s";
	result = s2;

	return result;
}

std::string lower_string(const std::string& str)
{
	std::string upper;
	transform(str.begin(), str.end(), std::back_inserter(upper), tolower);
	return upper;
}

BOOL DirectoryExists(const wchar_t* dirName) {
	DWORD attribs = ::GetFileAttributesW(dirName);
	if (attribs == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	return (attribs & FILE_ATTRIBUTE_DIRECTORY);
}