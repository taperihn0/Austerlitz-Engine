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
	static constexpr std::string_view benchmark_script_path = R"(source/data/BenchmarkScript.txt)";
};

// execute commands located in .txt file and measure time
inline void SearchBenchmark::start() {
	static Timer timer;
	
	std::ifstream source;
	source.open(benchmark_script_path.data());

	if (!source.is_open()) {
		OS << "Failed to open bench file: " << benchmark_script_path << '\n';
		return;
	}

	uci.setInputStream(&source);
	timer.go();

	uci.goLoop();

	timer.stop();
	source.close();
	
	uci.setInputStream(&std::cin);

	OS << "Total time: " << timer.duration() << " ms\n";
}

extern SearchBenchmark bench;
