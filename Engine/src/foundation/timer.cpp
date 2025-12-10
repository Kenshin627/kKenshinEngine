#include "pch.h"
#include <Windows.h>
#include "timer.h"

KENSHIN_BEGIN

static Timer timer;

bool Timer::init(void* configuration)
{
	QueryPerformanceFrequency(&mFrequency);
	return true;
}

void Timer::shutdown()
{
}

i64 Timer::int64MulDiv(i64 value, i64 numer, i64 denom)
{
    const i64 q = value / denom;
    const i64 r = value % denom;
    return q * numer + r * numer / denom;
}

i64 Timer::seconds(i64 microseconds)
{
	return microseconds / 1000000LL;
}

i64 Timer::milliseconds(i64 microseconds)
{
	return microseconds / 1000LL;
}

i64 Timer::now()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return int64MulDiv(counter.QuadPart, 1000000LL, mFrequency.QuadPart);
}

i64 Timer::timeFrom(i64 startTime)
{
	i64 timeNow = now();
	return timeNow - startTime;
}

i64 Timer::timeFromMilliseconds(i64 startTime)
{
	return milliseconds(timeFrom(startTime));
}

i64 Timer::timeFromSeconds(i64 startTime)
{
	return seconds(timeFrom(startTime));
}

i64 Timer::timeDelta(i64 endTime, i64 startTime)
{
	return endTime - startTime;
}

i64 Timer::timeDeltaMilliseconds(i64 endTime, i64 startTime)
{
	return milliseconds(endTime - startTime);
}

i64 Timer::timeDeltaSeconds(i64 endTime, i64 startTime)
{
	return seconds(endTime - startTime);
}

Timer* Timer::instance()
{
	return &timer;
}

KENSHIN_END