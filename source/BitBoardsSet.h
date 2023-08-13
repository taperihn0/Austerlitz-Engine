#include "BitBoard.h"

// game state variables
namespace gState {
	namespace {
		int ep_sq;
		bool turn;
		int castle, halfmove, fullmove;
	}
}

// piece enum for their bitboards
enum enumPiece_bbs {
	nWhitePawn,
	nWhiteKnight,
	nWhiteBishop,
	nWhiteRook,
	nWhiteQueen,
	nWhiteKing,

	nBlackPawn,
	nBlackKnight,
	nBlackBishop,
	nBlackRook,
	nBlackQueen,
	nBlackKing,

	nWhite,
	nBlack,
	nEmpty,
	nOccupied,
};

// bitboards set
class BitBoardsSet {
public:
	BitBoardsSet();
	BitBoardsSet(const std::string& _fen);
	
	// pieces bitboard initialization using FEN method
	void parseFEN(const std::string& fen);
	void parseGState(int i);

	U64& operator[](enumPiece_bbs piece_get) noexcept;
	const std::string& getFEN() noexcept;

	// display entire board of all pieces
	void printBoard();
private:
	std::array<char, 12> piece_char;
	std::array<U64, 16> bbs;
	std::string fen;
};


inline U64& BitBoardsSet::operator[](enumPiece_bbs piece_get) noexcept {
	return bbs[piece_get];
}

inline const std::string& BitBoardsSet::getFEN() noexcept {
	return fen;
}

extern BitBoardsSet BBs;