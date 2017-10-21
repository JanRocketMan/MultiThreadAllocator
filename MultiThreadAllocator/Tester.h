#pragma once
#include <iostream>
#include <random>
#include <time.h>
#include "mtallocator.cpp"

struct TestOptions {
	TestOptions(size_t t, size_t n, size_t i) : threadsCount(t),
		numIterations(n), items(i) {}
	size_t threadsCount;
	size_t numIterations;
	size_t items;
};

void runTest(const TestOptions& options, const std::vector<size_t>& itemsSizes)
{
	clock_t t = clock();

	float avgGlobalMalloc = 0, avgGlobalFree = 0;
	for (size_t i = 0; i < options.numIterations; i++) {
		std::vector<char*> items;
		items.reserve(options.items);
		clock_t t2 = clock();
		for (size_t j = 0; j < options.items; j++) {
			if (i == 1 && j == 172) {
				size_t k = 0;
			}
			//std::cout << i << j << std::endl;
			items.push_back((char*)mtalloc(itemsSizes[j]));
		}
		//std::cout << "got here";
		avgGlobalMalloc += ((float)clock() - t2) / CLOCKS_PER_SEC / options.numIterations;
		// std::cout << "Malloc time: " << ((float)clock() - t2) / CLOCKS_PER_SEC << std::endl;
		t2 = clock();
		for (size_t j = 0; j < items.size(); j++) {
			if (j == 3) {
				size_t k = 0;
			}
			//std::cout << j << std::endl;
			mtfree(items[j]);
		}
		avgGlobalFree += ((float)clock() - t2) / CLOCKS_PER_SEC / options.numIterations;
		// std::cout << "Free time: " << ((float)clock() - t2) / CLOCKS_PER_SEC << std::endl;
	}
	// std::cout << "Average init: " << avgInit << ", Average malloc: " << avgMalloc << ", Average free: " << avgFree << std::endl;
	std::cout << "Average global malloc: " << avgGlobalMalloc << ", Average global free: " << avgGlobalFree << std::endl;
	std::cout << "Total time: " << ((float)clock() - t) / CLOCKS_PER_SEC << std::endl;
}

void showResults(const TestOptions& options)
{
	std::cout << "Number of threads: " << options.threadsCount << std::endl;
	std::cout << "Number of iterations: " << options.numIterations << std::endl;
	std::cout << "Number of items: " << options.items << std::endl;
	std::cout << "Testing... " << std::endl;

	std::minstd_rand0 randGen;
	std::vector<size_t> itemsSize(options.items, 0);
	for (size_t j = 0; j < options.items; j++) {
		itemsSize[j] = (randGen() % 100) + 4;
	}

	for (size_t k = 0; k < 5; k++) {
		runTest(options, itemsSize);
	}
}
