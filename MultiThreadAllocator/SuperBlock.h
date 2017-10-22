#pragma once
#include <cassert>
#include <cmath>
#include <vector>

class Heap;

const size_t SuperBlockSize = (int) pow(2, 14); // size in Bytes

class SuperBlock {
public:
	struct StoreInfo {
		StoreInfo() = default;
		StoreInfo(SuperBlock* _owner) : owner(_owner) {}
		~StoreInfo() = default;
		SuperBlock* owner;
	};

	SuperBlock(size_t _blockSize, Heap* owner) :
		usedSize(0), blockSize(_blockSize), owner(owner),
		nBlocks((size_t)floor((float)SuperBlockSize / (float)blockSize))
	{
		begin = (char*)malloc(SuperBlockSize);
		next = nullptr;
		prev = nullptr;
		markupMemory();
	}

	~SuperBlock() {
		free(begin);
	}

	void ChangeOwner(Heap* newOwner)
	{
		owner = newOwner;
	}

	void* SbMalloc(size_t bytes)
	{
		assert(bytes <= blockSize);
		assert(usedSize + blockSize <= SuperBlockSize);

		usedSize += blockSize;
		void* ptr = freeblocks.back();
		freeblocks.pop_back();

		return ptr;
	}

	void SbFree(void* ptr)
	{
		assert(usedSize >= blockSize);
		freeblocks.push_back(ptr);
		usedSize -= blockSize;
	}

	size_t usedSize;
	const size_t blockSize;
	Heap* owner;
	SuperBlock* next;
	SuperBlock* prev;
private:
	void markupMemory()
	{
		char* temp = begin + SuperBlockSize + sizeof(StoreInfo);
		StoreInfo info(this);
		for (size_t i = 0; i < nBlocks; i++) {
			temp -= blockSize;
			memcpy(temp - sizeof(StoreInfo), &info, sizeof(StoreInfo));
			freeblocks.push_back(temp);
		}
		assert(freeblocks.size() == nBlocks);
	}

	char* begin;
	std::vector<void*> freeblocks;
	const size_t nBlocks;
};
