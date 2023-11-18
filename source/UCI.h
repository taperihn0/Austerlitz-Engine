#pragma once

#include <sstream>


class UCI {
public:
	UCI();
	void parsePosition(std::istringstream&);
	void goLoop(int argc = 1, char* argv[] = nullptr);
	std::istream* i_stream;
	std::ostream* o_stream;

	static constexpr const char* engine_name = "id name Austerlitz@",
		*author = "id author Simon B.";
};

// useful macros for stream pointer manipulating
#define OS *UCI_o.o_stream
#define IS *UCI_o.i_stream
#define OS_PTR UCI_o.o_stream
#define IS_PTR UCI_o.i_stream

extern UCI UCI_o;