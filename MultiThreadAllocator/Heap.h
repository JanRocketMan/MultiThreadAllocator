#pragma once
#include "Bin.h"
#include <array>
#include <mutex>

const float f = 0.5;
const size_t K = 5;
const size_t Nbins = 11;

SuperBlock* GetOwnerOfBlock(void* ptr)
{
	SuperBlock::StoreInfo info;
	memcpy(&info, (char*)ptr - sizeof(SuperBlock::StoreInfo), sizeof(SuperBlock::StoreInfo));
	return info.owner;
}

class Heap {
public:
	Heap() : usedSize(0), allocatedSize(0), emptyBlock(nullptr) {}
	~Heap() {
		clearEmpty();
	}

	void* HeapMalloc(size_t bytes, Heap* zeroHeap)
	{
		std::unique_lock<std::recursive_mutex> lock(mtx);
		void* ptr = nullptr;
		size_t currindex = index(bytes);

		assert(this != zeroHeap);

		SuperBlock* newSbck = Bins[currindex].getFirstForMalloc();

		if (newSbck == nullptr) {
			if (emptyBlock != nullptr) {
				newSbck = popfromempty();
			}
			else {
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
			newSbck->ChangeBlockSize(size(currindex));
			Bins[currindex].push(newSbck, true);
		}

		usedSize += size(currindex);
		ptr = newSbck->SbMalloc(bytes);

		return ptr;
	}

	bool HeapFree(void* ptr, Heap* zeroHeap)
	{
		SuperBlock* block = GetOwnerOfBlock(ptr);
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if (block->owner != this) {
			return false;
		}
		size_t currindex = index(block->blockSize);
		usedSize -= block->blockSize;
		assert(usedSize >= 0);
		block->SbFree(ptr);
		SuperBlock* res = Bins[currindex].update(block);
		if (res != nullptr) {
			pushtoempty(res);
		}

		if ((usedSize + K * SuperBlockSize < allocatedSize) && (usedSize < (size_t)((float)(1 - f)*allocatedSize))) {
			SuperBlock* to_send = SendBlock(currindex, zeroHeap);
			assert(to_send != nullptr);
			zeroHeap->ReceiveBlock(to_send, currindex);
		}

		return true;
	}

	SuperBlock* SendBlock(size_t binIndex, Heap* where) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		SuperBlock* ans = nullptr;
		ans = popfromempty();
		if (ans == nullptr) {
			ans = Bins[binIndex].pop(2);
			if (ans == nullptr) {
				ans = Bins[binIndex].pop(1);
				if (ans == nullptr) {
					ans = Bins[binIndex].pop(0);
				}
			}
		}
		if (ans != nullptr) {
			usedSize -= ans->usedSize;
			allocatedSize -= SuperBlockSize;
			ans->owner = where;
		}
		return ans;
	}

	void ReceiveBlock(SuperBlock* newblock, size_t binIndex) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		if (newblock->usedSize != 0) {
			Bins[binIndex].push(newblock, false);
		}
		else {
			pushtoempty(newblock);
		}

		usedSize += newblock->usedSize;
		allocatedSize += SuperBlockSize;
		newblock->owner = this;
	}

private:

	void clearEmpty() {
		SuperBlock* temp = emptyBlock;
		assert(temp == nullptr || temp->prev == nullptr);
		SuperBlock* temp2 = nullptr;
		while (temp != nullptr) {
			temp2 = temp->next;
			assert(temp2 != temp);
			delete temp;
			temp = temp2;
		}
	}

	void pushtoempty(SuperBlock* newblock) {
		if (emptyBlock != nullptr) {
			assert(emptyBlock->prev == nullptr);
			emptyBlock->prev = newblock;
		}
		assert(newblock->prev == nullptr && newblock->next == nullptr);
		newblock->next = emptyBlock;
		emptyBlock = newblock;
	}

	SuperBlock* popfromempty() {
		SuperBlock* ans = emptyBlock;
		if (ans != nullptr) {
			emptyBlock = ans->next;
			ans->next = nullptr;
			ans->prev = nullptr;
			if (emptyBlock != nullptr) {
				emptyBlock->prev = nullptr;
			}
		}
		return ans;
	}

	size_t index(int bytes)
	{
		assert(bytes > sizeof(SuperBlock::StoreInfo));
		return (size_t) ceil(log2(bytes) - 3);
	}

	size_t size(size_t index)
	{
		assert(index < Nbins);
		return (size_t) pow(2, index + 3);
	}

	std::array<Bin, Nbins> Bins;
	SuperBlock* emptyBlock;

	size_t usedSize;
	size_t allocatedSize;
	std::recursive_mutex mtx;
};
