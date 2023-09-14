#pragma once

#include <sstream>


#define TOGUI_S std::cout
#define FROMGUI_S std::cin


namespace UCI {
	
	void parsePosition(std::istringstream&);

	void goLoop(int argc, char* argv[]);

}