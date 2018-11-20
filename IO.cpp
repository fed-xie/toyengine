#include "include/IO.h"

#include <clocale>
#include <cstring>
#include <cstdarg> //for va_list

#ifdef _MSC_VER
#pragma warning(disable:4267) //From size_t to 32-bit variables
#endif

namespace toy
{
	namespace str
	{
		size_t utf8StrLen(const char * utf8str)
		{
			size_t i = 0, count = 0;
			unsigned char* str = (unsigned char*)utf8str;
			while (str[i])
			{
				if ((str[i] & 0xC0) != 0x80)
					++count;
				++i;
			}
			return count;
		}

#ifdef _WIN32
		size_t utf8ToUtf16le(void* utf16leDest, size_t destSizeInBytes, const char* utf8Str)
		{
			LPWSTR wstr = reinterpret_cast<LPWSTR>(utf16leDest);
			int numUtf16leCvted = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wstr, destSizeInBytes/sizeof(WCHAR));
			//return 0 == numUtf16leCvted ? GetLastError() : numUtf16leCvted;
			return 0 == numUtf16leCvted ? (size_t)-1 : numUtf16leCvted;
		}

		size_t ansiToUtf16le(void* utf16leDest, size_t destSizeInBytes, const char* ansiStr)
		{
			LPWSTR wstr = reinterpret_cast<LPWSTR>(utf16leDest);
			int numUtf16leCvted = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wstr, destSizeInBytes/sizeof(WCHAR));
			//return 0 == numUtf16leCvted ? GetLastError() : numUtf16leCvted;
			return 0 == numUtf16leCvted ? (size_t)-1 : numUtf16leCvted;
		}

		size_t utf16leToUtf8(char * utf8Dest, size_t destSizeInBytes, const void * utf16Str)
		{
			LPCWCH wsrc = reinterpret_cast<LPCWCH>(utf16Str);
			int numCharCvted = WideCharToMultiByte(CP_UTF8, 0, wsrc, -1, utf8Dest, destSizeInBytes, NULL, NULL);
			//return 0 == numCharCvted ? GetLastError() : numCharCvted;
			return 0 == numCharCvted ? (size_t)-1 : numCharCvted;
		}

		size_t utf16leToANSI(void* ansiDest, size_t destSizeInBytes, const void* utf16Str)
		{
			LPCWCH wsrc = reinterpret_cast<LPCWCH>(utf16Str);
			LPSTR dest = reinterpret_cast<LPSTR>(ansiDest);
			int numCharCvted = WideCharToMultiByte(CP_ACP, 0, wsrc, -1, dest, destSizeInBytes, NULL, NULL);
			//return 0 == numCharCvted ? GetLastError() : numCharCvted;
			return 0 == numCharCvted ? (size_t)-1 : numCharCvted;
		}


		void strnCpy(char* dest, size_t destSizeInBytes, const char* src, size_t maxCharCount)
		{
#if !defined(_CRT_SECURE_NO_WARNINGS)
			strncpy_s(dest, destSizeInBytes, src, maxCharCount);
#else
			strcpy(dest, src);
#endif
		}
		int striCmp(const char* s1, const char* s2)
		{
#if !defined(_CRT_NONSTDC_NO_DEPRECATE)
			return _stricmp(s1, s2);
#else
			return stricmp(s1, s2);
#endif
		}
		size_t ansiStrLen(const char* ansiStr)
		{
			return _mbstrlen(ansiStr);
		}
		size_t utf16leStrLen(const void* utf16leStr)
		{
			return wcslen(reinterpret_cast<const wchar_t*>(utf16leStr));
		}
#else //_WIN32
#error("OS not supported yet(toy::io::str)")
#endif //_WIN32

	}//namespace str

	namespace io
	{
		void Path::setPath(const char* path)
		{
			m_strLen = strlen(path);
			size_t i = 0;
			while('\0' != path[i] && i < m_strLen)
			{
				m_path[i] = path[i];
				++i;
			}
			m_path[i] = '\0';
		}
		int Path::getDirStrLen(const char* path, size_t strLen)
		{
			assert(path);
			if(0 == strLen)
				strLen = std::strlen(path);

			int i;
			for (i = static_cast<int>(strLen); i >= 0; --i)
			{
				if (path[i] == '/' || path[i] == '\\')
					return ++i;
			}
			return 0;
		}
		const char* Path::getFileName(const char* path)
		{
			int dirLen = getDirStrLen(path);
			return 0 == dirLen ? nullptr : &path[dirLen];
		}
		const char* Path::getSuffix(const char* filename)
		{
			if (nullptr == filename || *filename == '\0')
				return nullptr;

			size_t i = strlen(filename);
			for (; i != 0; --i)
			{
				if (filename[i] == '.')
					break;
			}
			return 0 == i ? nullptr : &filename[i + 1];
		}
		void Path::setFileName(const char* fileName)
		{
			assert(fileName);
			if(0 == m_dirLen)
				m_dirLen = getDirStrLen();
			int availspace = m_dirLen ? sizeof(m_path) - m_strLen - 1 : sizeof(m_path);

			int i = 0;
			while('\0' != fileName[i] && i < availspace)
			{
				m_path[m_dirLen + i] = fileName[i];
				++i;
			}
			m_path[m_dirLen + i] = '\0';
			m_strLen = m_dirLen + i;
		}


		void WPath::setPath(const char* path)
		{
			m_strLen = str::utf8ToUtf16le(m_path, sizeof(m_path), path);
			assert((size_t)-1 != m_strLen);
			if (m_strLen > 0)
				--m_strLen;
			if (m_strLen > 0 &&
				('\\' == m_path[m_strLen - 1] || '/' == m_path[m_strLen - 1]))
				m_dirLen = m_strLen;
			else
				m_dirLen = 0;
		}
		void WPath::setPath(const wchar_t* path)
		{
			size_t i = 0;
			constexpr size_t wlen = sizeof(m_path) / sizeof(wchar_t) - 1;
			while(L'\0' != path[i] && i < wlen)
			{
				m_path[i] = path[i];
				++i;
			}
			m_path[i] = L'\0';
			m_strLen = i;
			if (i > 0 && (L'\\' == m_path[i - 1] || L'/' == m_path[i - 1]))
				m_dirLen = i;
			else
				m_dirLen = 0;
		}
		int WPath::getDirStrLen(const wchar_t* path, size_t strLen)
		{
			assert(path);
			if (0 == strLen)
				strLen = wcslen(path);

			int i;
			for (i = static_cast<int>(strLen); i >= 0; --i)
			{
				if (path[i] == L'/' || path[i] == L'\\')
					return ++i;
			}
			return 0;
		}
		const wchar_t* WPath::getFileName(const wchar_t* path)
		{
			int dirLen = getDirStrLen(path);
			return 0 == dirLen ? nullptr : &path[dirLen];
		}
		const wchar_t* WPath::getSuffix(const wchar_t* filename)
		{
			if (nullptr == filename || *filename == L'\0')
				return nullptr;

			size_t i = wcslen(filename);
			for (; i != 0; --i)
			{
				if (filename[i] == L'.')
					break;
			}
			return 0 == i ? nullptr : &filename[i + 1];
		}
		const wchar_t* WPath::setFileName(const wchar_t* fileName)
		{
			if (0 == m_dirLen)
				getDirStrLen();
			int i = 0;
			int availspace = ARRAYLENGTH(m_path) - m_dirLen - 1;
			while (L'\0' != fileName[i] && i < availspace)
			{
				m_path[m_dirLen + i] = fileName[i];
				++i;
			}
			m_strLen = m_dirLen + i;
			m_path[m_strLen] = L'\0';
			return m_path;
		}
		const wchar_t* WPath::setFileName(const char* fileName)
		{
			if (0 == m_dirLen)
				getDirStrLen();
			int i = 0;
			size_t availSize = (ARRAYLENGTH(m_path) - m_dirLen - 1) * sizeof(wchar_t);
			size_t nameLen = str::utf8ToUtf16le(&m_path[m_dirLen], availSize, fileName);
			m_strLen = m_dirLen + nameLen;
			return m_path;
		}
		void WPath::getCWD()
		{
			uint cnt = GetCurrentDirectoryW(sizeof(m_path) / sizeof(WCHAR), m_path);
			assert(cnt < ARRAYLENGTH(m_path));
			m_path[cnt] = L'\\';
			m_path[cnt + 1] = L'\0';
			m_dirLen = m_strLen = cnt + 1;
		}

		static const wchar_t* fmstr[] = //file mode string, ordered by IOFile::OFlag
		{
			L"rb",
			L"rb+",
			L"wb",
			L"wb+"
		};
		ErrNo IOFile::open(WPath& path, OFlag mode, size_t* outFileSize)
		{
			close();

			errno_t err = _wfopen_s(&m_file, path.c_wcs(), fmstr[mode]);
			if (err)
			{
				Logger::log(Logger::LOG_ERROR, L"Fail to open file %s\n", path.c_wcs());
				return err;
			}

			if(outFileSize)
			{
				_fseeki64(m_file, 0, SEEK_END);
				*outFileSize = _ftelli64(m_file);
				_fseeki64(m_file, 0, SEEK_SET);
			}
			#ifndef _MSC_VER
			#error("IOFile open")
			#endif
			return 0;
		}

		ErrNo IOFile::read(void* buf, size_t bufSize, size_t sizeToRead)
		{
			assert(buf && "buffer for file reading is null");
			assert(bufSize >= sizeToRead && "buffer is too small");
			if (!m_file)
			{
				Logger::log(Logger::LOG_ERROR, L"File not opened!\n");
				return -1;
			}
			if (1 != fread_s(buf, bufSize, sizeToRead, 1, m_file))
			{
				Logger::log(Logger::LOG_ERROR, L"Fail to read file.\n");
				return -2;
			}
			return 0;
			#ifndef _MSC_VER
			#error("IOFile read")
			#endif
		}
		void IOFile::close()
		{
			if(m_file)
			{
				fclose(m_file);
				m_file = nullptr;
			}
		}

		char* readText(WPath& wpath)
		{
			IOFile file;
			size_t fsize;
			if (0 == file.open(wpath, IOFile::OFLAG_READONLY, &fsize))
			{
				char* text = new char[fsize + 1];
				if (0 == file.read(text, fsize, fsize))
				{
					file.close();
					text[fsize] = '\0';
					return text;
				}

				//Failed to read file
				delete[] text;
				file.close();
			}
			return nullptr;
		}
		void freeText(char* &text)
		{
			delete[](text);
			text = nullptr;
		}
		u8* readFile(WPath& wpath, size_t* outDataSize)
		{
			IOFile file;
			size_t fsize;
			if (0 == file.open(wpath, IOFile::OFLAG_READONLY, &fsize))
			{
				u8* text = new u8[fsize];
				if (0 == file.read(text, fsize, fsize))
				{
					file.close();
					if (outDataSize)
						*outDataSize = fsize;
					return text;
				}
				//Failed to read file
				delete[] text;
				file.close();
			}
			return nullptr;
		}
		void freeFile(u8* &fileData)
		{
			delete[] fileData;
			fileData = nullptr;
		}

#ifdef _DEBUG
		Logger::LogLevel Logger::envLogLevel = Logger::LogLevel::LOG_DEBUG;
#else
		Logger::LogLevel Logger::envLogLevel = Logger::LogLevel::LOG_WARNING;
#endif
		static const char* logLevelStr[] =
		{
			"Debug: ",
			"Info: ",
			"Warning: ",
			"Error: ",
			"Fatal: "
		};
		static const wchar_t* logLevelWStr[] =
		{
			L"Debug: ",
			L"Info: ",
			L"Warning: ",
			L"Error: ",
			L"Fatal: "
		};

		Logger::LogLevel Logger::setLogLevel(LogLevel level)
		{
			LogLevel r = envLogLevel;
			envLogLevel = level;
			return r;
		}

		void Logger::logInit()
		{
			setlocale(LC_CTYPE, "");
		}

		static void vlog(const char* format, va_list ap)
		{
			char utf8[2048];
#ifndef _CRT_SECURE_NO_WARNINGS
			vsprintf_s(utf8, format, ap);
#else
			vsprintf(utf8, format, ap);
#endif
			utf8[2047] = '\0';

			//TODO: print log to consult and file
			wchar_t utf16[1024];
			str::utf8ToUtf16le(utf16, sizeof(utf16), utf8);
			utf16[1023] = L'\0';
			wprintf(utf16);
#ifdef _MCS_VER
			OutputDebugString(utf8); //OutputDebugString only accept ASCII string
#endif
		}
		static void vlog(const wchar_t* format, va_list ap)
		{
			wchar_t utf16[1024];
			vswprintf(utf16, 1023, format, ap);
			utf16[1023] = L'\0';

			//TODO: print log to consult and file
			wprintf(utf16);
#if defined(TOY_LOG_TO_FILE)
			char utf8[2048];
			str::utf16leToUTF8(utf8, sizeof(ansi), utf16);
#endif
		}

		void Logger::log(LogLevel level, const wchar_t* fmtText, ...)
		{
			if (level < envLogLevel || nullptr == fmtText)
				return;

			va_list ap;
			va_start(ap, fmtText);
			vlog(fmtText, ap);
			va_end(ap);
		}
		void Logger::log(LogLevel level, const char* fmtText, ...)
		{
			if (nullptr == fmtText)
				return;

			va_list ap;
			va_start(ap, fmtText);
			vlog(fmtText, ap);
			va_end(ap);
		}
		void Logger::log(const char* fmtText, ...)
		{
			va_list ap;
			va_start(ap, fmtText);
			vlog(fmtText, ap);
			va_end(ap);
		}
		void Logger::log(const wchar_t* fmtText, ...)
		{
			va_list ap;
			va_start(ap, fmtText);
			vlog(fmtText, ap);
			va_end(ap);
		}
#ifdef _WIN32
		uint getCWD(uint bufSizeInByte, wchar_t* buffer)
		{
			//GetModuleFileName();
			return GetCurrentDirectoryW(bufSizeInByte/sizeof(TCHAR), buffer);
		}
		ErrNo setCWD(const wchar_t* path)
		{
			return 0 != SetCurrentDirectoryW(path) ? 0 : -1;
		}
		ErrNo setCWD(const char* path)
		{
			wchar_t utf16[1024];
			str::utf8ToUtf16le(utf16, sizeof(utf16), path);
			utf16[1023] = L'\0';
			return 0 != SetCurrentDirectoryW(utf16) ? 0 : -1;
		}
#else
#error("OS not supported (toy::io::cwd)")
#endif
	}
}