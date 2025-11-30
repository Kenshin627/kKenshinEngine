#include "pch.h"
#include <SDL3/SDL.h>
#include "memory.h"
#include "array.h"

int main()
{
	Kenshin::Log::Init();
	KS_CORE_INFO("ENGINE INIT!");
	Kenshin::MemoryService* ms = Kenshin::MemoryService::instance();
	ms->init(nullptr);
	Kenshin::HeapAllocator allocator;
	allocator.init(kmega(1));
	Kenshin::Array<f32> points;
	points.init(&allocator, 10, 0);
	points.pushBack(2.0f);
	f32& newPoint = points.pushUse();
	newPoint = 3.0f;
	return 0;
}