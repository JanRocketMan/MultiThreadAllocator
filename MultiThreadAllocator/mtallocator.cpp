#pragma once
#include "SuperBlock.h"
#include "Heap.h"
#include <cassert>
#include <cmath>
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

SuperBlock* GetOwnerOfBlock(void* ptr)
{
	SuperBlock::StoreInfo info;
	memcpy(&info, (char*)ptr - sizeof(SuperBlock::StoreInfo), sizeof(SuperBlock::StoreInfo));
	return info.owner;
}

class HoardAllocator {
public:
	HoardAllocator() :
		sbSize(8192), f(0.5), k(6), n(2 * std::thread::hardware_concurrency() + 1)
	{
		heaps.reserve(n);
		for (size_t i = 0; i < n; i++) {
			std::shared_ptr<Heap> ptr = std::make_shared<Heap>(sbSize, f, k);
			heaps.push_back(ptr);
		}
	}

	HoardAllocator(size_t sbSize, double f, size_t k) :
		sbSize(sbSize), f(f), k(k), n(2 * std::thread::hardware_concurrency() + 1)
	{
		heaps.reserve(n);
		for (size_t i = 0; i < n; i++) {
			std::shared_ptr<Heap> ptr = std::make_shared<Heap>(sbSize, f, k);
			heaps.push_back(ptr);
		}
	}

	void* mtalloc(size_t bytes)
	{
		if (bytes > sbSize / 2) {
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

	size_t const sbSize;
	double const f;
	size_t const k; // Minimum number of heaps in local heap.
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
