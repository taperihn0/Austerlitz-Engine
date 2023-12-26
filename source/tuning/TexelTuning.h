#pragma once

#include "../engine/Evaluation.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#define COLLECT_POSITION_DATA false
#define ENABLE_TUNING true

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
	tData() = default;
	bool openDataFile();

	bool loadTrainingData();
	void fillEvalSet();

	static inline constexpr const char* getFilePath() noexcept {
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
	bool openSessionFile();

	void initTuningData();
	void updateK();

	void runWeightTuning();
private:
	void optimizeParameter(int* const param);
	int localSearch(int* const param, const int lbound, const int hbound);

	double computeK(const int k_precision = 10);
	double computeE(const double g_k);
	double sigmoid(const double g_k, const double g_q);

	static constexpr double pre_computed_k = 0.529958;
	static constexpr size_t tuned_param_number = sizeof(Eval::params) / 4;
	static constexpr std::string_view filepath =
		R"(C:/Users/User/source/repos/ChessEngine/source/data/TuningSession.txt)";
	static constexpr unsigned range_threshold = 15;

	double k;
	tData data;

	std::array<int*, tuned_param_number> eval_params;
	std::ofstream session_file;
};

#if ENABLE_TUNING
extern tTuning tuning;
#endif