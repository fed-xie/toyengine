#pragma once
#include "Types.h"

namespace toy
{
	class Timer
	{
	public:
		static void initTimerEnv();
		void init();
		//set time to now
		void reset();
		//return milliseconds from the last call of setTime()
		s64 getTime();
		
	private:
		s64 getClockTick();
		//return milliseconds from the last call of setTime() to tisk
		s64 TickToTime(s64 tick);
		s64 m_setTick;
	};
}