#include "pch.h"
#include "descriptorBuilder.h"
#include "gpuDevice.h"
#include "descriptorSetLayoutCache.h"
#include "descriptorSetAllocator.h"

KENSHIN_BEGIN

DescriptorBuilder DescriptorBuilder::begin(GPUDevice* device, DescriptorSetLayoutCache* cache, DescriptorSetAllocator* allocator)
{
    DescriptorBuilder builder;
    builder.mDevice = device;
    builder.mCache = cache;
    builder.mAllocator = allocator;
    return builder;
}

DescriptorBuilder& DescriptorBuilder::bindImage(u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkDescriptorImageInfo* imageInfo)
{
    VkDescriptorSetLayoutBinding newBinding{ .pImmutableSamplers = nullptr };
    newBinding.binding = binding;
    newBinding.descriptorType = descriptorType;
    newBinding.stageFlags = shaderStage;
    newBinding.descriptorCount = 1;
    newBinding.stageFlags = shaderStage;
    mBindings.push_back(newBinding);

    VkWriteDescriptorSet writer{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr };
    writer.descriptorCount = 1;
    writer.descriptorType = descriptorType;
    writer.dstArrayElement = 0;
    writer.dstBinding = 0;
    writer.pImageInfo = imageInfo;
    writer.pTexelBufferView = nullptr;
    mWrites.push_back(writer);
    return *this;
}

DescriptorBuilder& DescriptorBuilder::bindBuffer(u32 binding, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkDescriptorBufferInfo* bufferInfo)
{
    VkDescriptorSetLayoutBinding newBinding{ .pImmutableSamplers = nullptr };
    newBinding.binding = binding;
    newBinding.descriptorCount = 1;
    newBinding.descriptorType = descriptorType;
    newBinding.stageFlags = shaderStage;
    mBindings.push_back(newBinding);

    VkWriteDescriptorSet writer{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr };
    writer.descriptorCount = 1;
    writer.descriptorType = descriptorType;
    writer.dstArrayElement = 0;
    writer.dstBinding = binding;
    writer.pBufferInfo = bufferInfo;
    writer.pTexelBufferView = nullptr;
    mWrites.push_back(writer);

    return *this;
}

bool DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& setlayout)
{
    VkDescriptorSetLayoutCreateInfo setlayoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = nullptr };
    setlayoutInfo.flags = 0;
    setlayoutInfo.bindingCount = mBindings.size();
    setlayoutInfo.pBindings = mBindings.data();
    VkDescriptorSetLayout setLayout = mCache->createDescriptorSetLayout(setlayoutInfo);
    VkDescriptorSet descriptorSet;
    bool allocResult = mAllocator->allocateDescriptorSet(&descriptorSet, setLayout);
    if (!allocResult)
    {
        KS_CORE_ASSERT(false, "allocate descriptorSet failed!");
    }
    vkUpdateDescriptorSets(mDevice->getDevice(), mWrites.size(), mWrites.data(), 0, nullptr);
    return true;
}

bool DescriptorBuilder::build(VkDescriptorSet& set)
{
    VkDescriptorSetLayout setlayout;
    return build(set, setlayout);
}

KENSHIN_END
