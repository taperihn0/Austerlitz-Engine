#include "TexelTuning.h"
#include "../engine/Search.h"
#include "../engine/BitBoardsSet.h"
#include "../engine/Zobrist.h"
#include <sstream>
#include <cmath>
#include <limits>
#include <iomanip>


inline constexpr std::string_view operator "" _sv(const char data[], size_t) {
	return std::string_view(data);
}


tGameCollector::tGameCollector()
	: files {
		std::make_pair(&file_1, R"(C:/Users/User/source/repos/ChessEngine/source/data/1.txt)"_sv),
		std::make_pair(&file_0, R"(C:/Users/User/source/repos/ChessEngine/source/data/0.txt)"_sv),
		std::make_pair(&file_0_5, R"(C:/Users/User/source/repos/ChessEngine/source/data/0.5.txt)"_sv)
	}, game_res(NO_RESULT)
{}

bool tGameCollector::openFile() {
	for (auto& [file, path] : files) {
		file->open(path.data(), std::ios::app);

		if (!file->is_open()) {
			OS << "Failed to open file: " << path << '\n';
			return false;
		}
	}

	return true;
}

void tGameCollector::flushBufor() {

	// if engine didn't come up with a winner, it's a draw
	if (game_res == NO_RESULT)
		game_res = DRAW;

	for (const auto& pos : pos_bufor) {
		switch (game_res) {
		case WHITE_LOSE:
			file_0 << pos << '\n';
			break;
		case WHITE_WIN:
			file_1 << pos << '\n';
			break;
		case DRAW:
			file_0_5 << pos << '\n';
			break;
		default: break;
		}
	}

	clearBufor();
}

void tGameCollector::setGameResult(GameResult gr) {
	game_res = gr;
}


bool Position::isQuiet() {
	MoveList ml;
	MoveGenerator::generateLegalMoves<MoveGenerator::CAPTURES>(ml);
	return !ml.size();
}


tData::tData()
	: files {
		std::make_tuple(&file_1, R"(C:/Users/User/source/repos/ChessEngine/source/data/tuner_1.txt)"_sv, 1.),
		std::make_tuple(&file_0, R"(C:/Users/User/source/repos/ChessEngine/source/data/tuner_0.txt)"_sv, 0.),
		std::make_tuple(&file_0_5, R"(C:/Users/User/source/repos/ChessEngine/source/data/tuner_0.5.txt)"_sv, 0.5)
	}, position_n(0)
{}

bool tData::openDataFile() {
	for (auto& [file, path, game_res] : files) {
		file->open(path.data(), std::ifstream::beg);

		if (!file->is_open()) {
			OS << "Failed to open file: " << path << '\n';
			return false;
		}
	}

	return true;
}

bool tData::loadTrainingData() {
	std::istringstream iss;
	std::string line, fen, res;

	OS << "Loading dataset ";

	for (auto& [file, path, game_result] : files) {
		while (getline(*file, line)) {
			iss.clear();
			iss.str(line);

			getline(iss, fen, '\n');

			BBs.parseFEN(fen);
			Position pos = { BBs, game_state };

			if (pos.isQuiet()) {
				position.push_back(pos);
				gres.push_back(game_result);
			}
		}

		OS << '.';
	}

	position_n = position.size();

	if (!position_n or gres.size() != position.size())
		return false;

	OS << std::endl << "Succesfully loaded training dataset" << std::endl
	   << "Total data samples: " << position_n << '\n';

	return true;
}

double tData::evaluatePosition(const Position& pos) {
	BBs = pos.bbs;
	game_state = pos.state;
	//game_state.turn = WHITE;
	//return static_cast<double>(Eval::evaluate(mSearch::low_bound, mSearch::high_bound));
	return static_cast<double>(Eval::materialEvaluation());
}


tTuning::tTuning()
: k(pre_computed_k) {

	// load parameters' pointers to change them during finding minimum
	void* s_ptr = &Eval::params;

	for (int i = 0, *ptr = reinterpret_cast<int*>(s_ptr); i < tuned_param_number; i++, ptr++) {
		eval_params[i] = ptr;
	}
}

bool tTuning::openSessionFile() {
	session_file.open(filepath.data(), std::ios::app);

	if (!session_file.is_open()) {
		OS << "Failed to open file: " << filepath << '\n';
		return false;
	}

	return true;
}

void tTuning::initTuningData() {
	assert(data.openDataFile());
	assert(data.loadTrainingData());
}

void tTuning::updateK() {
	time_data.start = now();

	const double new_k_val = computeK();
	OS << "K = " << new_k_val << '\n';
	k = new_k_val;

	OS << sinceStart_ms(time_data.start) << "ms\n";
}

void tTuning::runWeightTuning() {
	assert(openSessionFile());
	time_data.start = now();
	optimizeParameters();
	printTrainingResults();
}

void tTuning::printTrainingResults() {
	session_file << "{\n";

	for (int* const param : eval_params) {
		OS << *param << '\n';
		session_file << '\t' << *param << '\n';
	}

	session_file << "}\n"
		<< sinceStart_ms(time_data.start) << "ms\n";
}

void tTuning::printIndexInfo(const size_t i) {
	const double pos_eval = data.evaluatePosition(data.position[i - 1]);
	BBs.printBoard();
	OS << pos_eval << ' ' << sigmoid(k, pos_eval) << '\n';
}

void tTuning::printEvalError() {
	OS << "E = " << computeE(k) << '\n';
}

double tTuning::differenceQuotient(int* const param) {
	*param -= 1;
	const double a = computeE(k);

	*param += 2;
	const double b = computeE(k);

	*param -= 1;
	return b - a;
}

// vanilla gradient descent algorithm
void tTuning::optimizeParameters() {
	static constexpr double threshold = 0.00001;
	static constexpr int step_limit = 1000000;
	int k = 0;

	std::vector<double> gradient(tuned_param_number);
	bool found = false;

	OS << std::setprecision(10);

	while (k < step_limit) {
		found = true;

		for (int i = 0; i < tuned_param_number; i++) {
			const double diff = differenceQuotient(eval_params[i]);
			gradient[i] = diff;
			if (std::abs(diff) > threshold) found = false;
		}

		if (found) break;

		OS << "log (" << k << "):\n";

		for (int i = 0; i < tuned_param_number; i++) {
			double rate = gradient[i] * 100000.;
			const int step = static_cast<int>(rate < 0. ? std::floor(rate) : std::ceil(rate));

			OS  << i << " parameter " << *eval_params[i] << '\n'
				<< "diff: " << gradient[i] << " rate: " << rate << " step: " << step << '\n';

			*eval_params[i] -= step;
		}

		OS << '\n';

		k++;
	}
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
		const double error = data.gres[i] - sigmoid(g_k, data.evaluatePosition(data.position[i]));
		res += error * error;
	}

	return res / static_cast<double>(data.position_n);
}

double tTuning::sigmoid(const double g_k, const double eval_to_map) {
	return 1. / (1. + std::pow(10, -g_k * eval_to_map / 400.));
}