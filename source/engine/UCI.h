#pragma once

#include <sstream>


class UCInterface {
public:
	UCInterface();
	void parsePosition(std::istringstream&);
	void goLoop(int argc = 1, const char* argv[] = nullptr);
	void setInputStream(std::istream* new_stream);

	// current input/output stream pointers. Set to (&cout) and (&cin) by default.
	std::istream* i_stream;
	std::ostream* o_stream;

	static constexpr std::string_view 
		engine_name = "id name Austerlitz v1.4.8",
		author = "id author Szymon Belz",
		introduction = "Polish chess engine, Austerlitz v1.4.8 - by Szymon Belz\n";
private:
	void introduce();
	void parseGo(std::istringstream& strm);
	void newGame();
	void setOption(std::istringstream& strm);

#if defined(__DEBUG__)
	void seePrint(std::istringstream& strm);
	void evalInfo();
#endif

#if ENABLE_TUNING
	void parseTuning(std::istringstream& strm);
#endif
};

extern UCInterface uci;

// macros of current input/output streams. Set as cout and cin by default.
#define IS *uci.i_stream
#define OS *uci.o_stream
