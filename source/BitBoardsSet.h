#pragma once

#include "BitBoard.h"

class castleRights {
public:
	castleRights() = default;
	
	enum class cSide {
		bQUEEN, bROOK,
		wQUEEN, wROOK,
	};

	inline void operator=(const castleRights& cr) noexcept { decoded = cr.decoded; }
	inline void operator=(uint8_t d) noexcept { decoded = d; }
	inline void operator|=(int s) noexcept { decoded |= s; }
	inline auto operator&(int s) noexcept { return decoded & s; }
	inline void operator&=(uint32_t mask) noexcept { decoded &= mask; }
	inline uint32_t raw() noexcept { return decoded; }

	template <cSide SIDE>
	inline bool checkLegalCastle() noexcept { return decoded & (1 << static_cast<int>(SIDE)); }

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

inline constexpr castleRights::cSide operator&(enumSide SIDE, enumPiece PC) noexcept {
	return (SIDE == WHITE) ? 
			(PC == ROOK ? castleRights::cSide::wROOK : castleRights::cSide::wQUEEN) : 
		    (PC == ROOK ? castleRights::cSide::bROOK : castleRights::cSide::bQUEEN);
}


// piece enum for their bitboards
enum enumPiece_bbs {
	nWhitePawn, nBlackPawn,
	nWhiteKnight, nBlackKnight,
	nWhiteBishop, nBlackBishop,
	nWhiteRook, nBlackRook,
	nWhiteQueen, nBlackQueen,
	nWhiteKing, nBlackKing,
	nWhite, nBlack,
	nEmpty, nOccupied,
};

// custom operators returning proper type of index of std::array
inline constexpr size_t operator-(enumPiece_bbs pc, enumSide s) noexcept {
	return static_cast<size_t>(pc) - static_cast<size_t>(s);
}

inline constexpr size_t operator+(enumPiece_bbs pc, enumSide s) noexcept {
	return static_cast<size_t>(pc) + static_cast<size_t>(s);
}


namespace pcCastLookUp {
	constexpr std::array<enumPiece, 12> pc_cast = {
		PAWN, PAWN, KNIGHT, KNIGHT, BISHOP, BISHOP, ROOK, ROOK, QUEEN, QUEEN, KING, KING
	};
}

// get enumPiece from bitboard piece type
inline constexpr enumPiece toPieceType(size_t pc) {
	return pcCastLookUp::pc_cast[pc];
}

// handy functions helping getting proper piece index in bitboards set
namespace {

	template <enumPiece PC>
	inline enumPiece_bbs bbsIndex() noexcept;

	template <enumPiece PC>
	inline enumPiece_bbs bbsIndex() noexcept { return nEmpty; }

	template <>
	inline enumPiece_bbs bbsIndex<KNIGHT>() noexcept { return nWhiteKnight; }

	template <>
	inline enumPiece_bbs bbsIndex<BISHOP>() noexcept { return nWhiteBishop; }

	template <>
	inline enumPiece_bbs bbsIndex<ROOK>() noexcept { return nWhiteRook; }

	template <>
	inline enumPiece_bbs bbsIndex<QUEEN>() noexcept { return nWhiteQueen; }

	template <bool Side>
	inline enumPiece_bbs bbsIndex(uint32_t pc);

	template <>
	inline enumPiece_bbs bbsIndex<WHITE>(uint32_t pc) {
		return pc == PAWN ? nWhitePawn :
			pc == KNIGHT ? nWhiteKnight :
			pc == BISHOP ? nWhiteBishop :
			pc == ROOK ? nWhiteRook :
			pc == QUEEN ? nWhiteQueen :
			nWhiteKing;
	}

	template <>
	inline enumPiece_bbs bbsIndex<BLACK>(uint32_t pc) {
		return pc == PAWN ? nBlackPawn :
			pc == KNIGHT ? nBlackKnight :
			pc == BISHOP ? nBlackBishop :
			pc == ROOK ? nBlackRook :
			pc == QUEEN ? nBlackQueen :
			nBlackKing;
	}

	inline enumPiece_bbs bbsIndex(uint32_t pc, bool side) {
		return static_cast<enumPiece_bbs>(pc * 2 + side);
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


// game state variables
struct gState {
	enum gPhase {
		OPENING = 0, MIDDLEGAME, ENDGAME
	};

	enum Constans {
		START_SCORE = 28150,
		MIDDLEPHASE_SCORE = 26450,
		ENDPHASE_SCORE = 23300,
		MAX_MOVE_RULE = 50
	};

	inline bool is50moveDraw() noexcept {
		return halfmove >= MAX_MOVE_RULE;
	}

	inline gPhase gamePhase() noexcept {
		const int total_material = material[0] + material[1];

		return total_material < ENDPHASE_SCORE ? ENDGAME :
			fullmove > 10 or total_material <= MIDDLEPHASE_SCORE or !castle.raw() ? MIDDLEGAME :
			OPENING;
	}

	inline bool isPawnEndgame() noexcept {
		const int total_material = material[0] + material[1] - 20000;
		return (total_material / 100) == bitCount(BBs[nWhitePawn] | BBs[nBlackPawn]);
	}

	int ep_sq;
	enumSide turn;
	castleRights castle;
	int halfmove, fullmove;
	std::array<int, 2> material;
};

extern gState game_state;
