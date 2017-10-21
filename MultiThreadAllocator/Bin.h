#pragma once
#include <array>
#include "SuperBlock.h"

class Bin {
public:
	std::array<SuperBlock*, 4> densityType; // 0 - f >= 2/3; 1 - f >= 1/3; 2 - f < 1/3; 3 - f >= 3/3
											// f = usedSize / SuperBlockSize
	Bin() {
		for (size_t i = 0; i < 4; i++) {
			densityType[i] = nullptr;
		}
	}
	~Bin() {
		for (size_t i = 0; i < 4; i++) {
			SuperBlock* temp = densityType[i];
			while (temp != nullptr) {
				assert(temp != temp->next);
				temp = temp->next;
				if (temp != nullptr && temp->prev != nullptr) {
					delete temp->prev;
				}
			}
		}
	}
	SuperBlock* getFirstForMalloc() {
		SuperBlock* ans = nullptr;
		for (size_t i = 0; i < 3; i++) {
			ans = pop(i);
			if (ans != nullptr) {
				push(ans, true);
				break;
			}
		}
		return ans;
	}

	SuperBlock* pop(size_t index) {
		SuperBlock* ans = densityType[index];
		if (ans != nullptr) {
			densityType[index] = ans->next;
			ans->prev = nullptr;
			ans->next = nullptr;
		}
		if (densityType[index] != nullptr) {
			densityType[index]->prev = nullptr;
		}
		return ans;
	}

	void push(SuperBlock* newblock, bool nextIsMalloc) {
		size_t j = getIndexForSbck(newblock, nextIsMalloc);
		if (densityType[j] != nullptr) {
			densityType[j]->prev = newblock;
		}
		newblock->prev = nullptr;
		newblock->next = densityType[j];
		densityType[j] = newblock;
	}

	SuperBlock* update(SuperBlock* toUpdate) {
		bool canFind = findAndPop(3, toUpdate);
		if (!canFind) {
			canFind = findAndPop(0, toUpdate);
		}
		if (!canFind) {
			canFind = findAndPop(1, toUpdate);
		}
		if (!canFind) {
			canFind = findAndPop(2, toUpdate);
		}
		assert(canFind);
		if (toUpdate->usedSize < toUpdate->blockSize) {
			toUpdate->next = nullptr;
			toUpdate->prev = nullptr;
			return toUpdate;
		}
		else {
			push(toUpdate, false);
			return nullptr;
		}
	}

private:
	size_t getIndexForSbck(SuperBlock* sbck, bool nextIsMalloc) {
		float f = 0;
		if (nextIsMalloc) {
			f = (float)(sbck->usedSize + sbck->blockSize) / (float)SuperBlockSize;
		}
		else {
			f = (float)(sbck->usedSize) / (float)SuperBlockSize;
		}

		if (f > 0.99) {
			return 3;
		}
		else if (f > 0.66) {
			return 0;
		}
		else if (f > 0.33) {
			return 1;
		}
		else return 2;
	}

	bool findAndPop(size_t index, SuperBlock* toFind) {
		SuperBlock* ans = densityType[index];
		while (ans != nullptr && ans != toFind) {
			ans = ans->next;
		}
		if (ans == nullptr) {
			return false;
		}
		if (ans->prev != nullptr) {
			(ans->prev)->next = ans->next;
		}
		if (ans->next != nullptr) {
			(ans->next)->prev = ans->prev;
		}
		ans->prev = nullptr;
		ans->next = nullptr;
		return true;
	}
};
