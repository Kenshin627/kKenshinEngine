#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "platform.h"
#include "typeDefs.h"

KENSHIN_BEGIN

struct Viewport
{
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};

struct Scissor
{
	float offsetX;
	float offsetY;
	float width;
	float height;
};

struct Buffer
{
	VkBuffer vkBuffer;
	BufferHandle handle{ InvalidBuffer };
	BufferHandle mParentHandle{ InvalidBuffer };
	u32 offset;
	VmaAllocation allocation;
	VkDeviceSize size;
	VkDeviceMemory memory;
};

struct ComputeGroupSize
{
	u32 xSize;
	u32 ySize;
	u32 zSize;
};
KENSHIN_END