#pragma once
#include "kassert.h"
#include "platform.h"
#include "hash_map.hpp"

KENSHIN_BEGIN

struct ResourceManager;

struct Resource {

    void add_reference() 
    { 
        ++references; 
    }
    void remove_reference() 
    { 
        KS_CORE_ASSERT(references != 0, "references has already is zero!");
        --references; 
    }

    u64     references { 0       };
    cstring name       { nullptr };

}; 

struct ResourceCompiler 
{

}; 


struct ResourceLoader 
{
    virtual Resource* get(cstring name) = 0;
    virtual Resource* get(u64 hashed_name) = 0;
    virtual Resource* unload(cstring name) = 0;
    virtual Resource* createFromFile(cstring name, cstring filename, Kenshin::ResourceManager* resource_manager) 
    { 
        return nullptr; 
    }

}; 

struct ResourceFilenameResolver {

    virtual cstring getBinaryPathFromName(cstring name) = 0;

}; 

struct ResourceManager {

    void init(Allocator* allocator, ResourceFilenameResolver* resolver);
    void shutdown();

    template <typename T>
    T* load(cstring name);

    template <typename T>
    T* get(cstring name);

    template <typename T>
    T* get(u64 hashed_name);

    template <typename T>
    T* reload(cstring name);

    void setLoader(cstring resource_type, ResourceLoader* loader);
    void setCompiler(cstring resource_type, ResourceCompiler* compiler);

    FlatHashMap<u64, ResourceLoader*>       loaders;
    FlatHashMap<u64, ResourceCompiler*>     compilers;
    Allocator*                              allocator{ nullptr };
    ResourceFilenameResolver*               filenameResolver{ nullptr };;
};

template<typename T>
inline T* ResourceManager::load(cstring name) 
{
    ResourceLoader* loader = loaders.get(T::k_type_hash);
    if (loader) {
        T* resource = (T*)loader->get(name);
        if (resource)
        {
            return resource;
        }
        cstring path = filenameResolver->getBinaryPathFromName(name);
        return static_cast<T*>(loader->create_from_file(name, path, this));
    }
    return nullptr;
}

template<typename T>
inline T* ResourceManager::get(cstring name) 
{
    ResourceLoader* loader = loaders.get(T::k_type_hash);
    if (loader) 
    {
        return  static_cast<T*>(loader->get(name));
    }
    return nullptr;
}

template<typename T>
inline T* ResourceManager::get(u64 hashed_name) 
{
    ResourceLoader* loader = loaders.get(T::k_type_hash);
    if (loader) 
    {
        return static_cast<T*>(loader->get(hashed_name));
    }
    return nullptr;
}

template<typename T>
inline T* ResourceManager::reload(cstring name) {
    ResourceLoader* loader = loaders.get(T::k_type_hash);
    if (loader) {
        T* resource = static_cast<T*>(loader->get(name));
        if (resource) 
        {
            loader->unload(name);
            cstring path = filenameResolver->getBinaryPathFromName(name);
            return  static_cast<T*>(loader->create_from_file(name, path, this));
        }
    }
    return nullptr;
}

KENSHIN_END