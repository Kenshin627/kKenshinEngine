#pragma once
#include <spdlog/spdlog.h>
#include <memory>

namespace Kenshin
{ 
	class Log
	{
	public:
		static void Init();
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return mCoreLogger; }
	private:
		static std::shared_ptr<spdlog::logger> mCoreLogger;
	};
	
	#define KS_CORE_TRACE(...) Kenshin::Log::GetCoreLogger()->trace(__VA_ARGS__)
	#define KS_CORE_INFO(...)  Kenshin::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define KS_CORE_WARN(...)  Kenshin::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define KS_CORE_ERROR(...) Kenshin::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define KS_CORE_FATAL(...) Kenshin::Log::GetCoreLogger()->critical(__VA_ARGS__)
} // namespace Kenshin

