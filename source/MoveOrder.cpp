#include "MoveOrder.h"
#include <unordered_map>


namespace Order {

	int moveScore(const MoveItem::iMove& move, int ply) {
		// distinguish between quiets and captures
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim, target = move.getMask<MoveItem::iMask::TARGET>() >> 6;
			bool side = move.getMask<MoveItem::iMask::SIDE_F>();

			if (move.getMask<MoveItem::iMask::EN_PASSANT_F>())
				return MVV_LVA::lookup[PAWN][PAWN];

			// find victim piece
			for (auto pc = nBlackPawn - side; pc <= nBlackQueen; pc += 2) {
				if (getBit(BBs[pc], target)) {
					victim = toPieceType(pc);
					break;
				}
			}

			return MVV_LVA::lookup[att][victim];
		}
		else /* if quiets */ {
			// killer moves score slightly less than basic captures
			if (move == killer[0][ply])
				return 99;
			else if (move == killer[1][ply])
				return 80;
		}

		return 1;
	}
	

	void sort(MoveList& move_list, int ply) {
		std::unordered_map<uint32_t, int> hash;

		std::sort(move_list.begin(), move_list.end(), [&hash, ply](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			if (!hash[a.raw()]) hash[a.raw()] = moveScore(a, ply);
			if (!hash[b.raw()]) hash[b.raw()] = moveScore(b, ply);
			return hash[a.raw()] > hash[b.raw()];
		});
	}

} // namespace Order