#pragma once
#include "Bin.h"
#include <array>
#include <mutex>

const float f = 0.25;
const size_t K = 10;
const size_t Nbins = 10;

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
		void* ptr = nullptr;
		size_t currindex = index(bytes);

		std::unique_lock<std::mutex> lock(mtx);

		SuperBlock* newSbck = Bins[currindex].getFirstForMalloc();

		if (newSbck == nullptr) {
			if (emptyBlock != nullptr) {
				newSbck = popfromempty();
				newSbck->ChangeBlockSize(size(currindex));
			}
			else {
				newSbck = zeroHeap->SendBlock(currindex);
				if (newSbck != nullptr) {
					usedSize += newSbck->usedSize;
				}
				else {
					newSbck = new SuperBlock(size(currindex), this);
				}
				allocatedSize += SuperBlockSize;
			}
			Bins[currindex].push(newSbck, true);
			newSbck->owner = this;
		}

		usedSize += size(currindex);
		ptr = newSbck->SbMalloc(bytes);

		return ptr;
	}

	bool HeapFree(void* ptr, Heap* zeroHeap)
	{
		SuperBlock* block = GetOwnerOfBlock(ptr);
		std::unique_lock<std::mutex> lock(mtx);
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
			SuperBlock* to_send = SendBlock(currindex);
			assert(to_send != nullptr);
			zeroHeap->ReceiveBlock(to_send, currindex);
		}

		return true;
	}

	SuperBlock* SendBlock(size_t binIndex) {
		std::unique_lock<std::mutex> lock(mtx);
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
		}
		return ans;
	}

	void ReceiveBlock(SuperBlock* newblock, size_t binIndex) {
		std::unique_lock<std::mutex> lock(mtx);
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
		if (temp != nullptr) {
			temp->prev = nullptr;
		}
		while (temp != nullptr) {
			assert(temp != temp->next);
			temp = temp->next;
			if (temp != nullptr && temp->prev != nullptr) {
				delete temp->prev;
				temp->prev = nullptr;
			}
		}
	}

	void pushtoempty(SuperBlock* newblock) {
		if (emptyBlock != nullptr) {
			assert(emptyBlock != newblock);
			emptyBlock->prev = newblock;
		}
		newblock->next = emptyBlock;
		newblock->prev = nullptr;
		emptyBlock = newblock;
	}

	SuperBlock* popfromempty() {
		SuperBlock* ans = emptyBlock;
		if (ans != nullptr) {
			emptyBlock = ans->next;
			if (emptyBlock != nullptr) {
				emptyBlock->prev = nullptr;
			}
			ans->next = nullptr;
			ans->prev = nullptr;
		}
		return ans;
	}

	size_t index(int bytes)
	{
		assert(bytes > 8);
		return (size_t) ceil(log2(bytes) - 4);
	}

	size_t size(size_t index)
	{
		return (size_t) pow(2, index + 4);
	}

	std::array<Bin, Nbins> Bins;
	SuperBlock* emptyBlock;

	size_t usedSize;
	size_t allocatedSize;
	std::mutex mtx;
};
