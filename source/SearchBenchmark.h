#pragma once

#include "Timer.h"
#include "SearchBenchmark.h"
#include "UCI.h"
#include <iostream>
#include <string>
#include <fstream>


class SearchBenchmark {
public:
	SearchBenchmark();
	void start();
private:
	std::ifstream src;
};


inline SearchBenchmark::SearchBenchmark()
	: src() {}


inline void SearchBenchmark::start() {
	static Timer timer;

	src.open("source\\BenchmarkScript.txt");

	UCI_o.is = &src;
	timer.go();

	UCI_o.goLoop();

	timer.stop();
	src.close();
	UCI_o.is = &std::cin;
	*UCI_o.os << "Total time: " << timer.duration() << " ms\n";
}


extern SearchBenchmark bench;
