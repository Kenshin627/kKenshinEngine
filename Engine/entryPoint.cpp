#include "pch.h"
#include <SDL3/SDL.h>
int main()
{
	Kenshin::Log::Init();
	KS_CORE_INFO("ENGINE INIT!");
	std::cout << "123 " << std::endl;
	return 0;
}