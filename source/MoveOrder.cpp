#include "MoveOrder.h"
#include <unordered_map>


namespace Order {

	int moveScore(const MoveItem::iMove& move, int ply) {
		int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// distinguish between quiets and captures
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
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

			return MVV_LVA::lookup[att][victim] + 1000000;
		}
		
		// killer moves score slightly less than basic captures
		if (move == killer[0][ply])
			return 900000;
		else if (move == killer[1][ply])
			return 800000;

		int from = move.getMask<MoveItem::iMask::ORIGIN>();
		return (20 * history_moves[game_state.turn][from][target]) / butterfly[game_state.turn][from][target] + 1;
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