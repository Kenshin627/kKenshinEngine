#include "pch.h"
#include "descriptorSetLayoutCache.h"
#include "gpuDevice.h"

KENSHIN_BEGIN

void DescriptorSetLayoutCache::init(GPUDevice* device)
{
	mDevice = device;
}

void DescriptorSetLayoutCache::shutDown()
{
	for (auto& [k, v] : mSetLayoutCache)
	{
		vkDestroyDescriptorSetLayout(mDevice->getDevice(), v, mDevice->getAllocCallbacks());
	}
	mSetLayoutCache.clear();
}

VkDescriptorSetLayout DescriptorSetLayoutCache::createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& creation)
{
	DescriptorSetLayoutCreateInfo setlayoutCreateInfo{};
	setlayoutCreateInfo.bindings.reserve(creation.bindingCount);
	int lastBinding = -1;
	bool isSorted = false;
	for (u32 i = 0; i < creation.bindingCount; ++i)
	{
		setlayoutCreateInfo.bindings.push_back(creation.pBindings[i]);
		if (!isSorted)
		{
			if (setlayoutCreateInfo.bindings[i].binding > lastBinding)
			{
				lastBinding = setlayoutCreateInfo.bindings[i].binding;
				isSorted = false;
			}
			else
			{
				isSorted = true;
			}
		}		
	}
	if (!isSorted)
	{
		std::sort(setlayoutCreateInfo.bindings.begin(), setlayoutCreateInfo.bindings.end(), [](const VkDescriptorSetLayoutBinding& b1, const VkDescriptorSetLayoutBinding& b2)
		{
			return b1.binding < b2.binding;
		});
	}
	auto iter = mSetLayoutCache.find(setlayoutCreateInfo);
	if (iter != mSetLayoutCache.cend())
	{
		return iter->second;
	}
	else
	{
		VkDescriptorSetLayout layout;
		VK_CHECK(vkCreateDescriptorSetLayout(mDevice->getDevice(), &creation, mDevice->getAllocCallbacks(), &layout));
		mSetLayoutCache.insert({ setlayoutCreateInfo , layout });
	}
}

bool DescriptorSetLayoutCache::DescriptorSetLayoutCreateInfo::operator==(const DescriptorSetLayoutCreateInfo& other) const
{
	if (other.bindings.size() != bindings.size())
	{
		return false;
	}
	else {
		for (int i = 0; i < bindings.size(); i++) {
			if (other.bindings[i].binding != bindings[i].binding)
			{
				return false;
			}
			if (other.bindings[i].descriptorType != bindings[i].descriptorType)
			{
				return false;
			}
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount)
			{
				return false;
			}
			if (other.bindings[i].stageFlags != bindings[i].stageFlags)
			{
				return false;
			}
		}
		return true;
	}
}

sizet DescriptorSetLayoutCache::DescriptorSetLayoutCreateInfo::hash() const
{
	sizet result = std::hash<sizet>()(bindings.size());
	for (const VkDescriptorSetLayoutBinding& b : bindings)
	{
		size_t bindingHash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

		//shuffle the packed binding data and xor it with the main hash
		result ^= std::hash<sizet>()(bindingHash);
	}

	return result;
}

KENSHIN_END

