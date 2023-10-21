#pragma once

#include "BitBoard.h"

class castleRights {
public:
	castleRights() = default;
	
	enum class cSide {
		bQUEEN,
		bROOK,
		wQUEEN,
		wROOK,
	};

	void operator=(const castleRights& cr);

	inline void operator=(uint8_t d) noexcept {
		decoded = d;
	}

	inline void operator|=(int s) noexcept {
		decoded |= s;
	}

	inline auto operator&(int s) noexcept {
		return decoded & s;
	}

	template <cSide SIDE>
	inline bool checkLegalCastle() noexcept {
		return decoded & (1 << static_cast<int>(SIDE));
	}

	inline void operator&=(uint32_t mask) {
		decoded &= mask;
	}

	inline uint32_t raw() {
		return decoded;
	}
private:

	/*
	 * decoding castle rights:
	 * KQkq in FEN Notation
	 * K stands for 4th bit in decoded variable, Q stands for 3th bit
	 * k stands for 2nd bit and q stands for 1st bit
	 * decoded variable bits:	0	 0	  0	    0
	 * FEN Notation:			K	 Q	  k     q
	 */
	 
	uint8_t decoded;
};

inline void castleRights::operator=(const castleRights& cr) {
	decoded = cr.decoded;
}

inline constexpr castleRights::cSide operator&(enumSide SIDE, enumPiece PC) {
	return (SIDE == WHITE) ? 
			(PC == ROOK ? castleRights::cSide::wROOK : castleRights::cSide::wQUEEN) : 
		    (PC == ROOK ? castleRights::cSide::bROOK : castleRights::cSide::bQUEEN);
}


// game state variables
struct gState {
	static constexpr int max_move_rule = 50,
		start_game_score = 28150,
		end_game_score = 23300,
		middle_game_score = 26450;

	enum gPhase { OPENING = 0, MIDDLEGAME, ENDGAME };

	inline bool is50moveDraw() noexcept {
		return halfmove >= max_move_rule;
	}

	inline gPhase gamePhase() noexcept {
		const int total_mat = material[0] + material[1];
		return total_mat < end_game_score ? ENDGAME :
			fullmove > 10 or total_mat <= middle_game_score or !castle.raw() ? MIDDLEGAME : OPENING;
	}

	int ep_sq;
	enumSide turn;
	castleRights castle;
	int halfmove, fullmove;
	std::array<int, 2> material;
};

extern gState game_state;


// piece enum for their bitboards
enum enumPiece_bbs : int {
	nWhitePawn,
	nBlackPawn,

	nWhiteKnight,
	nBlackKnight,

	nWhiteBishop,
	nBlackBishop,

	nWhiteRook,
	nBlackRook,

	nWhiteQueen,
	nBlackQueen,

	nWhiteKing,
	nBlackKing,

	nWhite,
	nBlack,
	nEmpty,
	nOccupied,
};

// custom operators returning proper type of index of std::array
inline constexpr size_t operator-(enumPiece_bbs pc, enumSide s) noexcept {
	return static_cast<size_t>(pc) - static_cast<size_t>(s);
}

inline constexpr size_t operator+(enumPiece_bbs pc, enumSide s) noexcept {
	return static_cast<size_t>(pc) + static_cast<size_t>(s);
}

constexpr std::array<enumPiece, 12> _pc_cast = { PAWN, PAWN, KNIGHT, KNIGHT, BISHOP, BISHOP, ROOK, ROOK, QUEEN, QUEEN, KING, KING };
inline constexpr enumPiece toPieceType(size_t pc) noexcept {
	//return _pc_cast[pc];
	return static_cast<enumPiece>(pc / 2);
}

// handy functions helping getting proper piece index in bitboards set
namespace {

	template <enumPiece PC>
	enumPiece_bbs bbsIndex();

	template <enumPiece PC>
	inline enumPiece_bbs bbsIndex() {
		return nEmpty;
	}

	template <>
	inline enumPiece_bbs bbsIndex<KNIGHT>() {
		return nWhiteKnight;
	}

	template <>
	inline enumPiece_bbs bbsIndex<BISHOP>() {
		return nWhiteBishop;
	}

	template <>
	inline enumPiece_bbs bbsIndex<ROOK>() {
		return nWhiteRook;
	}

	template <>
	inline enumPiece_bbs bbsIndex<QUEEN>() {
		return nWhiteQueen;
	}

	template <bool Side>
	enumPiece_bbs bbsIndex(uint32_t pc);

	template <>
	enumPiece_bbs bbsIndex<WHITE>(uint32_t pc) {
		return pc == PAWN ? nWhitePawn :
			pc == KNIGHT ? nWhiteKnight :
			pc == BISHOP ? nWhiteBishop :
			pc == ROOK ? nWhiteRook :
			pc == QUEEN ? nWhiteQueen :
			nWhiteKing;
	}

	template <>
	enumPiece_bbs bbsIndex<BLACK>(uint32_t pc) {
		return pc == PAWN ? nBlackPawn :
			pc == KNIGHT ? nBlackKnight :
			pc == BISHOP ? nBlackBishop :
			pc == ROOK ? nBlackRook :
			pc == QUEEN ? nBlackQueen :
			nBlackKing;
	}

}

class BitBoardsSet {
public:
	BitBoardsSet() = default;
	BitBoardsSet(const BitBoardsSet& bbs) noexcept(nothrow_copy_assign);
	BitBoardsSet(const std::string& fen);

	// pieces bitboard initialization using FEN method
	void parseFEN(const std::string& fen);
	void parseGState(const std::string& fen, int i);

	U64& operator[](size_t piece_get);
	void operator=(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign);

	// display entire board of all pieces
	void printBoard();

	void clear();

	int count(size_t piece_get);

	static constexpr const char* start_pos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
private:
	static constexpr bool nothrow_copy_assign = std::is_nothrow_copy_assignable_v<std::array<U64, 16>>;

	// central storage of actual piece structure of every type
	std::array<U64, 16> bbs;
};

inline U64& BitBoardsSet::operator[](size_t piece_get) {
	return bbs[piece_get];
}

inline void BitBoardsSet::operator=(const BitBoardsSet& cbbs) noexcept(nothrow_copy_assign) {
	bbs = cbbs.bbs;
}

inline void BitBoardsSet::clear() {
	bbs.fill(eU64);
}

inline int BitBoardsSet::count(size_t piece_get) {
	return bitCount(bbs[piece_get]);
}

// declare global BitBoardsSet containing piece structure data
extern BitBoardsSet BBs;