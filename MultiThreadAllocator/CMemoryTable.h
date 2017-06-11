#pragma once
#include <algorithm>
#include <vector>
#include "CChunk.h"

extern size_t const CHUNKSIZE;
extern float const f;
static size_t const k = 20;

CChunk* GetOwner(void* ptr)
{
	return reinterpret_cast<BlockInfo*>((char*)ptr - sizeof(BlockInfo))->owner;
}

class CMemoryTable {
public:
	CMemoryTable() : usedSize(0), allocatedSize(0)
	{
		line.assign(20, Line());
	}
	~CMemoryTable()
	{
		for (auto it = line.begin(); it != line.end(); it++) {
			if (it->thin != nullptr) {
				delete it->thin;
			}
			if (it->dense != nullptr) {
				delete it->dense;
			}
			if (it->full != nullptr) {
				delete it->full;
			}
		}
	}

	void cutNode(CChunk* node)
	{
		if (node->prev != nullptr) {
			node->prev->next = node->next;
		}
		if (node->next != nullptr) {
			node->next->prev = node->prev;
		}
	}

	void* mallocThin(size_t lineIndex, size_t bytes)
	{
		Line& cLine = line[lineIndex];
		usedSize += cLine.thin->blockSize;
		void* ptr = cLine.thin->chMalloc(bytes);
		if (cLine.thin->IsDense()) {
			CChunk* chToMove = cLine.thin;
			popThin(lineIndex);
			pushDense(lineIndex, chToMove);
		}
		return ptr;
	}

	void CheckOwner(CChunk* owner)
	{
		if (owner->prev != nullptr) {
			assert(owner->prev->next == owner);
		}
		if (owner->next != nullptr) {
			assert(owner->next->prev == owner);
		}
	}

	void freeThin(size_t lineIndex, CChunk* owner, void* ptr)
	{
		CheckOwner(owner);
		Line& cLine = line[lineIndex];
		usedSize -= cLine.thin->blockSize;
		owner->chFree(ptr);
		
		cutNode(owner);
		cLine.thin->prev = owner;
		owner->next = cLine.thin;
		cLine.thin = owner;
	}

	void* mallocDense(size_t lineIndex, size_t bytes)
	{
		Line& cLine = line[lineIndex];
		usedSize += cLine.dense->blockSize;
		void* ptr = cLine.dense->chMalloc(bytes);
		if (cLine.dense->IsFull()) {
			CChunk* chToMove = cLine.dense;
			popDense(lineIndex);
			pushFull(lineIndex, chToMove);
		}
		return ptr;
	}

	void freeDense(size_t lineIndex, CChunk* owner, void* ptr)
	{
		CheckOwner(owner);
		Line& cLine = line[lineIndex];
		usedSize -= cLine.dense->blockSize;
		owner->chFree(ptr);

		cutNode(owner);
		cLine.dense->prev = owner;
		owner->next = cLine.dense;
		cLine.dense = owner;

		if (owner->IsThin()) {
			cutNode(owner);
			pushThin(lineIndex, owner);
		}
	}

	void freeFull(size_t lineIndex, CChunk* owner, void* ptr)
	{
		CheckOwner(owner);
		Line& cLine = line[lineIndex];
		usedSize -= cLine.full->blockSize;
		owner->chFree(ptr);
		cutNode(owner);
		pushDense(lineIndex, owner);
	}

	void pushThin(size_t lineIndex, CChunk* chunk)
	{
		Line& cLine = line[lineIndex];
		if (cLine.thin != nullptr) {
			cLine.thin->prev = chunk;
		}
		chunk->next = cLine.thin;
		cLine.thin = chunk;
	}

	void popThin(size_t lineIndex)
	{
		Line& cLine = line[lineIndex];
		cLine.thin = cLine.thin->next;
		if (cLine.thin != nullptr) {
			cLine.thin->prev = nullptr;
		}
	}

	void pushDense(size_t lineIndex, CChunk* chunk)
	{
		Line& cLine = line[lineIndex];
		if (cLine.dense != nullptr) {
			cLine.dense->prev = chunk;
		}
		chunk->next = cLine.dense;
		cLine.dense = chunk;
	}

	void popDense(size_t lineIndex)
	{
		Line& cLine = line[lineIndex];
		cLine.dense = cLine.dense->next;
		if (cLine.dense != nullptr) {
			cLine.dense->prev = nullptr;
		}
	}

	void pushFull(size_t lineIndex, CChunk* chunk)
	{
		Line& cLine = line[lineIndex];
		if (cLine.full != nullptr) {
			cLine.full->prev = chunk;
		}
		chunk->next = cLine.full;
		cLine.full = chunk;
	}

	void popFull(size_t lineIndex)
	{
		Line& cLine = line[lineIndex];
		cLine.full = cLine.full->next;
		if (cLine.full != nullptr) {
			cLine.full->prev = nullptr;
		}
	}

	bool IsThin()
	{
		return (usedSize < allocatedSize + CHUNKSIZE * k) && (usedSize < (1 - f) * allocatedSize);
	}

	struct Line {
		CChunk* thin;
		CChunk* dense;
		CChunk* full;
	};

	size_t usedSize;
	size_t allocatedSize;
	std::vector<Line> line;
};