#pragma once
#include "platform.h"
#include "hash_map.hpp"

KENSHIN_BEGIN

class Service;
class Allocator;

class ServiceManager
{
public:
	ServiceManager() = default;
	~ServiceManager() = default;
	bool init(Allocator* alloc);
	void shutdown();
	void addService(Service* service, cstring key);
	void removeService(cstring key);
	Service* getService(cstring key);
	ServiceManager* instance();
	template<typename T>
	T* get();
private:
	static ServiceManager sInstance;
	Allocator* mAllocator{ nullptr };
	FlatHashMap<u64, Service*> mServices;
};

template<typename T>
inline T* ServiceManager::get()
{
	Service* s = getService(T::typeName);
	if (s)
	{
		return static_cast<T*>(s);
	}
	else
	{
		return nullptr;
	}
}

KENSHIN_END