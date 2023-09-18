#include "MoveOrder.h"
#include <unordered_map>


namespace Order {

	int moveScore(const MoveItem::iMove& move) {
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
		//else /* if quiets */ 

		return 1;
	}
	

	void sort(MoveList& move_list) {
		std::unordered_map<uint32_t, int> hash;

		std::sort(move_list.begin(), move_list.end(), [&hash](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			if (!hash[a.raw()]) hash[a.raw()] = moveScore(a);
			if (!hash[b.raw()]) hash[b.raw()] = moveScore(b);
			return hash[a.raw()] > hash[b.raw()];
		});
	}

} // namespace Order