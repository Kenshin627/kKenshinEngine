#pragma once

#include "platform.h"
#include "memory.h"

KENSHIN_BEGIN

struct GPUTimestamp
{
    u32                             start;
    u32                             end;
    f64                             elapsedMs;
    u16                             parentIndex;
    u16                             depth;
    u32                             color;
    u32                             frameIndex;
    const char* name;
};

struct GPUTimestampManager
{
    void init(Allocator* allocator, u16 queriesPerFrame, u16 maxFrames);
    void shutdown();
    bool hasValidQueries() const;
    void reset();
    u32  resolve(u32 current_frame, GPUTimestamp* timestamps_to_fill);    // Returns the total queries for this frame.
    u32  push(u32 current_frame, const char* name);    // Returns the timestamp query index.
    u32  pop(u32 current_frame);

    Allocator*    allocator = nullptr;
    GPUTimestamp* timestamps = nullptr;
    u64*          timestampsData = nullptr;
    u32           queriesPerFrame = 0;
    u32           currentQuery = 0;
    u32           parentIndex = 0;
    u32           depth = 0;
    bool          currentFrameResolved = false;    // Used to query the GPU only once per frame if get_gpu_timestamps is called more than once per frame.

};

KENSHIN_END