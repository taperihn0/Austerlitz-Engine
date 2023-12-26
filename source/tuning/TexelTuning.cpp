#include "TexelTuning.h"
#include "../engine/Search.h"
#include "../engine/BitBoardsSet.h"
#include "../engine/Zobrist.h"
#include <sstream>
#include <cmath>
#include <limits>


bool tGameCollector::openFile() {
	file.open(tData::getFilePath(), std::ios::app);

	if (!file.is_open()) {
		OS << "Failed to open file: " << tData::getFilePath() << '\n';
		return false;
	}

	return true;
}

void tGameCollector::flushBufor() {
	storeGameResult();

	for (const auto& pos : pos_bufor) {
		file << pos << "# " << game_res * 0.5 << '\n';
	}

	clearBufor();
}

void tGameCollector::storeGameResult() {
	m_search.time_data.setFixedTime(0);
	const int score = m_search.alphaBeta<false>(mSearch::low_bound, mSearch::high_bound, 3, 0);

	if (score < mSearch::mate_comp) registerGameResult(game_state.turn ? WHITE_WIN : WHITE_LOSE);
	else if (score > -mSearch::mate_comp) registerGameResult(game_state.turn ? WHITE_LOSE : WHITE_WIN);
	else registerGameResult(DRAW);
}


bool tData::openDataFile() {
	file.open(filepath.data(), std::ifstream::beg);

	if (!file.is_open()) {
		OS << "Failed to open file: " << filepath << '\n';
		return false;
	}

	return true;
}

bool tData::loadTrainingData() {
	std::istringstream iss;

	std::string line, fen, res;

	for (int i = 1; getline(file, line); i++) {
		iss.clear();
		iss.str(line);

		getline(iss, fen, '#');
		iss >> std::skipws >> res;

		position.push_back(fen);
		gres.push_back(stod(res));
	}

	position_n = position.size();
	return !position.empty() and gres.size() == position.size();
}

void tData::fillEvalSet() {
	eval.resize(position_n);

	for (int i = 0, abs_qeval; i < position_n; i++) {
		BBs.parseFEN(position[i]);
		abs_qeval = m_search.qSearch(mSearch::low_bound, mSearch::high_bound, 0);
		eval[i] = game_state.turn ? -abs_qeval : abs_qeval;
	}
}


tTuning::tTuning()
: k(pre_computed_k) {}

void tTuning::initTuningData() {
	assert(data.openDataFile());
	assert(data.loadTrainingData());
	data.fillEvalSet();
}

void tTuning::updateK() {
	const double new_k_val = computeK();
	OS << "K = " << new_k_val << '\n';
	k = new_k_val;
}

double tTuning::computeK(const int k_precision) {
	double start = 0., end = 10., step = 1., curr = start, 
		best = std::numeric_limits<double>::max(), error;

	for (int i = 0; i < k_precision; i++) {
		while (curr < end) {
			error = computeE(curr);

			if (error <= best) {
				OS << "curr best: " << curr << '\n';
				best = error;
				start = curr;
			}

			curr += step;
		}

		end = start + step;
		start = start - step;
		step /= 10.;
		curr = start;
	}

	return start;
}

double tTuning::computeE(const double g_k) {
	double res = 0.;

	for (int i = 0; i < data.position_n; i++) {
		res += std::pow(data.gres[i] - sigmoid(g_k, static_cast<double>(data.eval[i])), 2);
	}

	return res / static_cast<double>(data.position_n);
}

inline double tTuning::sigmoid(const double g_k, const double g_q) {
	return 1 / (1 + std::pow(10, -g_k * g_q / 400));
}