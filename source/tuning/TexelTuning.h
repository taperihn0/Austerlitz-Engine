#pragma once

#include "../engine/Evaluation.h"
#include "../engine/Timer.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <utility>

#define COLLECT_POSITION_DATA true
#define ENABLE_TUNING false

class tGameCollector {
public:
	enum GameResult {
		WHITE_LOSE, DRAW, WHITE_WIN, NO_RESULT
	};

	tGameCollector();
	~tGameCollector() = default;

	bool openFile();

	inline void registerPosition(const std::string& fen) {
		pos_bufor.push_back(fen);
	}

	void flushBufor();
	void setGameResult(GameResult gr);

private:
	inline void clearBufor() noexcept {
		pos_bufor.clear();
		game_res = NO_RESULT;
	}

	std::vector<std::string> pos_bufor;
	GameResult game_res;
	std::array<std::pair<std::ofstream*, std::string_view>, 3> files;
	std::ofstream file_1, file_0, file_0_5;
};

#if COLLECT_POSITION_DATA
extern tGameCollector game_collector;
#endif

struct Position {
	bool isQuiet();

	BitBoardsSet bbs;
	gState state;
};

class tData {
public:
	tData();
	~tData() = default;
	bool openDataFile();

	bool loadTrainingData();
	double evaluatePosition(const Position& pos);

	std::vector<double> gres;
	std::vector<Position> position;

	size_t position_n;

private:
	std::array<std::tuple<std::ifstream*, std::string_view, double>, 3> files;
	std::ifstream file_1, file_0, file_0_5;
};


class tTuning {
public:
	tTuning();
	bool openSessionFile();

	void initTuningData();
	void updateK();

	void runWeightTuning();
	void printIndexInfo(const size_t i);
	void printEvalError();

private:
	double differenceQuotient(int* const param);
	void optimizeParameters();
	void printTrainingResults();

	double computeK(const int k_precision = 10);
	double computeE(const double g_k);
	double sigmoid(const double g_k, const double eval_to_map);
	
	// previous scaling constant K = (0.861318, 0.943628)
	static constexpr double pre_computed_k = 0.900358;
	// default number of parameters = sizeof(Eval::params) / sizeof(int)
	static constexpr size_t tuned_param_number = 6;
	static constexpr std::string_view filepath =
		R"(C:/Users/User/source/repos/ChessEngine/source/data/TuningSession.txt)";

	double k;
	tData data;

	std::array<int*, tuned_param_number> eval_params;
	std::ofstream session_file;

	Time time_data;
};

#if ENABLE_TUNING
extern tTuning tuning;
#endif