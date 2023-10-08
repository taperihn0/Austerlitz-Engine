#include "MoveOrder.h"
#include "Zobrist.h"
#include "Search.h"


namespace Order {
	
	int moveScore(const MoveItem::iMove& move, int ply) {
		const int target = move.getMask<MoveItem::iMask::TARGET>() >> 6;

		// pv move detected
		if (PV::pv_line[ply][0] == move)
			return pv_score;
		// distinguish between quiets and captures
		else if (move.getMask<MoveItem::iMask::CAPTURE_F>()) {
			const int att = move.getMask<MoveItem::iMask::PIECE>() >> 12;
			int victim;
			const bool side = move.getMask<MoveItem::iMask::SIDE_F>();;

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
		return (scale * history_moves[pc][target]) / (butterfly[pc][target] + 1) + 1;
	}
	
	// hash table for move ordering
	static constexpr int move_coords = static_cast<int>(MoveItem::iMask::COORDS);
	static std::array<int, move_coords> m_score_hash;

	void sort(MoveList& move_list, int ply) {
		m_score_hash.fill(0);

		std::sort(move_list.begin(), move_list.end(), [ply](const MoveItem::iMove& a, const MoveItem::iMove& b) {
			if (!m_score_hash[a.raw() & move_coords]) 
				m_score_hash[a.raw() & move_coords] = moveScore(a, ply);
			if (!m_score_hash[b.raw() & move_coords]) 
				m_score_hash[b.raw() & move_coords] = moveScore(b, ply);
			return m_score_hash[a.raw() & move_coords] > m_score_hash[b.raw() & move_coords];
		});
	}

} // namespace Order
