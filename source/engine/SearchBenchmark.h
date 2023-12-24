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
	static constexpr std::string_view bench_path = R"(source/data/BenchmarkScript.txt)";
};

// execute commands located in .txt file and measure time
inline void SearchBenchmark::start() {
	static Timer timer;

	src.open(bench_path.data());

	if (!src.is_open()) {
		OS << "Failed to open bench file: " << bench_path << '\n';
		return;
	}

	// change current input stream to given file
	IS_PTR = &src;
	timer.go();

	UCI_o.goLoop();

	timer.stop();
	src.close();
	IS_PTR = &std::cin;
	OS << "Total time: " << timer.duration() << " ms\n";
}


extern SearchBenchmark bench;
