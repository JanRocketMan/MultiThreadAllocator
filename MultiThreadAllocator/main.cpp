#pragma once
#include "Tester.h"
#include "SuperBlock.h"

int main() {
	size_t threadsCount = 2;
	size_t numIterations = 10000;
	size_t items = 10;
	TestOptions t(threadsCount, numIterations, items);
	showResults(t);
	//std::cout << sizeof(SuperBlock::StoreInfo) << std::endl;
	//std::cout << sizeof(char) << std::endl;
	//std::cin >> items;
	//system("pause");
	return 0;
}
