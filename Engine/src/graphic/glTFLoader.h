#pragma once
#include "platform.h"

KENSHIN_BEGIN



class GLTFLoader
{
public:
	GLTFLoader() = default;
	virtual ~GLTFLoader() = default;
	bool loadFromFile(cstring filename);
};

KENSHIN_END