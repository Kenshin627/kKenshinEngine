#include "pch.h"
#include <SDL3/SDL.h>
#include "memory.h"
int main()
{
	Kenshin::Log::Init();
	KS_CORE_INFO("ENGINE INIT!");
	Kenshin::MemoryService* ms = Kenshin::MemoryService::instance();
	ms->init(nullptr);
	return 0;
}