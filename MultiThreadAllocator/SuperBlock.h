#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <memory>
#include <vector>

using std::size_t;
class Heap;

class SuperBlock;

static void SetSbckOwner(void* sbck, SuperBlock *owner) {
	((SuperBlock **)sbck)[-1] = owner;
}

static const size_t SuperBlockSize = (size_t)pow(2, 14); // size of superblock in bytes
static const size_t AdditionalSize = sizeof(void*);

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

	void* SbMalloc()
	{
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
	std::atomic<Heap*> owner;
private:

	void markupMemory()
	{
		freeBlocks.reserve(nBlocks);
		for (size_t i = nBlocks; i > 0; i--) {
			char* ptr = begin + AdditionalSize + (i - 1) * blockSize;
			freeBlocks.push_back(ptr);
			SetSbckOwner(ptr, this);
		}
		assert(freeBlocks.size() == nBlocks);
	}

	char* begin;
	std::vector<void*> freeBlocks;
	const size_t nBlocks;
};

