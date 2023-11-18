#pragma once

#include "Timer.h"
#include "SearchBenchmark.h"
#include "UCI.h"
#include <iostream>
#include <string>
#include <fstream>


// simple benchmark driver
class SearchBenchmark {
public:
	SearchBenchmark() = default;
	inline void start();
private:
	std::ifstream src;
};

// execute commands located in .txt file and measure time
inline void SearchBenchmark::start() {
	static Timer timer;

	src.open("source\\BenchmarkScript.txt");

	IS_PTR = &src;
	timer.go();

	UCI_o.goLoop();

	timer.stop();
	src.close();
	IS_PTR = &std::cin;
	OS << "Total time: " << timer.duration() << " ms\n";
}


extern SearchBenchmark bench;
