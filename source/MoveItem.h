#pragma once

#include "BitBoard.h"
#include "BitBoardsSet.h"
#include "MoveSystem.h"


/* move list resources -
	* move item consists of:
	* 0000 0000 0000 0000 0011 1111 origin square    - 6 bits (0..63 val range)
	* 0000 0000 0000 1111 1100 0000 target square    - 6 bits (0..63 val range)
	* 0000 0000 0111 0000 0000 0000 piece            - 3 bits (0..5 value range)
	* 0000 0000 1000 0000 0000 0000 double push flag - 1 bit
	* 0000 0001 0000 0000 0000 0000 capture flag     - 1 bit
	* 0000 0010 0000 0000 0000 0000 en passant flag  - 1 bit
	* 0000 0100 0000 0000 0000 0000 castle flag      - 1 bit
	* 0000 1000 0000 0000 0000 0000 side flag        - 1 bit
	* 0111 0000 0000 0000 0000 0000 promotion flag   - 3 bits (0..5)
	* 
	*						    23 bits of total:
	*						    uint32_t is enought for storing move data
	*/

namespace MoveItem {

	// enum classes used for separating bit masks and encoding type
	// masks for extracting relevant bits from 32-bit int data
	enum class iMask {
		ORIGIN        = 0x3F,
		TARGET        = 0xFC0,
		PIECE         = 0x7000,
		DOUBLE_PUSH_F = 0x8000,
		CAPTURE_F     = 0x10000,
		EN_PASSANT_F  = 0x20000,
		CASTLE_F      = 0x40000,
		SIDE_F        = 0x80000,
		PROMOTION     = 0x700000
	};

	// encoding mode
	enum class encodeType {
		QUIET,
		CAPTURE,
		DOUBLE_PUSH,
		EN_PASSANT,
		CASTLE
	};


	class iMove {
	public:
		iMove(uint32_t enc)
			: cmove(enc) {}
		iMove(const iMove&) = default;
		iMove() = default;

		// no need to return a value
		void operator=(uint32_t data) noexcept {
			cmove = data;
		}

		bool operator==(iMove b) {
			return cmove == b.cmove;
		}

		template <iMask MASK>
		inline uint32_t getMask() const noexcept {
			return cmove & static_cast<uint32_t>(MASK);
		}

		inline auto& print() const {
			return std::cout << ' ' << index_to_square[getMask<iMask::ORIGIN>()]
				<< index_to_square[getMask<iMask::TARGET>() >> 6] << '\n';
		}
	private:
		uint32_t cmove;
	};

	// claim move item states and return encoded move item ready to save in move list
	// special functions of more arguments than only origin and target squares:
		
	// decide about capture flag relevancy
	template <enumPiece PC, enumSide SIDE>
	inline uint32_t encodeQuietCapture(int origin, int target, bool capture) noexcept {
		return (SIDE << 19) | (capture << 16) | (PC << 12) | (target << 6) | origin;
	}

	template <enumSide SIDE>
	inline uint32_t encodeQuietCapture(int origin, int target, bool capture, enumPiece pc) noexcept {
		return (SIDE << 19) | (capture << 16) | (pc << 12) | (target << 6) | origin;
	}

	// encode promotion and picked promotion piece
	template <enumSide Side, enumPiece Promoted, bool Capture = false>
	inline uint32_t encodePromotion(int origin, int target) noexcept {
		return (Promoted << 20) | (Side << 19) | (Capture << 16) | (target << 6) | origin;
	}

	template <enumSide Side>
	inline uint32_t encodeEnPassant(int origin, int target) noexcept {
		return (Side << 19) | (1 << 17) | (1 << 16) | (target << 6) | origin;
	}

	template <enumSide Side>
	inline uint32_t encodeCastle(int origin, int target) noexcept {
		return (Side << 19) | (1 << 18) | (KING << 12) | (target << 6) | origin;
	}

	// direct function template as a parameter of a specific encoding mode
	template <encodeType eT>
	uint32_t encode(int origin, int target, enumPiece piece, enumSide side);

	template <>
	inline uint32_t encode<encodeType::QUIET>(int origin, int target, enumPiece piece, enumSide side) noexcept {
		return (side << 19) | (piece << 12) | (target << 6) | origin;
	}

	template <>
	inline uint32_t encode<encodeType::CAPTURE>(int origin, int target, enumPiece piece, enumSide side) noexcept {
		return (side << 19) | (piece << 12) | (1 << 16) | (target << 6) | origin;
	}

	template <>
	inline uint32_t encode<encodeType::DOUBLE_PUSH>(int origin, int target, enumPiece piece, enumSide side) noexcept {
		return (side << 19) | (piece << 12) | (1 << 15) | (target << 6) | origin;
	}

	template <enumSide SIDE>
	MoveItem::iMove toMove(int target, int origin, char promo) {
		const bool is_pawn = bitU64(origin) & BBs[nWhitePawn + SIDE],
			capture = bitU64(target) & BBs[nBlack - SIDE];

		// check en passant capture
		if (is_pawn and target == game_state.ep_sq + (SIDE ? Compass::nort : Compass::sout)) {
			return encodeEnPassant<SIDE>(origin, target);
		} // promotion detection
		else if (is_pawn and promo != '\0') {
			MoveItem::iMove res;

			switch (std::tolower(promo)) {
			case 'n':
				res = capture ? encodePromotion<SIDE, KNIGHT, true>(origin, target)
					: encodePromotion<SIDE, KNIGHT>(origin, target);
				break;
			case 'b':
				res = capture ? encodePromotion<SIDE, BISHOP, true>(origin, target)
					: encodePromotion<SIDE, BISHOP>(origin, target);
				break;
			case 'r':
				res = capture ? encodePromotion<SIDE, ROOK, true>(origin, target)
					: encodePromotion<SIDE, ROOK>(origin, target);
				break;
			case 'q':
				res = capture ? encodePromotion<SIDE, QUEEN, true>(origin, target)
					: encodePromotion<SIDE, QUEEN>(origin, target);
				break;
			default: break;
			}

			return res;
		} // double push detection
		else if (is_pawn and ((SIDE ? Constans::r5_rank : Constans::r4_rank) & bitU64(target))) {
			return encode<encodeType::DOUBLE_PUSH>(origin, target, PAWN, SIDE);
		} // check castle
		else if (getLS1BIndex(BBs[nWhiteKing + SIDE]) == origin and
			((game_state.castle.checkLegalCastle<SIDE & ROOK>() and target == SIDE ? g8 : g1) or
				(game_state.castle.checkLegalCastle<SIDE & QUEEN>() and target == SIDE ? c8 : c1))) {
			return encodeCastle<SIDE>(origin, target);
		}

		// get piece type
		for (auto piece = nWhitePawn + SIDE; piece <= nBlackKing; piece += 2) {
			if (bitU64(origin) & BBs[piece]) {
				return encodeQuietCapture<SIDE>(origin, target, capture, toPieceType(piece));
			}
		}

		return 0;
	}

} // namespace MoveItem