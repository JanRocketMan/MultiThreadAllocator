#pragma once
#include <ctime>
#include "Tester.h"
#include "SuperBlock.h"

int main() {
	clock_t time = clock();
	size_t threadsCount = 1;
	size_t numIterations = 10;
	size_t items = 10000;
	//unsigned rand_state = 60;
	TestOptions t(threadsCount, numIterations, items);
	showResults(t);
	std::cout << "Work Time:" << ((float)clock() - time) / CLOCKS_PER_SEC << std::endl;
	time = clock();
	delete hoardAllocator;
	std::cout << "Delete Time:" << ((float)clock() - time) / CLOCKS_PER_SEC << std::endl;

	//std::cout << sizeof(SuperBlock::StoreInfo) << std::endl;
	//std::cout << sizeof(char) << std::endl;
	std::cin >> items;
	//system("pause");
	return 0;
}
