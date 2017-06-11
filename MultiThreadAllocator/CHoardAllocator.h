#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include "CMemoryTable.h"

extern size_t const CHUNKSIZE;
extern float const f;
extern size_t const k;

class CHoardAllocator {
public:
	CHoardAllocator() :
		n(2 * std::thread::hardware_concurrency() + 1)
	{
		heap.assign(n, CMemoryTable());
		std::vector<std::mutex> list(n);
		mtx.swap(list);
	}

	void* mtalloc(size_t bytes)
	{
		if (bytes > CHUNKSIZE / 2) {
			char* ptr = (char*)(malloc(bytes + sizeof(BlockInfo)));
			BlockInfo info(nullptr, 0);
			std::memcpy(ptr, &info, sizeof(BlockInfo));
			return (void*)(ptr + sizeof(BlockInfo));
		}
		else {
			size_t cHeapIndex = hash(std::this_thread::get_id());
			CMemoryTable& cHeap = heap[cHeapIndex];
			size_t lineIndex = indexOfLine(bytes);
			std::unique_lock<std::mutex> lock(mtx[cHeapIndex]);
			CMemoryTable::Line& cLine = cHeap.line[lineIndex];
			
			if (cLine.dense != nullptr) {
				return cHeap.mallocDense(lineIndex, bytes);
			}
			else if (cLine.thin != nullptr) {
				return cHeap.mallocThin(lineIndex, bytes);
			}
			else if (heap[0].line[lineIndex].thin != nullptr) {
				std::unique_lock<std::mutex> lock2(mtx[0]);
				CChunk* chToMove = heap[0].line[lineIndex].thin;
				chToMove->indexOfOwner = cHeapIndex;
				heap[0].popThin(lineIndex);
				cHeap.pushThin(lineIndex, chToMove);

				cHeap.usedSize += chToMove->usedSize;
				heap[0].usedSize -= chToMove->usedSize;
				cHeap.allocatedSize += CHUNKSIZE;
				heap[0].allocatedSize -= CHUNKSIZE;
				return cHeap.mallocThin(lineIndex, bytes);
			}
			else if (heap[0].line[lineIndex].dense != nullptr) {
				std::unique_lock<std::mutex> lock2(mtx[0]);
				CChunk* chToMove = heap[0].line[lineIndex].dense;
				chToMove->indexOfOwner = cHeapIndex;
				heap[0].popDense(lineIndex);
				cHeap.pushDense(lineIndex, chToMove);

				cHeap.usedSize += chToMove->usedSize;
				heap[0].usedSize -= chToMove->usedSize;
				cHeap.allocatedSize += CHUNKSIZE;
				heap[0].allocatedSize -= CHUNKSIZE;
				return cHeap.mallocDense(lineIndex, bytes);
			}
			else {
				CChunk* newChunk = new CChunk(size(lineIndex), cHeapIndex);
				cHeap.pushThin(lineIndex, newChunk);
				cHeap.allocatedSize += CHUNKSIZE;
				return cHeap.mallocThin(lineIndex, bytes);
			}
		}
	}

	void mtfree(void* ptr)
	{
		if (ptr != nullptr) {
			CChunk* owner = GetOwner(ptr);
			if (owner != nullptr) {
				bool success = false;
				while (!success) {
					size_t cHeapIndex = owner->indexOfOwner;
					std::unique_lock<std::mutex> lock(mtx[cHeapIndex]);
					if (owner->indexOfOwner != cHeapIndex) {
						continue;
					}

					size_t lineIndex = indexOfLine(owner->blockSize);
					CMemoryTable& cHeap = heap[cHeapIndex];
					if (owner->IsFull()) {
						cHeap.freeFull(lineIndex, owner, ptr);
					}
					else if (owner->IsDense()) {
						cHeap.freeDense(lineIndex, owner, ptr);
					}
					else {
						cHeap.freeThin(lineIndex, owner, ptr);
					}

					if (cHeap.IsThin()) {
						
					}
					success = true;
				}
			}
			else {
				free((char*)ptr - sizeof(BlockInfo));
			}
		}
	}

private:
	size_t indexOfLine(size_t size)
	{
		return (size > 4) ? (ceil(log(size) / log(2)) - 2) : 0;
	}

	size_t size(size_t indexOfLine)
	{
		return floor(pow(2, indexOfLine + 2));
	}

	inline size_t hash(std::thread::id id)
	{
		return hasher(id) % (n - 1) + 1;
	}

	size_t const n;

	std::vector<CMemoryTable> heap;
	std::vector<std::mutex> mtx;
	std::hash<std::thread::id> hasher;
};

CHoardAllocator h;

extern void* mtalloc(size_t bytes) {
	return h.mtalloc(bytes);
}

extern void mtfree(void* ptr) {
	h.mtfree(ptr);
}
