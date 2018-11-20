#pragma once
#include "Types.h"

#include <cstdio>

namespace toy
{
	namespace str
	{
		enum Enctype { ET_UTF8, ET_ANSI, ET_UTF16LE, ET_UTF16BE };		
		size_t utf8StrLen(const char* utf8Str);
	
		/* return number of charaters be converted , terminating null including, if error, return (size_t)-1 */
		size_t utf8ToUtf16le(void* utf16leDest, size_t destSizeInBytes, const char* utf8Str);
		size_t ansiToUtf16le(void* utf16leDest, size_t destSizeInBytes, const char* ansiStr);
		size_t utf16leToUtf8(char* utf8Dest, size_t destSizeInBytes, const void* utf16Str);
		size_t utf16leToANSI(void* ansiDest, size_t destSizeInBytes, const void* utf16Str);
		
		void strnCpy(char* dest, size_t destSizeInBytes, const char* src, size_t maxCharCount);

		int striCmp(const char* s1, const char* s2);

		constexpr u64 BKDRHash(const char* str)
		{
			if (nullptr == str || '\0' == *str)
				return 0;

			register u64 hash = 0;
			while ('\0' != *str)
			{
				hash = hash * 31 + *str++;
				//hash = (hash << 5) - hash + ch;
			}
			return hash;
		}
		
		template<size_t structSize = 1024>
		class TShortString
		{
		public:
			TShortString(const TShortString &v) = delete;
			TShortString()
			{
				m_str.mbs[0] = '\0';
				m_properties.strSizeInBytes = 0;
				m_properties.Enctype = str::ET_UTF8;
			}
			TShortString(const void* str, str::Enctype et = str::ET_UTF8)
			{
				m_properties.Enctype = et;
				if (nullptr == str)
				{
					m_str.mbs[0] = '\0';
					m_properties.strSizeInBytes = 0;
					return;
				}

				if (str::ET_UTF8 == et || str::ET_ANSI == et)
				{
					const char* mbs = reinterpret_cast<const char*>(str);
					size_t slen = strlen(mbs) % sizeof(m_str);
					str::strnCpy(m_str.mbs, sizeof(m_str), mbs, slen);
					m_str.mbs[slen + 1] = '\0';
					m_properties.strSizeInBytes = slen + 1;
				}
				else if (str::ET_UTF16LE == et || str::ET_UTF16BE == et)
				{
					const wchar_t* utf16s = reinterpret_cast<const wchar_t*>(str);
					copyWcs(utf16s, et);
				}
				else
				{
					m_str.mbs[0] = '\0';
					m_properties.strSizeInBytes = 0;
					m_properties.Enctype = str::ET_UTF8;
				}
			}

			bool toUTF8()
			{
				if (str::ET_UTF8 == m_properties.Enctype)
					return true;
				if (str::ET_ANSI == m_properties.Enctype)
				{
					wchar_t wcs[sizeof(m_str) / sizeof(wchar_t)];

					str::ansiToUtf16le(wcs, sizeof(wcs), m_str.mbs);
					str::utf16leToUtf8(m_str.mbs, sizeof(m_str.mbs), wcs);
					m_properties.strSizeInBytes = strlen(m_str.mbs) + 1;
					m_properties.Enctype = str::ET_UTF8;
					return true;
				}
				if (str::ET_UTF16LE == m_properties.Enctype)
				{
					char utf8s[sizeof(m_str)];
					str::utf16leToUtf8(utf8s, sizeof(utf8s), m_str.wcs);
					size_t slen = strlen(utf8s) % sizeof(m_str);
					str::strnCpy(m_str.mbs, sizeof(m_str), utf8s, slen);
					m_str.mbs[slen + 1] = '\0';
					m_properties.strSizeInBytes = slen + 1;
					m_properties.Enctype = str::ET_UTF8;
					return true;
				}
				return false;
			}

			bool toANSI()
			{
				if (str::ET_ANSI == m_properties.Enctype)
					return true;
				if (str::ET_UTF8 == m_properties.Enctype)
				{
					wchar_t wcs[sizeof(m_str) / sizeof(wchar_t)];

					str::utf8ToUtf16le(wcs, sizeof(wcs), m_str.mbs);
					str::utf16leToANSI(m_str.mbs, sizeof(m_str.mbs), wcs);
					m_properties.strSizeInBytes = strlen(m_str.mbs) + 1;
					m_properties.Enctype = str::ET_ANSI;
					return true;
				}
				if (str::ET_UTF16LE == m_properties.Enctype)
				{
					char utf8s[sizeof(m_str)];
					str::utf16leToANSI(utf8s, sizeof(utf8s), m_str.wcs);
					size_t slen = strlen(utf8s) % sizeof(m_str);
					str::strnCpy(m_str.mbs, sizeof(m_str), utf8s, slen);
					m_str.mbs[slen + 1] = '\0';
					m_properties.strSizeInBytes = slen + 1;
					m_properties.Enctype = str::ET_ANSI;
					return true;
				}
				return false;
			}

			bool toUTF16LE()
			{
				if (str::ET_UTF16LE == m_properties.Enctype)
					return m_str.wcs;
				if (str::ET_UTF8 == m_properties.Enctype)
				{
					wchar_t utf16les[sizeof(m_str) / sizeof(wchar_t)];
					str::utf8ToUtf16le(utf16les, sizeof(utf16les), m_str.mbs);
					copyWcs(utf16les, str::ET_UTF16LE);
					return m_str.wcs;
				}
				if (str::ET_ANSI == m_properties.Enctype)
				{
					wchar_t utf16les[sizeof(m_str) / sizeof(wchar_t)];
					str::ansiToUtf16le(utf16les, sizeof(utf16les), m_str.mbs);
					copyWcs(utf16les, str::ET_UTF16LE);
					return m_str.wcs;
				}
				return false;
			}

			const char* c_str() { return m_str.mbs; }
			const wchar_t* c_wcs() { return	m_str.wcs; }

			str::Enctype getEnctype() { return m_properties.Enctype; }
			size_t size() { return m_properties.strSizeInBytes; }

		private:
			void copyWcs(const wchar_t* wcs, str::Enctype et)
			{
				size_t i = 0;
				constexpr size_t wlen = sizeof(m_str) / sizeof(wchar_t) - 1;
				while (wcs[i] != L'\0' && i < wlen)
				{
					m_str.wcs[i] = wcs[i];
					++i;
				}
				m_str.wcs[i] = L'\0';
				m_properties.strSizeInBytes = i * sizeof(wchar_t);
				m_properties.Enctype = et;
			}
			struct StringProperties
			{
				size_t strSizeInBytes; //terminating null including
				str::Enctype Enctype;
			}m_properties;
			union
			{
				char mbs[(structSize - sizeof(StringProperties))/sizeof(char)];
				wchar_t wcs[(structSize - sizeof(StringProperties)) / sizeof(wchar_t)];
			}m_str;
		};
	} //namespace str
	
	namespace io
	{
		class Path
		{
		public:
			void setPath(const char* path);
			Path(const char* path) { setPath(path); }
			Path(const Path& v) = delete;
			~Path() {}
			
			//return directory string length, same as the position [last '/' or '\\'] + 1,
			//if not found, return 0
			static int getDirStrLen(const char* path, size_t strLen = 0);
			
			//if not found, return nullptr
			static const char* getFileName(const char* path);
			
			//if not found, return nullptr
			static const char* getSuffix(const char* filename);
			
			//return directory string length, same as the position [last '/' or '\\'] + 1,
			//if not found, return 0
			int getDirStrLen() { return getDirStrLen(m_path, m_strLen); }
			const char* getSuffix() { return getSuffix(m_path); }
			const char* getFileName() { return getFileName(m_path); }
			const char* c_str() const { return m_path; }
			
			void setFileName(const char* fileName);
			
		private:
			uint m_strLen = 0;
			int m_dirLen = 0;
			char m_path[1024];
		};
		
		/* Assuming WPath has 3 types:
		   1. Filename, including suffix (eg. L"FileName.txt")
		   2. Directory, end with L'\\' or L'/' (eg. L"NewDirectory/")
		   3. Full path, including both directory and filename (eg. L"./asset/model.dae")
		*/
		class WPath
		{
		public:
			void setPath(const char* path);
			void setPath(const wchar_t* path);
			
			WPath(const char* path) { setPath(path); }
			WPath(const wchar_t* path) { setPath(path); }
			WPath(const WPath& v) = delete;
			~WPath() {}
			
			//return directory string length, same as the position [last L'/' or L'\\'] + 1,
			//if not found, return 0
			static int getDirStrLen(const wchar_t* path, size_t strLen = 0);
			
			//if not found, return nullptr
			static const wchar_t* getFileName(const wchar_t* path);
			
			//if not found, return nullptr
			static const wchar_t* getSuffix(const wchar_t* filename);
			
			//return directory string length, same as the position [last L'/' or L'\\'] + 1,
			//if not found, return 0
			int getDirStrLen() { return m_dirLen = getDirStrLen(m_path, m_strLen); }
			const wchar_t* getSuffix() { return getSuffix(m_path); }
			const wchar_t* getFileName() { return getFileName(m_path); }
			const wchar_t* c_wcs() const { return m_path; }
			uint getStrLen() { return m_strLen; }

			const wchar_t* setFileName(const wchar_t* fileName);
			const wchar_t* setFileName(const char* fileName);
			void getCWD();
			
		private:
			uint m_strLen = 0;
			uint m_dirLen = 0;
			wchar_t m_path[1024];
		};
		
		class IOFile
		{
		public:
			enum OFlag{ OFLAG_READONLY, OFLAG_READWRITE, OFLAG_WRITEONLY, OFLAG_WRITECOVER };
			ErrNo open(WPath& path, OFlag mode, size_t* outFileSize = nullptr);
			ErrNo read(void* buf, size_t bufSize, size_t sizeToRead);
			void close();
			
			std::FILE* getFILE() { return m_file; }
			~IOFile() { close(); }
		private:
			std::FILE* m_file = nullptr;
		};

		/* Assuming all text files are encrypted by UTF-8 */
		char* readText(WPath& wpath);
		void freeText(char* &text);
		/* Read binary file */
		u8* readFile(WPath& wpath, size_t* outDataSize = nullptr);
		void freeFile(u8* &fileData);
		
		class Logger
		{
		public:
			enum LogLevel
			{
				LOG_DEBUG,
				LOG_INFO,
				LOG_WARNING,
				LOG_ERROR,
				LOG_FATAL,
			};
			//Return previous LogLevel
			static LogLevel setLogLevel(LogLevel level);
			static void logInit();
			static void log(LogLevel level, const wchar_t* fmtText, ...);
			static void log(LogLevel level, const char* fmtText, ...);
			static void log(const char* fmtText, ...);
			static void log(const wchar_t* fmtText, ...);
		
		private:
			static LogLevel envLogLevel;
		};

		/* Return value specifies the number of characters that are written to the buffer,
		* not including the terminating null character. */
		uint getCWD(uint bufSizeInByte, wchar_t* buffer);
		ErrNo setCWD(const wchar_t* path);
		ErrNo setCWD(const char* path);
	}
}

#define LOG_EXT(level, fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(level, fmtText, ##__VA_ARGS__);}
#define LOG_EXT_DEBUG(fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(toy::io::Logger::LOG_DEBUG, fmtText, ##__VA_ARGS__);}
#define LOG_EXT_INFO(fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(toy::io::Logger::LOG_INFO, fmtText, ##__VA_ARGS__);}
#define LOG_EXT_WARNING(fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(toy::io::Logger::LOG_WARNING, fmtText, ##__VA_ARGS__);}
#define LOG_EXT_ERROR(fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(toy::io::Logger::LOG_ERROR, fmtText, ##__VA_ARGS__);}
#define LOG_EXT_FATAL(fmtText, ...) \
	{toy::io::Logger::log("(%d,%s):", __LINE__, __FILE__); \
	toy::io::Logger::log(toy::io::Logger::LOG_FATAL, fmtText, ##__VA_ARGS__);}
#define LOGDEBUG(fmt, ...) \
	toy::io::Logger::log(toy::io::Logger::LOG_DEBUG, fmt, ##__VA_ARGS__);
#define LOGINFO(fmt, ...) \
	toy::io::Logger::log(toy::io::Logger::LOG_INFO, fmt, ##__VA_ARGS__);
#define LOGWARNING(fmt, ...) \
	toy::io::Logger::log(toy::io::Logger::LOG_WARNING, fmt, ##__VA_ARGS__);
#define LOGERROR(fmt, ...) \
	toy::io::Logger::log(toy::io::Logger::LOG_ERROR, fmt, ##__VA_ARGS__);
#define LOGFATAL(fmt, ...) \
	toy::io::Logger::log(toy::io::Logger::LOG_FATAL, fmt, ##__VA_ARGS__);