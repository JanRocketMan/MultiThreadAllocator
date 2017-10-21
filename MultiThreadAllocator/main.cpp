#pragma once
#include "Tester.h"
#include "SuperBlock.h"

int main() {
	size_t threadsCount = 1;
	size_t numIterations = 100;
	size_t items = 100;
	TestOptions t(threadsCount, numIterations, items);
	showResults(t);
	//std::cout << sizeof(SuperBlock::StoreInfo) << std::endl;
	//std::cout << sizeof(char) << std::endl;
	std::cin >> items;
	system("pause");
}
