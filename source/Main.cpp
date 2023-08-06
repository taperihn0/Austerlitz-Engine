#include "HeaderFile.h"

int main() {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			std::cout << bitCount(relevantOccupancyBishop(i * 8 + j)) << ',';
		}
		std::cout << '\n';
	}
}