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