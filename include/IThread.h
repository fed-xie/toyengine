#pragma once
#include "EngineConfig.h"

#if defined(_MSC_VER) && _MSC_VER >= 1900
#include <mutex>
#include <thread>
#endif

#include <atomic>
#include <condition_variable>
#include <chrono>

namespace toy
{
	using ThreadFunc = unsigned long(*)(void*);

	//DO NOT use this, it's just a toy
	class ToySpinLock
	{
	public:
		ToySpinLock() : m_lock{ATOMIC_FLAG_INIT} {}
		void lock() { while (m_lock.test_and_set(std::memory_order_acquire)); }
		bool try_lock() { return m_lock.test_and_set(std::memory_order_acquire); }
		void unlock() { m_lock.clear(std::memory_order_release); }
	private:
		std::atomic_flag m_lock;
	};
#ifdef _WIN32

#if _MSC_VER >= 1900
	using IMutex = std::mutex;
	using IThread = std::thread;
#else
	class IMutex
	{
	public:
		IMutex(){ InitializeCriticalSection(&m_mutex); }
		~IMutex() { DeleteCriticalSection(&m_mutex); }
		void lock() { EnterCriticalSection(&m_mutex); }
		bool try_lock() { return TryEnterCriticalSection(&m_mutex); }
		void unlock() { LeaveCriticalSection(&m_mutex); }

	private:
		CRITICAL_SECTION m_mutex;
	};
	
	class IThread
	{
	public:
		IThread() { m_handle = nullptr; m_id = 0; }
		IThread(const IThread&) = delete;
		void createThread(ThreadFunc func, void* param)
		{
			m_handle = CreateThread(NULL, 0, func, param, 0, &m_id);
		}
		unsigned long get_id() const { return m_id; }
		void join()
		{
			WaitForMultipleObjects(1, &m_handle, true, INFINITE);
		}
		//void closeThread() { CloseHandle(m_handle); }

	private:
		HANDLE m_handle;
		unsigned long m_id;
	};
#endif //_MSC_VER >= 1900
	
#else //_WIN32
#pragma message("Unsupported os, using std namespace.")
#include <mutex>
#include <thread>
	using IMutex = std::mutex;
	using IThread = std::thread;
#endif //_WIN32
}