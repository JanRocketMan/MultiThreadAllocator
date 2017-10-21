#pragma once
#include <cassert>
#include <cmath>

extern class Heap;

const size_t SuperBlockSize = pow(2, 14); // size in Bytes

class SuperBlock {
public:
	struct StoreInfo {
		StoreInfo() = default;
		StoreInfo(SuperBlock* own, int off) : owner(own), offsetToNext(off) {}
		~StoreInfo() = default;
		SuperBlock* owner;
		int offsetToNext;
	};

	SuperBlock(size_t _blockSize, Heap* owner) :
		usedSize(0), blockSize(_blockSize + sizeof(StoreInfo)), owner(owner)
	{
		nBlocks = floor(SuperBlockSize / blockSize);
		begin = (char*)malloc(SuperBlockSize);
		markupMemory();
	}

	~SuperBlock() = default;

	void ChangeBlockSize(size_t newBlocksize)
	{
		assert(usedSize == 0);
		blockSize = newBlocksize + sizeof(StoreInfo);
		nBlocks = floor(SuperBlockSize / blockSize);
		assert(freeBlock == begin + sizeof(StoreInfo));
		markupMemory();
	}

	void ChangeOwner(Heap* newOwner)
	{
		owner = newOwner;
	}

	void Clear()
	{
		free(begin);
	}

	void* SbMalloc(size_t bytes)
	{
		assert(bytes <= blockSize - sizeof(StoreInfo));
		assert(usedSize + blockSize <= SuperBlockSize);

		usedSize += blockSize;
		void* ptr = (void*)freeBlock;
		pop();

		return ptr;
	}

	void SbFree(void* ptr)
	{
		assert(usedSize >= blockSize);
		char* ptr2 = (char*)ptr;
		push(ptr2);
		usedSize -= blockSize;
	}

	size_t usedSize;
	size_t blockSize;
	Heap* owner;
	SuperBlock* next;
	SuperBlock* prev;
private:
	void markupMemory()
	{
		char* temp = begin + SuperBlockSize + sizeof(StoreInfo);
		freeBlock = temp;
		for (size_t i = 0; i < nBlocks; i++) {
			temp -= blockSize;
			push(temp);
		}
		assert(freeBlock == begin + sizeof(StoreInfo));
	}

	void pop()
	{
		StoreInfo info;
		memcpy(&info, freeBlock - sizeof(StoreInfo), sizeof(StoreInfo));
		freeBlock += info.offsetToNext;
	}

	void push(char* ptr)
	{
		StoreInfo info(this, int(freeBlock - ptr));
		memcpy(ptr - sizeof(StoreInfo), &info, sizeof(StoreInfo));
		freeBlock = ptr;
	}

	char* begin;
	char* freeBlock;
	size_t nBlocks;
};
