#pragma once
#include <stdio.h>
#include "platform.h"

KENSHIN_BEGIN

using FileHandle = FILE*;

class Allocator;

struct FileResult
{
	char* data;
	sizet size;
};

char* readTextFile(cstring file, Allocator* allocator, sizet* size);

char* readBinaryFile(cstring file, Allocator* allocator, sizet* size);

FileResult readTextFile(cstring file, Allocator* allocator);

FileResult readBinaryFile(cstring file, Allocator* allocator);

bool fileWriteBinary(cstring file, void* data, sizet size);

sizet fileDataSize(FileHandle file);

bool fileExists(cstring path);

void fileOpen(cstring filename, cstring mode, FileHandle* file);

void fileClose(FileHandle file);

sizet fileWrite(uint8_t* memory, u32 element_size, u32 count, FileHandle file);

bool fileDelete(cstring path);

KENSHIN_END
