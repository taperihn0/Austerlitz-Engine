#include "HeaderFile.h"
#include <string>

// global variables storing universal piece data
BitBoardsSet BBs;

CSinglePieceAttacks<PAWN> cpawn_attacks;
CSinglePieceAttacks<KNIGHT> cknight_attacks;
CSinglePieceAttacks<KING> cking_attacks;

mData mdata;
gState game_state;

int main() {

	InitState::initMAttacksTables<ROOK>();
	InitState::initMAttacksTables<BISHOP>();

	BBs.parseFEN("7R/1P2P1B1/p1nP4/2k5/4Q3/p2K3p/P6p/2r1b3 w - - 0 1");
	BBs.printBoard();

	auto move_list = MoveGenerator::generateLegalMoves();
	MoveGenerator::Analisis::populateMoveList(move_list);

	auto bbs = BBs;
	auto gs = game_state;

	for (auto& move : move_list) {
		std::cout << index_to_square[move.getMask<MoveItem::iMask::ORIGIN>()]
			<< index_to_square[move.getMask < MoveItem::iMask::TARGET>() >> 6] << '\n';
		
		std::cin.get();

		MovePerform::makeMove(move);
		BBs.printBoard();
		MovePerform::unmakeMove(bbs, gs);
	}

	/*
	BBs.clear();
	BBs.parseFEN(fen);
	BBs.printBoard();

	auto move_list = MoveGenerator::generateLegalMoves();
	MoveGenerator::Analisis::populateMoveList(move_list);
	*/
}