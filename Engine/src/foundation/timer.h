#pragma once
#include "service.h"
#include "platform.h"

KENSHIN_BEGIN

class Timer : public Service
{
public:
	virtual bool init(void* configuration = nullptr) override;
	virtual void shutdown() override;
	i64 now();
	i64 timeFrom(i64 startTime);
	i64 timeFromMilliseconds(i64 startTime);
	i64 timeFromSeconds(i64 startTime);
	i64 timeDelta(i64 endTime, i64 startTime);	
	i64 timeDeltaMilliseconds(i64 endTime, i64 startTime);
	i64 timeDeltaSeconds(i64 endTime, i64 startTime);
	KS_SERVICE_TYPE(Timer);
	constexpr static cstring typeName = "timer Service";
private:
	i64 int64MulDiv(i64 value, i64 numer, i64 denom);
	i64 seconds(i64 microseconds);
	i64 milliseconds(i64 microseconds);
private:
	LARGE_INTEGER mFrequency{};
};

KENSHIN_END