#pragma once
#include "SuperBlock.h"
#include "Heap.h"
#include <cassert>
#include <cstring>
#include <thread>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

void* ReserveMemoryForBigBlock(size_t bytes)
{
	char* ptr = (char*)(malloc(bytes + sizeof(SuperBlock::StoreInfo)));
	SuperBlock::StoreInfo info(nullptr, 0);
	memcpy(ptr, &info, sizeof(SuperBlock::StoreInfo));
	return (void*)(ptr + sizeof(SuperBlock::StoreInfo));
}

void FreeMemoryForBigBlock(void* ptr)
{
	free((char*)ptr - sizeof(SuperBlock::StoreInfo));
}

class HoardAllocator {
public:
	HoardAllocator() :
		n(2 * std::thread::hardware_concurrency() + 1)
	{
		heaps.reserve(n);
		for (size_t i = 0; i < n; i++) {
			std::shared_ptr<Heap> ptr = std::make_shared<Heap>();
			heaps.push_back(ptr);
		}
	}

	void* mtalloc(size_t bytes)
	{
		bytes += sizeof(SuperBlock::StoreInfo);
		if (bytes > SuperBlockSize / 2) {
			return ReserveMemoryForBigBlock(bytes);
		}
		else {
			return heaps[hash(std::this_thread::get_id())]->HeapMalloc(bytes, heaps[0]);
		}
	}

	void mtfree(void* ptr)
	{
		if (ptr != nullptr) {
			SuperBlock* owner = GetOwnerOfBlock(ptr);
			if (owner != nullptr) {
				while (!owner->owner->HeapFree(ptr, heaps[0])) {

				}
			}
			else {
				FreeMemoryForBigBlock(ptr);
			}
		}
	}

	~HoardAllocator() = default;
private:
	inline size_t hash(std::thread::id id)
	{
		return hasher(id) % (n - 1) + 1;
	}

	std::vector<std::shared_ptr<Heap>> heaps;

	size_t const n;

	std::hash<std::thread::id> hasher;
};

HoardAllocator hoardAllocator;

extern void* mtalloc(size_t bytes) {
	return hoardAllocator.mtalloc(bytes);
}

extern void mtfree(void* ptr) {
	hoardAllocator.mtfree(ptr);
}
