#include "pch.h"
#include "file.h"
#include "memory.h"

KENSHIN_BEGIN

char* readTextFile(cstring file, Allocator* allocator, sizet* size)
{
    FileHandle f;
    errno_t res = fopen_s(&f, file, "r");
    if (res == 0 && f)
    {
        sizet fileSize = fileDataSize(f);
        char* data = reinterpret_cast<char*>(allocator->allocate(fileSize + 1, 1));
        sizet bytesRead = fread(data, 1, fileSize, f);
        data[bytesRead] = 0;
        fclose(f);
        if (size)
        {
            *size = fileSize;
        }
        return data;
    }
    return nullptr;
}

char* readBinaryFile(cstring file, Allocator* allocator, sizet* size)
{
    FileHandle f;
    auto res = fopen_s(&f, file, "rb");
    if (res == 0 && f)
    {
        sizet fileSize = fileDataSize(f);
        char* data = reinterpret_cast<char*>(allocator->allocate(fileSize, 1));
        fread(data, fileSize, 1, f);
        fclose(f);
        if (size)
        {
            *size = fileSize;
            return data;
        }
    }
    return nullptr;
}

FileResult readTextFile(cstring file, Allocator* allocator)
{
    FileResult res;
	res.data = readTextFile(file, allocator, &res.size);
    return res;
}

FileResult readBinaryFile(cstring file, Allocator* allocator)
{
    FileResult res;
    res.data = readBinaryFile(file, allocator, &res.size);
    return res;
}

bool fileWriteBinary(cstring file, void* data, sizet size)
{
    FileHandle f;
    auto res = fopen_s(&f, file, "wb");
    if (f)
    {
        fwrite(data, size, 1, f);
        fclose(f);
        return true;
    }
    return false;
}

sizet fileDataSize(FileHandle handle)
{
    long size;
    fseek(handle, 0, SEEK_END);
    size = ftell(handle);
    fseek(handle, 0, SEEK_SET);
    return static_cast<sizet>(size);
}

bool fileExists(cstring path)
{
   //TODO
    return false;
}

void fileOpen(cstring filename, cstring mode, FileHandle* file)
{
   fopen_s(file, filename, mode);
}

void fileClose(FileHandle file)
{
    if (file)
    {
        fclose(file);
    }
}

sizet fileWrite(uint8_t* memory, u32 elementSize, u32 count, FileHandle file)
{
    return fwrite(memory, elementSize, count, file);
}

bool fileDelete(cstring path)
{
    int res = remove(path);
    return  res == 0;
}

KENSHIN_END