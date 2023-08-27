#pragma once

#include "LegalityTest.h"
#include "MoveSystem.h"


namespace {
	namespace pinData {
		// cache table to store inbetween paths of king and his x-ray attackers
		// for pinned pieces, since such pinned piece can only move through their inbetween path
		std::array<U64, 64> inbetween_blockers;
		U64 pinned;
	}
}


/* move list resources -
 * move item consists of:
 * 0000 0000 0000 0011 1111 origin square   - 6 bits (0..63)
 * 0000 0000 1111 1100 0000 target square   - 6 bits (0..63)
 * 0000 0111 0000 0000 0000 promoted piece  - 3 bits (0..5)
 * 0000 1000 0000 0000 0000 double push flag - 1 bit
 * 0001 0000 0000 0000 0000 capture flag     - 1 bit
 * 0010 0000 0000 0000 0000 en passant flag  - 1 bit
 * 0100 0000 0000 0000 0000 castle flag      - 1 bit
 *						    19 bits of total:
 *						    uint32_t is enought for storing move data
 */


namespace {
	namespace MoveItem {

		// enum classes used for separating bit masks and encoding type
		// masks for extracting relevant bits from 32-bit int data
		enum class iMask {
			ORIGIN = 0x3F,
			TARGET = 0xFC0,
			PROMOTED = 0x7000,
			DOUBLE_PUSH_F = 0x8000,
			CAPTURE_F = 0x10000,
			EN_PASSANT_F = 0x20000,
			CASTLE_F = 0x40000
		};

		// encoding mode
		enum class encodeType {
			QUIET, 
			CAPTURE,
			DOUBLE_PUSH,
			EN_PASSANT
		};


		class iMove {
		public:
			void operator=(uint32_t data) noexcept {
				cmove = data;
			}

			template <iMask MASK>
			inline uint32_t getMask() noexcept {
				return (cmove & MASK);
			}
		private:
			uint32_t cmove;
		};


		// claim move item states and return encoded move item ready to save in move list

		// decide about capture flag relevancy
		inline uint32_t encodeQuietCapture(int origin, int target, bool capture) noexcept {
			return (capture << 16) | (target << 6) | origin;
		}

		// direct function template as a parameter of a specific encoding mode
		template <encodeType eT>
		uint32_t encode(int origin, int target);

		template <>
		inline uint32_t encode<encodeType::QUIET>(int origin, int target) noexcept {
			return (target << 6) | origin;
		}
		
		template <>
		inline uint32_t encode<encodeType::CAPTURE>(int origin, int target) noexcept {
			return (1 << 16) | (target << 6) | origin;
		}

		template <>
		inline uint32_t encode<encodeType::DOUBLE_PUSH>(int origin, int target) noexcept {
			return (1 << 15) | (target << 6) | origin;
		}

		template <>
		inline uint32_t encode<encodeType::EN_PASSANT>(int origin, int target) noexcept {
			return (1 << 17) | (target << 6) | origin;
		}
	}


	namespace MoveList {

		// maximum possible playable moves at one turn
		static constexpr int MAX_PLAY_MOVES = 256;

		// main move list storage array of directly defined size
		std::array<MoveItem::iMove, MAX_PLAY_MOVES> move_list;

		// count of generated moves
		int count;
	}

} // namespace


// move generation resources
namespace MoveGenerator {

	// main generation function, generating all the legal moves for all the turn-to-move pieces
	void generateLegalMoves();

} 