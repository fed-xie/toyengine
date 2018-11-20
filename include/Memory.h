#pragma once
#include <cassert>
#include <new>

#include "Types.h"
#include "IThread.h"
#include "IO.h"

namespace toy
{
	template <typename UnitType>
	class tPoolAllocator
	{
	public:
		tPoolAllocator() = delete;
		tPoolAllocator(void* buffer, size_t unitNum, const char* name = nullptr) :
			m_poolSize(sizeof(UnitType)*unitNum)
		{
			size_t poolSize = m_poolSize + 16; //16 is alignment
			size_t rawAddr = reinterpret_cast<size_t>(buffer);
			u8 adjustment = 16 - (rawAddr & 15);
			m_poolBottom = reinterpret_cast<void*>(rawAddr + adjustment);
			u8* temp = reinterpret_cast<u8*>(m_poolBottom) - 1;
			*temp = adjustment;

			m_availFstUnit = reinterpret_cast<PoolUnit*>(m_poolBottom);
			for (size_t i = 0; i != unitNum - 2; ++i)
			{
				m_availFstUnit[i].m_next = &m_availFstUnit[i + 1];
			}
			m_availFstUnit[unitNum - 1].m_next = nullptr;

			str::strnCpy(m_name, sizeof(m_name),
				name ? name : "PoolAllocator", sizeof(m_name) - 1);
			m_name[sizeof(m_name) - 1] = '\0';
			io::log(io::LogLevel::LOG_INFO,
				"%s construct %li bytes buffer for pool\n", m_name, poolSize);
		}
		~tPoolAllocator()
		{
			//u8* temp = reinterpret_cast<u8*>(m_poolBottom);
			//u8 adjustment = *(temp - 1);
			//delete[] (temp - adjustment);
			io::log(io::LogLevel::LOG_INFO,
				"%s destruct %li bytes buffer for pool\n", m_name, m_poolSize + 16);
		}
		void* allocUnit()
		{
			m_mutex.lock();
			PoolUnit* r = m_availFstUnit;
			m_availFstUnit = m_availFstUnit->m_next;
			m_mutex.unlock();
			return r;
		}
		void freeUnit(void* unitPtr)
		{
			assert(((size_t)unitPtr - (size_t)m_poolBottom) % sizeof(PoolUnit) == 0 &&
				(size_t)unitPtr >= (size_t)m_poolBottom &&
				(size_t)unitPtr <= (size_t)m_poolBottom + m_poolSize);
			reinterpret_cast<PoolUnit*>(unitPtr)->m_next = m_availFstUnit;
			m_mutex.lock();
			m_availFstUnit = reinterpret_cast<PoolUnit*>(unitPtr);
			m_mutex.unlock();
		}
		void clear()
		{
			auto unitNum = m_poolSize / sizeof(PoolUnit);
			m_mutex.lock();
			m_availFstUnit = reinterpret_cast<PoolUnit*>(m_poolBottom);
			for (size_t i = 0; i != unitNum - 1; ++i)
			{
				m_availFstUnit[i].m_next = &m_availFstUnit[i + 1];
			}
			m_availFstUnit[unitNum - 1].m_next = nullptr;
			m_mutex.unlock();
		}
	private:
		struct PoolUnit
		{
			UnitType m_data; //Reserve the space
			PoolUnit* m_next = nullptr;
		};
		void* m_poolBottom;
		const size_t m_poolSize;
		PoolUnit* m_availFstUnit;

		IMutex m_mutex;
		char m_name[32];
	};

	/* Double end stack allocator */
	class DE_Allocator
	{
	public:
		using StackPtr = void*;
		DE_Allocator() = delete;
		DE_Allocator(void* stackBuffer, size_t size, const char* name = nullptr) :
			m_stackSize(size), m_isNewStack(false), m_availSize(size)
		{
			m_availBottomL = m_stackBottom = stackBuffer;
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);

			str::strnCpy(m_name, sizeof(m_name),
				name ? name : "StackAllocator", sizeof(m_name) - 1);
			m_name[sizeof(m_name) - 1] = '\0';
			//io::log(io::LogLevel::LOG_INFO,
			//	"%s construct %li bytes buffer for stack\n", m_name, m_stackSize);
		}
		DE_Allocator(size_t size, const char* name = nullptr) :
			m_stackSize(size), m_isNewStack(true), m_availSize(size)
		{
			m_availBottomL = m_stackBottom = new u8[size];
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);

			str::strnCpy(m_name, sizeof(m_name),
				name ? name : "StackAllocator", sizeof(m_name) - 1);
			m_name[sizeof(m_name) - 1] = '\0';
			io::Logger::log(io::Logger::LOG_INFO,
				"%s new and construct %li bytes buffer for stack\n", m_name, m_stackSize);
		}
		~DE_Allocator()
		{
			if (m_isNewStack)
				delete []m_stackBottom;
			io::Logger::log(io::Logger::LOG_INFO,
				"%s destruct %ld bytes buffer for stack\n", m_name, m_stackSize);
		}
		void* allocL(size_t size)
		{
			m_mutex.lock();
			if (size > m_availSize)
			{
				m_mutex.unlock();
				return nullptr;
			}

			void* r = m_availBottomL;
			m_availBottomL = reinterpret_cast<void*>((size_t)m_availBottomL + size);
			m_availSize -= size;
			m_mutex.unlock();
			return r;
		}
		template<typename T>
		inline T* allocL(int unitNum)
		{
			return reinterpret_cast<T*>(allocL(sizeof(T) * unitNum));
		}
		void freeL(StackPtr mark)
		{
			assert((size_t)mark <= (size_t)m_availBottomL &&
				(size_t)mark >= (size_t)m_stackBottom);

			m_mutex.lock();
			m_availSize += (size_t)m_availBottomL - (size_t)mark;
			m_availBottomL = mark;
			m_mutex.unlock();
		}
		void* allocAlignedL(size_t size, u8 alignment)
		{
			assert(alignment && (alignment % 2 == 0));

			size += alignment;
			size_t rawAddr = reinterpret_cast<size_t>(allocL(size));
			if (0 == rawAddr)
				return nullptr;
			u8 mask = alignment - 1;
			u8 adjustment = alignment - (rawAddr & mask);

			u8* r = reinterpret_cast<u8*>(rawAddr + adjustment);
			*(r - 1) = adjustment;
			return r;
		}
		template<typename T>
		inline T* allocAlignedL(int unitNum)
		{
			return reinterpret_cast<T*>(allocAlignedL(sizeof(T) * unitNum, alignof(T)));
		}
		void freeAlignedL(StackPtr mark)
		{
			u8* ptr = reinterpret_cast<u8*>(mark);
			u8 adjustment = *(ptr - 1);
			assert(adjustment);
			freeL(ptr - adjustment);
		}
		void* allocR(size_t size)
		{
			m_mutex.lock();
			if (size + sizeof(size_t) + sizeof(bool) > m_availSize)
			{
				m_mutex.unlock();
				return nullptr;
			}

			/* store the size before the block,
			store a mark for multi-threaded free */
			void* r = reinterpret_cast<void*>((size_t)m_availBottomR - size);
			m_availBottomR = reinterpret_cast<void*>((size_t)r - sizeof(size_t) - sizeof(bool));
			*reinterpret_cast<bool*>(m_availBottomR) = false;
			*reinterpret_cast<size_t*>((size_t)m_availBottomR + sizeof(bool)) = size;
			m_availSize -= size + sizeof(size_t) + sizeof(bool);
			m_mutex.unlock();

			return r;
		}
		template<typename T>
		inline T* allocR(int unitNum)
		{
			return reinterpret_cast<T*>(allocR(sizeof(T) * unitNum));
		}
		void freeR(StackPtr mark)
		{
			assert((size_t)mark > (size_t)m_availBottomR &&
				(size_t)mark <= (size_t)m_stackBottom + m_stackSize);
			m_mutex.lock();
			if ((size_t)mark == (size_t)m_availBottomR + sizeof(size_t) + sizeof(bool))
			{
				do
				{
					/* fetch the size */
					size_t size = *reinterpret_cast<size_t*>((size_t)mark - sizeof(size_t));
					/* Detect if the size had been overwritten */
					assert(size <= m_stackSize - m_availSize);

					m_availBottomR = reinterpret_cast<void*>((size_t)mark + size);

					m_availSize += size + sizeof(size_t) + sizeof(bool);
				} while ((size_t)m_availBottomR != (size_t)m_stackBottom + m_stackSize
					&& *(bool*)m_availBottomR);
			}
			else
			{
				*reinterpret_cast<bool*>((size_t)mark - sizeof(size_t) - sizeof(bool)) = true;
			}
			m_mutex.unlock();
		}
		void* allocAlignedR(size_t size, u8 alignment)
		{
			assert(alignment && (alignment % 2 == 0));

			size += alignment;
			size_t rawAddr = reinterpret_cast<size_t>(allocR(size));
			if (0 == rawAddr)
				return nullptr;
			u8 mask = alignment - 1;
			u8 adjustment = alignment - (rawAddr & mask);

			u8* r = reinterpret_cast<u8*>(rawAddr + adjustment);
			*(r - 1) = adjustment;
			return r;
		}
		template<typename T>
		inline T* allocAlignedR(int unitNum)
		{
			return reinterpret_cast<T*>(allocAlignedR(sizeof(T) * unitNum, alignof(T)));
		}
		void freeAlignedR(StackPtr mark)
		{
			u8* ptr = reinterpret_cast<u8*>(mark);
			u8 adjustment = *(ptr - 1);
			assert(adjustment);
			freeR(ptr - adjustment);
		}
		void clear()
		{
			m_mutex.lock();
			m_availBottomL = m_stackBottom;
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);
			m_availSize = m_stackSize;
			m_mutex.unlock();
		}
		void clearL()
		{
			freeL(m_stackBottom);
		}
		void clearR()
		{
			m_mutex.lock();
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);
			m_availSize = (size_t)m_availBottomR - (size_t)m_availBottomL;
			m_mutex.unlock();
		}

		const size_t getAvailSize() const
		{
			m_mutex.lock();
			auto ret = m_availSize;
			m_mutex.unlock();
			return ret;
		}

	private:
		const size_t m_stackSize;
		const bool m_isNewStack;
		void* m_stackBottom;

		void* m_availBottomL;
		void* m_availBottomR;
		size_t m_availSize;

		mutable IMutex m_mutex;
		char m_name[32];
	};

	class DE_Allocator_NoLock
	{
	public:
		using StackPtr = void*;
		DE_Allocator_NoLock() = delete;
		DE_Allocator_NoLock(void* stackBuffer, size_t size, const char*name = nullptr) :
			m_stackSize(size), m_isNewStack(false), m_availSize(size)
		{
			m_availBottomL = m_stackBottom = stackBuffer;
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);

			str::strnCpy(m_name, sizeof(m_name),
				name ? name : "StackAllocator", sizeof(m_name) - 1);
			m_name[sizeof(m_name) - 1] = '\0';
		}
		DE_Allocator_NoLock(size_t size, const char* name = nullptr) :
			m_stackSize(size), m_isNewStack(true), m_availSize(size)
		{
			m_availBottomL = m_stackBottom = new u8[size];
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);

			str::strnCpy(m_name, sizeof(m_name),
				name ? name : "StackAllocator", sizeof(m_name)-1);
			m_name[sizeof(m_name) - 1] = '\0';
			io::Logger::log(io::Logger::LOG_INFO,
				"%s new and construct %li bytes buffer for stack\n", m_name, m_stackSize);
		}
		~DE_Allocator_NoLock()
		{
			if (m_isNewStack)
				delete []m_stackBottom;
		}
		void* allocL(size_t size)
		{
			if (size > m_availSize)
				throw std::bad_alloc();

			void* r = m_availBottomL;
			m_availBottomL = reinterpret_cast<void*>((size_t)m_availBottomL + size);
			m_availSize -= size;
			return r;
		}
		template<typename T>
		inline T* allocL(int unitNum)
		{
			return reinterpret_cast<T*>(allocL(sizeof(T) * unitNum));
		}
		void freeL(StackPtr mark)
		{
			assert((size_t)mark <= (size_t)m_availBottomL &&
				(size_t)mark >= (size_t)m_stackBottom);

			m_availSize += (size_t)m_availBottomL - (size_t)mark;
			m_availBottomL = mark;
		}
		void* allocAlignedL(size_t size, u8 alignment)
		{
			assert(alignment && (alignment % 2 == 0));

			size += alignment;
			size_t rawAddr = reinterpret_cast<size_t>(allocL(size));
			if (0 == rawAddr)
				return nullptr;
			u8 mask = alignment - 1;
			u8 adjustment = alignment - (rawAddr & mask);

			u8* r = reinterpret_cast<u8*>(rawAddr + adjustment);
			*(r - 1) = adjustment;
			return r;
		}
		template<typename T>
		inline T* allocAlignedL(int unitNum)
		{
			return reinterpret_cast<T*>(allocAlignedL(sizeof(T) * unitNum, alignof(T)));
		}
		void freeAlignedL(StackPtr mark)
		{
			u8* ptr = reinterpret_cast<u8*>(mark);
			u8 adjustment = *(ptr - 1);
			assert(adjustment);
			freeL(ptr - adjustment);
		}
		void* allocR(size_t size)
		{
			if (size + sizeof(size_t) + sizeof(bool) > m_availSize)
				throw std::bad_alloc();

			/* store the size before the block,
			store a mark for multi-threaded free */
			void* r = reinterpret_cast<void*>((size_t)m_availBottomR - size);
			m_availBottomR = reinterpret_cast<void*>((size_t)r - sizeof(size_t) - sizeof(bool));
			*reinterpret_cast<bool*>(m_availBottomR) = false;
			*reinterpret_cast<size_t*>((size_t)m_availBottomR + sizeof(bool)) = size;
			m_availSize -= size + sizeof(size_t) + sizeof(bool);

			return r;
		}
		template<typename T>
		inline T* allocR(int unitNum)
		{
			return reinterpret_cast<T*>(allocR(sizeof(T) * unitNum));
		}
		void freeR(StackPtr mark)
		{
			assert((size_t)mark > (size_t)m_availBottomR &&
				(size_t)mark <= (size_t)m_stackBottom + m_stackSize);

			/* fetch the size */
			size_t size = *reinterpret_cast<size_t*>((size_t)mark - sizeof(size_t));
			/* Detect if the size had been overwritten */
			assert(size <= m_stackSize - m_availSize);
			
			m_availBottomR = reinterpret_cast<void*>((size_t)mark + size);

			m_availSize += size + sizeof(size_t);
		}
		void* allocAlignedR(size_t size, u8 alignment)
		{
			assert(alignment && (alignment % 2 == 0));

			size += alignment;
			size_t rawAddr = reinterpret_cast<size_t>(allocR(size));
			if (0 == rawAddr)
				return nullptr;
			u8 mask = alignment - 1;
			u8 adjustment = alignment - (rawAddr & mask);

			u8* r = reinterpret_cast<u8*>(rawAddr + adjustment);
			*(r - 1) = adjustment;
			return r;
		}
		template<typename T>
		inline T* allocAlignedR(int unitNum)
		{
			return reinterpret_cast<T*>(allocAlignedR(sizeof(T) * unitNum, alignof(T)));
		}
		void freeAlignedR(StackPtr mark)
		{
			u8* ptr = reinterpret_cast<u8*>(mark);
			u8 adjustment = *(ptr - 1);
			assert(adjustment);
			freeR(ptr - adjustment);
		}
		void clear()
		{
			m_availBottomL = m_stackBottom;
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);
			m_availSize = m_stackSize;
		}
		void clearL()
		{
			freeL(m_stackBottom);
		}
		void clearR()
		{
			m_availBottomR = (void*)((size_t)m_stackBottom + m_stackSize);
			m_availSize = (size_t)m_availBottomR - (size_t)m_availBottomL;
		}

		const size_t getAvailSize() const { return m_availSize; }

	private:
		const size_t m_stackSize;
		const bool m_isNewStack;
		void* m_stackBottom;

		void* m_availBottomL;
		void* m_availBottomR;
		size_t m_availSize;

		char m_name[32];
	};
}