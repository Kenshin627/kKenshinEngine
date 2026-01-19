#include "pch.h"
#include "resourceManager.h"

KENSHIN_BEGIN

void ResourceManager::init(Allocator* allocator, ResourceFilenameResolver* resolver) 
{
    this->allocator = allocator;
    this->filenameResolver = resolver;
    loaders.init(allocator, 8);
    compilers.init(allocator, 8);
}

void ResourceManager::shutdown() 
{
    loaders.shutdown();
    compilers.shutdown();
}

void ResourceManager::setLoader(cstring resource_type, ResourceLoader* loader) 
{
    const u64 hashed_name = hash_calculate(resource_type);
    loaders.insert(hashed_name, loader);
}

void ResourceManager::setCompiler(cstring resource_type, ResourceCompiler* compiler) 
{
    const u64 hashed_name = hash_calculate(resource_type);
    compilers.insert(hashed_name, compiler);
}

KENSHIN_END