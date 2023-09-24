#include "MoveOrder.h"
#include "Zobrist.h"
#include <unordered_map>


namespace Order {
	
	int moveScore(const MoveItem::iMove& move, int ply) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		if (tt.read<ReadType::ONLY_BESTMOVE>(0, 0, 0) == move)
			return 1500000;

		// distinguish between quiets and captures
		if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
			const bool side = move.getMask<MoveItem::iMask::SIDE_F>();

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
		
		// killer moves score less than basic captures
		if (move == killer[0][ply])
			return 900000;
		else if (move == killer[1][ply])
			return 800000;

		// relative history move score
		const int pc = move.getMask<MoveItem::iMask::PIECE>() >> 12;
		static constexpr int scale = 13;
		return (scale * history_moves[pc][target]) / butterfly[pc][target] + 1;
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
