#ifndef DATAWRITE_H
#define DATAWRITE_H
#include "piece.h"
#include <unordered_map>

namespace MyNest {
	class DataWrite
	{
	public:
		static DataWrite *datawriter;
		static DataWrite* getInstance();

		void writeNfps(const std::unordered_map<std::string, polygon_t>& nfps, std::string &filePath) const;
		void writePackingResult(const box_t &usedBin, const std::vector<Piece> &pieces) const;

		void plotPiece(Piece &piece) const;
		void plotPieces(std::vector<Piece> &pieces) const;
		void plotPieces(box_t &bin, std::vector<Piece> &pieces) const;
		void plotPieces(box_t &bin, std::vector<Piece> pieces, std::vector<Vector> &vectors) const;

		void plotPolygon(polygon_t &polygon) const;
		void plotPolygons(std::vector<polygon_t> &polygons) const;

		void plotNoFitPolygon(Piece &pieceA, Piece &pieceB) const;
		void plotNoFitPolygon(polygon_t &polyA, polygon_t &polyB) const;

		void plotInnerFitPolygon(box_t &bin, polygon_t &polygon)const;
		void plotInnerFitPolygon(box_t &bin, Piece &piece) const;

	private:
		DataWrite();
		DataWrite(const DataWrite&) = delete;
		void operator=(const DataWrite&) = delete;
	};

}

#endif // DATAWRITE_H
