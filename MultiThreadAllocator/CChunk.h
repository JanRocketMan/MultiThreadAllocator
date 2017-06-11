#pragma once
#include <cassert>
#include <cmath>
#include <cstring>

static size_t const CHUNKSIZE = pow(2, 13);
static float const f = 0.5;

extern class CMemoryTable;
class CChunk;

struct BlockInfo {
	BlockInfo() = default;
	BlockInfo(CChunk* ptr, int offset) : owner(ptr), 
		offsetToNext(offset) {}
	~BlockInfo() = default;
	CChunk* owner;
	int offsetToNext;
};

class CChunk {
public:
	CChunk() = delete;
	CChunk(size_t blockSize, size_t indexOfOwner) :
		usedSize(0), freeSize(CHUNKSIZE), blockSize(blockSize), next(nullptr),
		prev(nullptr), indexOfOwner(indexOfOwner), 
		size(CHUNKSIZE + floor(CHUNKSIZE / blockSize) * sizeof(BlockInfo)),
		blockWithInfoSize(blockSize + sizeof(BlockInfo)),
		nBlocks(floor(CHUNKSIZE / blockSize))
	{
		begin = (char*)malloc(size);
		freeBlock = begin + sizeof(BlockInfo);
		markData();
	}

	CChunk(CChunk&& other) = delete;
	CChunk(const CChunk& other) = delete;
	CChunk& operator=(const CChunk& other) = delete;
	~CChunk()
	{
		free(begin);
		if (next != nullptr) {
			delete next;
		}
	}

	void* chMalloc(size_t bytes)
	{
		assert(bytes <= blockSize);
		assert(bytes <= freeSize);

		usedSize += blockSize;
		freeSize -= blockSize;

		void* ptr = (void*)freeBlock;
		pop();
		return ptr;
	}

	void chFree(void* ptr)
	{
		char* ptr2 = (char*)ptr;
		assert(ptr2 - begin <= size && ptr2 - begin > 0);
		usedSize -= blockSize;
		freeSize += blockSize;
		push(ptr2);
	}

	bool IsFull()
	{
		return freeSize < blockSize;
	}

	bool IsDense()
	{
		return freeSize <= f * CHUNKSIZE;
	}

	bool IsThin()
	{
		return freeSize > f * CHUNKSIZE;
	}

	size_t usedSize;
	size_t freeSize;
	size_t const blockSize;
	CChunk* next;
	CChunk* prev;
	size_t indexOfOwner;
private:
	void markData() 
	{
		char* temp = begin;
		BlockInfo info(this, (int)blockWithInfoSize);
		for (size_t i = 0; i < nBlocks - 1; i++) {
			std::memcpy(temp, &info, sizeof(BlockInfo));
			temp += blockWithInfoSize;
		}
		info.offsetToNext = 0;
		std::memcpy(temp, &info, sizeof(BlockInfo));
	}

	void pop()
	{
		BlockInfo info;
		std::memcpy(&info, freeBlock - sizeof(BlockInfo), sizeof(BlockInfo));
		freeBlock += info.offsetToNext;
	}

	void push(char* ptr)
	{
		BlockInfo info(this, ptr - freeBlock);
		std::memcpy(freeBlock - sizeof(BlockInfo), &info, sizeof(BlockInfo));
		freeBlock = ptr;
	}
	
	char* begin;
	char* freeBlock;
	size_t const size;
	size_t const blockWithInfoSize;
	size_t const nBlocks;
};
