#pragma once
#include <cassert>
#include <cmath>
#include <ctime>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>

extern class Heap;

size_t nInits = 0;
size_t nMallocs = 0;
size_t nFrees = 0;
double avgInit = 0;
double avgMalloc = 0;
double avgFree = 0;

class SuperBlock {
public:
	struct StoreInfo {
		StoreInfo() = default;
		StoreInfo(SuperBlock* own, int off) : owner(own), offsetToNext(off) {}
		~StoreInfo() = default;
		SuperBlock* owner;
		int offsetToNext;
	};

	SuperBlock(size_t size, size_t blockSize, Heap* owner) :
		usedSize(0), blockSize(blockSize), owner(owner), size(size),
		blockWithInfoSize(blockSize + sizeof(StoreInfo)), nBlocks(floor(size / blockSize))
	{
		nInits++;
		avgInit *= (double)(nInits - 1) / nInits;
		clock_t t = clock();

		reserveMemory();
		freeBlock = begin + sizeof(StoreInfo);

		avgInit += ((double)clock() - t) / CLOCKS_PER_SEC / nInits;
	}

	SuperBlock(const SuperBlock& other) = default;
	SuperBlock& operator=(const SuperBlock& other) = default;
 
	~SuperBlock() = default;

	void Clear()
	{
		free(begin);
	}

	void* SbMalloc(size_t bytes)
	{
		nMallocs++;
		avgMalloc *= (double)(nMallocs - 1) / nMallocs;
		clock_t t = clock();

		assert(bytes <= blockSize);
		assert(usedSize + blockSize <= size);

		usedSize += blockSize;
		void* ptr = (void*)freeBlock;
		pop();

		avgMalloc += ((double)clock() - t) / CLOCKS_PER_SEC / nMallocs;
		return ptr;
	}

	void SbFree(void* ptr)
	{
		nFrees++;
		avgFree *= (double)(nFrees - 1) / nFrees;
		clock_t t = clock();

		char* ptr2 = (char*)ptr;
		//int diff = ptr2 - begin - sizeof(StoreInfo);
		//assert(diff % blockWithInfoSize == 0);
		//assert(diff / blockWithInfoSize < nBlocks);
		usedSize -= blockSize;
		push(ptr2);

		avgFree += ((double)clock() - t) / CLOCKS_PER_SEC / nFrees;
	}

	size_t usedSize;
	size_t blockSize;
	Heap* owner;
private:
	void reserveMemory()
	{
		begin = (char*)malloc(size + nBlocks * sizeof(StoreInfo));
		char* temp = begin;
		StoreInfo info(this, blockWithInfoSize);
		for (size_t i = 0; i < nBlocks - 1; i++) {
			memcpy(temp, &info, sizeof(StoreInfo));
			temp += blockWithInfoSize;
		}
		info.offsetToNext = 0;
		memcpy(temp, &info, sizeof(StoreInfo));
	}

	void pop()
	{
		StoreInfo info;
		memcpy(&info, freeBlock - sizeof(StoreInfo), sizeof(StoreInfo));
		freeBlock += info.offsetToNext;
	}

	void push(char* ptr)
	{
		StoreInfo info;
		memcpy(&info, freeBlock - sizeof(StoreInfo), sizeof(StoreInfo));
		info.offsetToNext = int(ptr - freeBlock);
		memcpy(freeBlock - sizeof(StoreInfo), &info, sizeof(StoreInfo));
		freeBlock = ptr;
	}

	char* begin;
	char* freeBlock;
	size_t size;
	size_t blockWithInfoSize;
	size_t nBlocks;
};
