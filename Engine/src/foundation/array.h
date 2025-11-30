#pragma once
#include "memory.h"

KENSHIN_BEGIN

class Allocator;

template<typename T>
class Array
{
public:
	Array() = default;
	~Array() = default;
	void init(Allocator* alloc, sizet capacity, sizet size);
	void shutdown();
	void grow(sizet newCapacity);
	void pushBack(const T& v);
	T& pushUse();
	void pop();
	T& operator[](sizet index);
	const T& operator[](sizet index) const;
	void deleteSwap(sizet index);
	T& front();
	const T& front() const;
	T& back();
	const T& back() const;
	void setSize(sizet newSize);
	void setCapacity(sizet newCapacity);
	sizet sizeByBytes() const;
	sizet capacityByBytes() const;
	void clear();
private:
	T*		   mData		{ nullptr };
	sizet	   mCapacity	{ 0		  };
	sizet	   mSize		{ 0		  };
	Allocator* mAllocator	{ nullptr };
};

template<typename T>
void Array<T>::init(Allocator* alloc, sizet capacity, sizet size)
{
	KS_CORE_ASSERT(alloc, "Allocator is nullptr!");
	mAllocator = alloc;
	mSize = size;
	mData = nullptr;
	if (capacity)
	{
		grow(capacity);
	}
}

template<typename T>
void Array<T>::shutdown()
{
	if (mData)
	{
		mAllocator->deallocate(mData);
	}
	mCapacity = 0;
	mSize = 0;
	mData = nullptr;
}

template<typename T>
void Array<T>::grow(sizet newCapacity)
{
	if (newCapacity < mCapacity * 2)
	{
		newCapacity = mCapacity * 2;
	}
	else if (newCapacity < 4)
	{
		newCapacity = 4;
	}

	T* newData = static_cast<T*>(mAllocator->allocate(newCapacity * sizeof(T), sizeof(T)));
	if (mCapacity)
	{
		memcpy(newData, mData, mCapacity * sizeof(T));
		mAllocator->deallocate(mData);
	}
	mData = newData;
	mCapacity = newCapacity;
}

template<typename T>
void Array<T>::pushBack(const T& v)
{
	if (mSize >= mCapacity)
	{
		grow(mCapacity + 1);
	}
	mData[mSize++] = v;
}

template<typename T>
T& Array<T>::pushUse()
{
	if (mSize >= mCapacity)
	{
		grow(mCapacity + 1);
	}
	++mSize;
	return back();
}

template<typename T>
void Array<T>::pop()
{
	KS_CORE_ASSERT(mSize > 0, "Array is empty, cannot pop!");
	--mSize;
}

template<typename T>
T& Array<T>::operator[](sizet index)
{
	KS_CORE_ASSERT(index < mSize && index >= 0, "Array index out of bounds!");
	return mData[index];
}

template<typename T>
const T& Array<T>::operator[](sizet index) const
{
	KS_CORE_ASSERT(index < mSize && index >= 0, "Array index out of bounds!");
	return mData[index];
}

template<typename T>
void Array<T>::deleteSwap(sizet index)
{
	KS_CORE_ASSERT(index < mSize && index >= 0, "Array index out of bounds!");
	mData[index] = mData[--mSize];
}

template<typename T>
T& Array<T>::front()
{
	KS_CORE_ASSERT(mSize > 0, "Array is empty, no front element!");
	return mData[0];
}

template<typename T>
const T& Array<T>::front() const
{
	KS_CORE_ASSERT(mSize > 0, "Array is empty, no front element!");
	return mData[0];
}

template<typename T>
T& Array<T>::back()
{
	KS_CORE_ASSERT(mSize > 0, "Array is empty, no back element!");
	return mData[mSize - 1];
}

template<typename T>
const T& Array<T>::back() const
{
	KS_CORE_ASSERT(mSize > 0, "Array is empty, no back element!");
	return mData[mSize - 1];
}

template<typename T>
void Array<T>::setSize(sizet newSize)
{
	if (newSize > mCapacity)
	{
		grow(newSize);
	}
	mSize = newSize;;
}

template<typename T>
void Array<T>::setCapacity(sizet newCapacity)
{
	if (newCapacity > mCapacity)
	{
		grow(newCapacity);
	}
}

template<typename T>
sizet Array<T>::sizeByBytes() const
{
	return mSize * sizeof(T);
}

template<typename T>
sizet Array<T>::capacityByBytes() const
{
	return mCapacity * sizeof(T);
}

template<typename T>
void Array<T>::clear()
{
	mSize = 0;
}

KENSHIN_END