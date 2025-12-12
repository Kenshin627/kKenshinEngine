#include "pch.h"
#include "serviceManager.h"
#include "logger.h"

KENSHIN_BEGIN

ServiceManager ServiceManager::sInstance;

bool ServiceManager::init(Allocator* alloc)
{
	KS_CORE_INFO("Initializing Service Manager.");
	mAllocator = alloc;
	mServices.init(mAllocator, 8);
	return true;
}

void ServiceManager::shutdown()
{
	KS_CORE_INFO("Shutting down Service Manager.");
	mServices.shutdown();
}

void ServiceManager::addService(Service* service, cstring key)
{
	u64 hashCode = hash_calculate(key);
	FlatHashMapIterator it = mServices.find(hashCode);
	if (it.is_valid())
	{
		KS_CORE_WARN("Service with key '{}' already exists in ServiceManager!", key);
	}
	mServices.insert(hashCode, service);
}

void ServiceManager::removeService(cstring key)
{
	u64 hashCode = hash_calculate(key);
	mServices.remove(hashCode);
}

Service* ServiceManager::getService(cstring key)
{
	u64 hashCode = hash_calculate(key);
	FlatHashMapIterator it = mServices.find(hashCode);	
	if (it.is_invalid())
	{
		KS_CORE_WARN("Service with key '{}' not found in ServiceManager!", key);
		return nullptr;
	}
	return mServices.get_structure(it).value;
}

ServiceManager* ServiceManager::instance()
{
	return &sInstance;
}

KENSHIN_END