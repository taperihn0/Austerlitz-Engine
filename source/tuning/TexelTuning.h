#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#define COLLECT_POSITION_DATA true
#define ENABLE_TUNING false

class tGameCollector {
public:
	enum GameResult {
		WHITE_LOSE, DRAW, WHITE_WIN, NO_RESULT
	};

	tGameCollector()
	: game_res(static_cast<double>(NO_RESULT)) {}

	bool openFile();

	inline void registerGameResult(GameResult res) noexcept {
		game_res = static_cast<double>(res);
	}

	inline void registerPosition(const std::string& fen) {
		pos_bufor.push_back(fen);
	}

	void flushBufor();

private:

	inline void clearBufor() noexcept {
		pos_bufor.clear();
		game_res = static_cast<double>(NO_RESULT);
	}
	
	void storeGameResult();

	std::vector<std::string> pos_bufor;
	double game_res;
	std::ofstream file;
};

#if COLLECT_POSITION_DATA
extern tGameCollector game_collector;
#endif

struct tData {
	tData();
	bool openDataFile();

	bool loadTrainingData();
	void fillEvalSet();

	static constexpr const char* getFilePath() noexcept {
		return filepath.data();
	}

	std::vector<std::string> position;
	std::vector<double> gres;
	std::vector<int> eval;

	size_t position_n;
private:
	static constexpr std::string_view filepath = 
		R"(C:/Users/User/source/repos/ChessEngine/source/data/TuningData.txt)";
	std::ifstream file;
};

class tTuning {
public:
	tTuning();

	void initTuningData();

	double computeK(const int k_precision = 10);
private:
	double computeE(const double k);
	double sigmoid(const double k, const double q);

	tData data;
};

#if ENABLE_TUNING
extern tTuning tuning;
#endif