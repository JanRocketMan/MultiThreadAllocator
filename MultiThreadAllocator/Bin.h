#pragma once
#include "SuperBlock.h"
#include <algorithm>
#include <array>

const float f = 0.5;

class Bin {
public:
	std::array<std::vector<SuperBlock*>, 3> listsOfBlocks; 
	// 0 - density < f; 1 - density >= f; 2 - density == 1 
	// density = usedSize / SuperBlockSize
	Bin() = default;
	~Bin() 
	{
		for (size_t i = 0; i < 3; i++) {
			for (auto it = listsOfBlocks[i].begin(); it != listsOfBlocks[i].end(); it++) {
				delete (*it);
			}
		}
	}

	SuperBlock* GetBlock() 
	{
		SuperBlock* ans = nullptr;
		for (size_t i = 0; i < 2; i++) {
			ans = pop(i);
			if (ans != nullptr) {
				break;
			}
		}
		return ans;
	}

	SuperBlock* GetBlock(size_t index)
	{
		return pop(index);
	}

	void AddBlock(SuperBlock* blockToAdd, bool forMalloc)
	{
		push(getIndex(blockToAdd, forMalloc), blockToAdd);
	}

	void UpdateBlockAfterFree(SuperBlock* blockToUpdate) 
	{
		size_t i = getIndex(blockToUpdate, true);
		bool res = pullOut(i, blockToUpdate);
		assert(res);
		SuperBlock* block = pop(i);
		assert(block == blockToUpdate);
		push(getIndex(block, false), block);
	}

private:
	size_t getIndex(SuperBlock* sbck, bool nextIsMallocOrFreeWasBefore) {
		float density = 0;
		if (nextIsMallocOrFreeWasBefore) {
			density = (float)(sbck->usedSize + sbck->blockSize) / (float)SuperBlockSize;
		}
		else {
			density = (float)(sbck->usedSize) / (float)SuperBlockSize;
		}
		
		if (density < f) {
			return 0;
		}
		else if (density < 0.99) {
			return 1;
		}
		else {
			return 2;
		}
	}

	SuperBlock* pop(size_t index)
	{
		SuperBlock* ans = nullptr;
		if (!listsOfBlocks[index].empty()) {
			ans = listsOfBlocks[index].back();
			listsOfBlocks[index].pop_back();
		}
		return ans;
	}

	void push(size_t index, SuperBlock* blockToPush)
	{
		listsOfBlocks[index].push_back(blockToPush);
	}

	bool pullOut(size_t index, SuperBlock* blockToFind)
	{
		for (auto it = listsOfBlocks[index].rbegin(); it != listsOfBlocks[index].rend(); it++) {
			if (*it == blockToFind) {
				std::iter_swap(it, listsOfBlocks[index].rbegin());
				return true;
			}
		}
		return false;
	}
};
