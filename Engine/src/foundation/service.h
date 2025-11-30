#pragma once

namespace Kenshin
{
	class Service {
	public:
		Service() {};
		virtual ~Service() = default;
		virtual bool init(void* configuration = nullptr) = 0;
		virtual void shutdown() = 0;
		#define KS_SERVICE_TYPE(Type) static Type* instance();
	};
}