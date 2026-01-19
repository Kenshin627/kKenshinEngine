#include "pch.h"
#include "gpuTimestampManager.h"

KENSHIN_BEGIN

void GPUTimestampManager::init(Allocator* allocator, u16 queriesPerFrame, u16 maxFrames) 
{
    allocator = allocator;
    queriesPerFrame = queriesPerFrame;

    // Data is start, end in 2 u64 numbers.
    const u32 k_data_per_query = 2;
    const sizet allocated_size = sizeof(GPUTimestamp) * queriesPerFrame * maxFrames + sizeof(u64) * queriesPerFrame * maxFrames * k_data_per_query;
    u8* memory = kallocm(allocated_size, allocator);

    timestamps = (GPUTimestamp*)memory;
    // Data is start, end in 2 u64 numbers.
    timestampsData = (u64*)(memory + sizeof(GPUTimestamp) * queriesPerFrame * maxFrames);
    reset();
}

void GPUTimestampManager::shutdown() 
{
    kfree(timestamps, allocator);
}

void GPUTimestampManager::reset() 
{
    currentQuery = 0;
    parentIndex = 0;
    currentFrameResolved = false;
    depth = 0;
}

bool GPUTimestampManager::hasValidQueries() const 
{
    // Even number of queries means asymettrical queries, thus we don't sample.
    return currentQuery > 0 && (depth == 0);
}

u32 GPUTimestampManager::resolve(u32 current_frame, GPUTimestamp* timestamps_to_fill) 
{
    memoryCopy(timestamps_to_fill, &timestamps[current_frame * queriesPerFrame], sizeof(GPUTimestamp) * currentQuery);
    return currentQuery;
}

u32 GPUTimestampManager::push(u32 current_frame, const char* name) 
{
    u32 query_index = (current_frame * queriesPerFrame) + currentQuery;
    GPUTimestamp& timestamp = timestamps[query_index];
    timestamp.parentIndex = (u16)parentIndex;
    timestamp.start = query_index * 2;
    timestamp.end = timestamp.start + 1;
    timestamp.name = name;
    timestamp.depth = (u16)depth++;

    parentIndex = currentQuery;
    ++currentQuery;
    return (query_index * 2);
}

u32 GPUTimestampManager::pop(u32 current_frame) 
{
    u32 query_index = (current_frame * queriesPerFrame) + parentIndex;
    GPUTimestamp& timestamp = timestamps[query_index];
    // Go up a level
    parentIndex = timestamp.parentIndex;
    --depth;
    return (query_index * 2) + 1;
}

KENSHIN_END