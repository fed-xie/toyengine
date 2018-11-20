#include "include/Timer.h"

#ifdef _WIN64
#include <Windows.h>

namespace toy
{
	static LARGE_INTEGER sysFreq;
	static LARGE_INTEGER perfCounter;
	void Timer::initTimerEnv()
	{
		QueryPerformanceFrequency(&sysFreq);
		QueryPerformanceCounter(&perfCounter);
	}
	void Timer::init()
	{
		reset();
	}
	void Timer::reset()
	{
		m_setTick = getClockTick();
	}
	s64 Timer::getTime()
	{
		return (getClockTick() - m_setTick) * 1000LL / sysFreq.QuadPart;
	}
	s64 Timer::getClockTick()
	{
		QueryPerformanceCounter(&perfCounter);
		return perfCounter.QuadPart;
	}
	s64 Timer::TickToTime(s64 tick)
	{
		return (tick - m_setTick) * 1000LL / sysFreq.QuadPart;
	}
}
#endif //_WIN64