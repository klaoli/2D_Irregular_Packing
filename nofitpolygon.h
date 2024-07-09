#ifndef NOFITPOLYGON_H
#define NOFITPOLYGON_H

#include "piece.h"

namespace MyNest {

	class NoFitPolygon
	{
	public:
		static NoFitPolygon* nfp;
		static NoFitPolygon* getInstance();

		polygon_t minkowskiDifNfp(const polygon_t &pA, const polygon_t &pB) const;
		polygon_t slideNfp(libnfporb::polygon_t &pA, libnfporb::polygon_t &pB) const;
		polygon_t generateIfp(const box_t &bin, const polygon_t &p) const;

	private:
		NoFitPolygon();  
		NoFitPolygon(const NoFitPolygon&) = delete;
		void operator=(const NoFitPolygon&) = delete;
	};

}


#endif // NOFITPOLYGON_H
