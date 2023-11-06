#pragma once

#include <sstream>

// useful macros for stream pointer manipulating
#define OS *UCI_o.os
#define IS *UCI_o.is
#define OS_DST UCI_o.os
#define IS_DST UCI_o.is


class UCI {
public:
	UCI();
	void parsePosition(std::istringstream&);
	void goLoop(int argc = 1, char* argv[] = nullptr);
	std::istream* is;
	std::ostream* os;

	static constexpr const char* engine_name = "id name Austerlitz@",
		* author = "id author Simon B.";
};

extern UCI UCI_o;