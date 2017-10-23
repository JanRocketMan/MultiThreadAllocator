#pragma once
#include <cassert>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

using std::size_t;
class Heap;

class SuperBlock;

struct StoreInfo {
	StoreInfo(SuperBlock* _owner = nullptr) : owner(_owner) {}
	~StoreInfo() = default;
	SuperBlock* owner;
};

const size_t SuperBlockSize = (size_t) pow(2, 14); // size of superblock in bytes
const size_t AdditionalSize = sizeof(StoreInfo);

class SuperBlock {
public:

	SuperBlock() = delete;

	SuperBlock(size_t _blockSize, Heap* owner) :
		usedSize(0), blockSize(_blockSize), owner(owner),
		nBlocks((size_t)floor((float)SuperBlockSize / (float)blockSize))
	{
		begin = (char*)malloc(SuperBlockSize);
		markupMemory();
	}

	~SuperBlock() 
	{
		assert(freeBlocks.size() == nBlocks);
		free(begin);
	}

	void* SbMalloc(size_t bytes)
	{
		assert(bytes <= blockSize);
		assert(usedSize + blockSize <= SuperBlockSize);

		usedSize += blockSize;
		void* ptr = freeBlocks.back();
		freeBlocks.pop_back();

		return ptr;
	}

	void SbFree(void* ptr)
	{
		assert(usedSize >= blockSize);
		freeBlocks.push_back(ptr);
		usedSize -= blockSize;
	}

	size_t usedSize;
	const size_t blockSize;
	Heap* owner;
private:
	void markupMemory()
	{
		char* temp = begin + SuperBlockSize + AdditionalSize;
		StoreInfo info(this);
		for (size_t i = 0; i < nBlocks; i++) {
			temp -= blockSize;
			memcpy(temp - AdditionalSize, &info, AdditionalSize);
			freeBlocks.push_back(temp);
		}
		assert(freeBlocks.size() == nBlocks);
	}

	char* begin;
	std::vector<void*> freeBlocks;
	const size_t nBlocks;
};
