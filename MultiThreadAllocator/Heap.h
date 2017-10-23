#pragma once
#include "Bin.h"
#include <numeric>
#include <mutex>

static const size_t K = 5;
static const size_t Nbins = 11;

static SuperBlock* GetOwnerOfBlock(void* ptr)
{
	return ((SuperBlock **)ptr)[-1];
}

class Heap {
public:
	Heap() : usedSize(0), allocatedSize(0)
	{
		std::iota(randomIndexes.begin(), randomIndexes.end(), 0);
		std::random_shuffle(randomIndexes.begin(), randomIndexes.end());
	}
	~Heap() = default;

	void* HeapMalloc(size_t bytes, Heap* zeroHeap)
	{
		std::unique_lock<std::recursive_mutex> lock(mtx);
		void* ptr = nullptr;
		size_t currindex = index(bytes);

		assert(this != zeroHeap);

		SuperBlock* newSbck = Bins[currindex].GetBlock();

		if (newSbck == nullptr) {
			newSbck = zeroHeap->SendBlock(currindex, this);
			if (newSbck != nullptr) {
				usedSize += newSbck->usedSize;
			}
			else {
				newSbck = new SuperBlock(size(currindex), this);
			}
			newSbck->owner = this;
			allocatedSize += SuperBlockSize;
		}

		Bins[currindex].AddBlock(newSbck, true);
		usedSize += size(currindex);
		ptr = newSbck->SbMalloc();

		return ptr;
	}

	bool HeapFree(void* ptr, Heap* zeroHeap)
	{
		std::unique_lock<std::recursive_mutex> lock(mtx);
		SuperBlock* block = GetOwnerOfBlock(ptr);
		if (block->owner != this) {
			return false;
		}

		size_t currindex = index(block->blockSize);
		usedSize -= block->blockSize;
		assert(usedSize >= 0);
		block->SbFree(ptr);

		Bins[currindex].UpdateBlockAfterFree(block);

		if (this == zeroHeap) {
			return true;
		}

		if ((usedSize + K * SuperBlockSize < allocatedSize) && (usedSize < (size_t)((float)(1 - f)*allocatedSize))) {
			size_t i = 0;
			SuperBlock* to_send = SendSomeBlock(zeroHeap, &i);
			assert(to_send != nullptr);
			zeroHeap->ReceiveBlock(to_send, i);
		}
		return true;
	}

	SuperBlock* SendBlock(size_t binIndex, Heap* where)
	{
		std::unique_lock<std::recursive_mutex> lock(mtx);
		SuperBlock* ans = Bins[binIndex].GetBlock();
		if (ans != nullptr) {
			usedSize -= ans->usedSize;
			allocatedSize -= SuperBlockSize;
			ans->owner = where;
		}
		return ans;
	}

	SuperBlock* SendSomeBlock(Heap* where, size_t* index)
	{
		SuperBlock* to_send = nullptr;
		for (size_t k = 0; k < Nbins; k++) {
			to_send = Bins[randomIndexes[k]].GetBlock(0);
			if (to_send != nullptr) {
				*index = randomIndexes[k];
				std::swap(randomIndexes[k], randomIndexes[Nbins - 1]);
				break;
			}
		}
		assert(to_send != nullptr);
		to_send->owner = where;
		usedSize -= to_send->usedSize;
		allocatedSize -= SuperBlockSize;
		return to_send;
	}

	void ReceiveBlock(SuperBlock* newblock, size_t binIndex) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		Bins[binIndex].AddBlock(newblock, false);

		usedSize += newblock->usedSize;
		allocatedSize += SuperBlockSize;
		newblock->owner = this;
	}

private:

	size_t index(size_t bytes)
	{
		assert(bytes > AdditionalSize);
		return (size_t)ceil(log2(bytes) - 3);
	}

	size_t size(size_t index)
	{
		assert(index < Nbins);
		return (size_t)pow(2, index + 3);
	}

	std::array<Bin, Nbins> Bins;

	size_t usedSize;
	size_t allocatedSize;
	std::array<size_t, Nbins> randomIndexes;
	std::recursive_mutex mtx;
};
