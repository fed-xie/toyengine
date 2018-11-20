#pragma once
#include "Types.h"
#include "IThread.h"

namespace toy
{
	using TaskCall = ErrNo(*) (void* arg);
	
	struct alignas(CACHELINESIZE) TaskQuest
	{
		TaskCall tc; //task call function
		TaskCall cb; //callback function
		u8 padding[CACHELINESIZE - 2 * sizeof(TaskCall)/sizeof(u8)];
	};

	/* Multi-producer-single-consumer lock-free task queue,
	   it will have mistakes when the queue is full.
	!! qlen MUST be power of 2, DO NOT set mask */
	template<int qlen = 4096, int mask = qlen - 1>
	class tTaskQueue
	{
	public:
		void init() { assert(2 == ATOMIC_INT_LOCK_FREE); }

		TaskQuest* createQuest()
		{
			m_inCount.fetch_add(1, std::memory_order_relaxed);
			int in = m_dqi.fetch_add(1, std::memory_order_relaxed) & mask;
			m_dqi.fetch_and(mask, std::memory_order_relaxed);
			return &m_dq[in];
		}

		void push(TaskQuest* quest)
		{
			int dqOffset = static_cast<int>(quest - m_dq);
			int iqOffset = m_iqwi.fetch_add(1, std::memory_order_relaxed) & mask;
			m_iqwi.fetch_and(mask, std::memory_order_relaxed);
			m_iq[iqOffset] = dqOffset;

			atomic_thread_fence(std::memory_order_relaxed);

			int iqo = iqOffset;
			while (!m_iqri.compare_exchange_strong(iqo, (iqOffset + 1) & mask,
					std::memory_order_acq_rel))
				iqo = iqOffset;
		}

		void run()
		{
			int ri = m_iqri.load(std::memory_order_relaxed);
			while (m_iqo != ri)
			{
				auto &quest = m_dq[m_iq[m_iqo]];
				if (quest.tc)
				{
					if (TOY_OK == quest.tc(quest.padding) && quest.cb)
						quest.cb(quest.padding);
				}
				_Compiler_barrier();
				m_iqo = (m_iqo + 1) & mask;

				++m_runCount;
			}
		}

		/* functions help to manage the number of task */
		void resetRunCount() { m_runCount = 0; }
		int getRunCount() { return m_runCount; }
		void resetInputCount() { m_inCount.store(0, std::memory_order_relaxed); }
		int getInputCount() { return m_inCount.load(std::memory_order_relaxed); }

	private:
		TaskQuest m_dq[qlen]; //Data queue

		std::atomic_int m_dqi = 0;
		std::atomic_int m_inCount = 0;
		u8 padding1[CACHELINESIZE - 2 * sizeof(std::atomic_int) / sizeof(u8)];

		std::atomic_int m_iqwi = 0;
		std::atomic_int m_iqri = 0;
		u8 padding2[CACHELINESIZE - 2 * sizeof(std::atomic_int) / sizeof(u8)];
		int m_iq[qlen] = { 0 }; //Index queue
		
		int m_iqo = 0;
		int m_runCount = 0;
	};
}