#ifndef PACKING_H
#define PACKING_H

#include <vector>
#include "piece.h"
#include "vector.h"

namespace MyNest {


	class Packing
	{
	public:
		static Packing* packing;
		static Packing* getInstance();
		
		void preprocess();
		int checkNfps();
		int checkIfps();

		point_t findMostLeftPoint(std::vector<ring_t> &rings);
		double run(std::vector<Piece> &placedPieces, std::vector<Vector> &placedVectors);

	private:
		Packing();
		Packing(const Packing&) = delete;
		void operator=(const Packing&) = delete;
	};
}


#endif // NEST_H

