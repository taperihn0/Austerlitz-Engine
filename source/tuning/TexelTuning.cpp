#include "TexelTuning.h"
#include "../engine/Search.h"
#include "../engine/BitBoardsSet.h"
#include "../engine/Zobrist.h"
#include <sstream>
#include <cmath>
#include <limits>


bool tGameCollector::openFile() {
	file.open(filepath.data(), std::ios::app);

	if (!file.is_open()) {
		OS << "Failed to open file: " << filepath << '\n';
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

inline constexpr std::string_view operator "" _sv(const char data[], size_t) {
	return std::string_view(data);
}

tData::tData()
: files{ 
		std::make_pair(&file_1, R"(C:/Users/User/source/repos/ChessEngine/source/data/1.txt)"_sv),
		std::make_pair(&file_0, R"(C:/Users/User/source/repos/ChessEngine/source/data/0.txt)"_sv),
		std::make_pair(&file_0_5, R"(C:/Users/User/source/repos/ChessEngine/source/data/0.5.txt)"_sv)
} {}

tData::~tData() {
	for (auto& [file, path] : files)
		file->close();
}

bool tData::openDataFile() {
	for (auto& [file, path] : files) {
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

	for (auto& [file, path] : files) {
		while (getline(*file, line)) {
			iss.clear();
			iss.str(line);

			getline(iss, fen, '#');
			iss >> std::skipws >> res;

			position.push_back(fen);
			gres.push_back(stod(res));
		}
	}

	position_n = position.size();
	return !position.empty() and gres.size() == position.size();
}

void tData::fillEvalSet() {
	eval.resize(position_n);
	m_search.time_data.setFixedTime(0);

	for (int i = 0, abs_qeval; i < position_n; i++) {
		BBs.parseFEN(position[i]);
		abs_qeval = m_search.qSearch(mSearch::low_bound, mSearch::high_bound, 0);
		eval[i] = game_state.turn ? -abs_qeval : abs_qeval;
	}
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

	data.fillEvalSet();
	const double new_k_val = computeK();
	OS << "K = " << new_k_val << '\n';
	k = new_k_val;

	OS << sinceStart_ms(time_data.start) << "ms\n";
}

void tTuning::runWeightTuning() {
	time_data.start = now();
	gradientDescent();

	for (int* const param : eval_params) {
		OS << *param << '\n';
		session_file << *param << '\n';
	}

	session_file << sinceStart_ms(time_data.start) << "ms\n";
}

void tTuning::printIndexInfo(const size_t i) {
	if (data.eval.empty())
		data.fillEvalSet();
	OS << data.eval[i] << ' ' << sigmoid(k, data.eval[i]) << '\n';
	OS << "E = " << computeE(k) << '\n';
}

inline double tTuning::differenceQuotient(int* const param) {
	*param -= 1;
	data.fillEvalSet();
	const double a = computeE(k);

	*param += 2;
	data.fillEvalSet();
	const double b = computeE(k);

	*param -= 1;
	return (b - a) / 2;
}

void tTuning::gradientDescent() {
	static constexpr double threshold = 0.00002;
	bool stop = false;

	while (!stop) {
		stop = true;

		for (int* const param : eval_params) {
			const double diff = differenceQuotient(param);

			OS << "log: diff " << diff << '\n';

			if (std::abs(diff) < threshold) continue;
			else stop = false;

			const int step = static_cast<int>(-diff * 100000 * 5 / 9);
			OS << "log: " << *param << " step " << step << '\n';
			*param += step;
		}
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
		res += std::pow(data.gres[i] - sigmoid(g_k, static_cast<double>(data.eval[i])), 2);
	}

	return res / static_cast<double>(data.position_n);
}

inline double tTuning::sigmoid(const double g_k, const double g_q) {
	return 1 / (1 + std::pow(10, -g_k * g_q / 400));
}