#pragma once

#include <sstream>


class UCI {
public:
	UCI();
	void parsePosition(std::istringstream&);
	void goLoop(int argc = 1, char* argv[] = nullptr);
	std::istream* is;
	std::ostream* os;
};

extern UCI UCI_o;

// useful and pretty macros
#define OS *UCI_o.os
#define IS *UCI_o.is
#define OS_DST UCI_o.os
#define IS_DST UCI_o.is
