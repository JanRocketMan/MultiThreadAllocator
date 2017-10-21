#pragma once
#include "SuperBlock.h"
#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <vector>

SuperBlock* getOwner(void* ptr)
{
	return reinterpret_cast<SuperBlock::StoreInfo*>((char*)ptr - sizeof(SuperBlock::StoreInfo))->owner;
}

enum type { full, ffull, fempty };

struct GetResult {
	GetResult(SuperBlock* bl, type factor) : block(bl), fillFactor(factor) {}
	SuperBlock* block;
	type fillFactor;
};

class TableLine {
public:
	TableLine() = delete;
	TableLine(float f, size_t blSize, size_t sbSize) :
		f(f), blockSize(blSize), superBlockSize(sbSize) {}
	~TableLine()
	{
		for (auto it = fFull.begin(); it != fFull.end(); it++) {
			it->Clear();
		}
		for (auto it = Full.begin(); it != Full.end(); it++) {
			it->Clear();
		}
		for (auto it = fEmpty.begin(); it != fEmpty.end(); it++) {
			it->Clear();
		}
	}

	GetResult GetFirstEmpty()
	{
		if (!fFull.empty()) {
			return GetResult(&fFull.front(), ffull);
		}
		else if (!fEmpty.empty()) {
			return GetResult(&fEmpty.front(), fempty);
		}
		else {
			return GetResult(nullptr, fempty);
		}
	}

	void UpdateSuperBlock(GetResult& res)
	{
		SuperBlock block = *res.block;
		if (res.fillFactor == ffull) {
			if (block.usedSize + blockSize > superBlockSize) {
				fFull.pop_front();
				Full.push_front(block);
			}
			else if (block.usedSize <= (float)(1 - f) * superBlockSize) {
				fFull.pop_front();
				fEmpty.push_front(block);
			}
		}
		else if (res.fillFactor == fempty) {
			if (block.usedSize > (float)(1 - f) * superBlockSize) {
				fEmpty.pop_front();
				fFull.push_front(block);
			}
		}
		else if (res.fillFactor == full) {
			Full.pop_front();
			fFull.push_front(block);
		}
	}

	void FindSuperBlockAndUpdate(SuperBlock* blockToFind)
	{
		GetResult res(blockToFind, fempty);
		if (blockToFind->usedSize + 2 * blockSize > superBlockSize) {
			res.fillFactor = full;
		}
		else if (blockToFind->usedSize + blockSize > (float)(1 - f) * superBlockSize) {
			res.fillFactor = ffull;
		}
		if (res.fillFactor == full) {
			std::swap(*Full.begin(), *blockToFind);
		}
		else if (res.fillFactor == ffull) {
			std::swap(*fFull.begin(), *blockToFind);
		}
		else if (res.fillFactor == fempty) {
			std::swap(*fEmpty.begin(), *blockToFind);
		}
		UpdateSuperBlock(res);
	}

	void AddSuperBlock(SuperBlock blToAdd, type type)
	{
		if (type == fempty) {
			fEmpty.push_front(blToAdd);
		}
		else if (type == ffull) {
			fFull.push_front(blToAdd);
		}
	}

	void PopSuperBlock(type type)
	{
		if (type == fempty) {
			fEmpty.pop_front();
		}
		else if (type == ffull) {
			fFull.pop_front();
		}
	}

	SuperBlock PopfEmpty()
	{
		SuperBlock ans = fEmpty.front();
		fEmpty.pop_front();
		return ans;
	}

	size_t GetFEmptySize()
	{
		return fEmpty.size();
	}

	float const f;
	size_t const blockSize;
	size_t const superBlockSize;
private:
	std::list<SuperBlock> Full;
	std::list<SuperBlock> fFull; // usedSize > (1 - f) * superBlockSize
	std::list<SuperBlock> fEmpty; // usedSize <= (1 - f) * superBlockSize
};

class Heap {
public:
	Heap() = delete;
	Heap(size_t const superBlockSize, double const f, size_t const k) :
		usedSize(0), allocatedSize(0), superBlockSize(superBlockSize), f(f),
		k(k)
	{
		for (size_t i = 0; i < 20; i++) {
			table.emplace_back(f, size(i), superBlockSize);
		}
	}
	~Heap() = default;

	void* HeapMalloc(size_t bytes, std::shared_ptr<Heap> zeroHeap)
	{
		void* ptr = nullptr;
		size_t lineIndex = hash(bytes);

		std::unique_lock<std::mutex> lock(mtx);

		GetResult res = table[lineIndex].GetFirstEmpty();

		if (res.block != nullptr) {
			usedSize += table[lineIndex].blockSize;
			ptr = res.block->SbMalloc(bytes);
			table[lineIndex].UpdateSuperBlock(res);
		}
		else {
			std::unique_lock<std::mutex> lock2(zeroHeap->mtx);
			res = zeroHeap->table[lineIndex].GetFirstEmpty();
			if (res.block != nullptr) {
				zeroHeap->usedSize -= res.block->usedSize;
				zeroHeap->allocatedSize -= superBlockSize;
				SuperBlock bl = *res.block;
				zeroHeap->table[lineIndex].PopSuperBlock(res.fillFactor);
				res.block->owner = this;

				lock2.unlock();

				usedSize += res.block->usedSize + table[lineIndex].blockSize;
				allocatedSize += superBlockSize;

				ptr = res.block->SbMalloc(bytes);
				table[lineIndex].AddSuperBlock(bl, res.fillFactor);
			}
			else {
				lock2.unlock();
				SuperBlock newBlock(size(lineIndex), this);
				allocatedSize += superBlockSize;
				usedSize += newBlock.blockSize;
				ptr = newBlock.SbMalloc(bytes);
				table[lineIndex].AddSuperBlock(newBlock, fempty);
			}
		}

		return ptr;
	}

	bool HeapFree(void* ptr, std::shared_ptr<Heap> zeroHeap)
	{
		SuperBlock* block = getOwner(ptr);
		std::unique_lock<std::mutex> lock(mtx);
		if (block->owner != this) {
			return false;
		}
		size_t lineIndex = hash(block->blockSize);
		usedSize -= block->blockSize;
		block->SbFree(ptr);
		table[lineIndex].FindSuperBlockAndUpdate(block);

		if (this != zeroHeap.get() && (usedSize + (int)(k * superBlockSize) < allocatedSize) &&
			(usedSize < (int)((double)(1 - f) * allocatedSize))) {
			DropToZeroHeap(zeroHeap);
		}

		assert(usedSize >= 0);
		return true;
	}

private:
	void DropToZeroHeap(std::shared_ptr<Heap> zeroHeap)
	{
		for (size_t i = 0; i < 20; i++) {
			if (usedSize >= (int)((double)(1 - f) * allocatedSize)
				|| usedSize + (int)(k * superBlockSize) < allocatedSize) {
				break;
			}

			size_t k = table[i].GetFEmptySize();

			for (size_t j = 0; j < k; j++) {

				if (usedSize >= (int)((double)(1 - f) * allocatedSize)
					|| usedSize + (int)(k * superBlockSize) < allocatedSize) {
					break;
				}

				SuperBlock block = table[i].PopfEmpty();

				usedSize -= block.usedSize;
				allocatedSize -= superBlockSize;

				std::unique_lock<std::mutex> lock2(zeroHeap->mtx);
				block.owner = zeroHeap.get();
				zeroHeap->table[i].AddSuperBlock(block, fempty);
				zeroHeap->usedSize += block.usedSize;
				zeroHeap->allocatedSize += superBlockSize;
			}
		}
	}

	size_t hash(int size)
	{
		size_t ans = 0;
		if (size > floor(pow(1.5, 6))) {
			ans = ceil(log(size) / log(1.5)) - 6;
		}
		return ans;
	}

	size_t size(size_t index)
	{
		return floor(pow(1.5, index + 6));
	}

	std::vector<TableLine> table;

	int usedSize;
	int allocatedSize;
	size_t const superBlockSize;
	float const f;
	size_t const k;
	std::mutex mtx;
};
