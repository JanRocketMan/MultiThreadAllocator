#pragma once
#include "Tester.h"

int main() {
	size_t threadsCount = 1;
	size_t numIterations = 100;
	size_t items = 100000;
	TestOptions t(threadsCount, numIterations, items);
	showResults(t);
	system("pause");
}
