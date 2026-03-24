#include "parameters.h"

namespace MyNest {
	Parameters parameters;  // 定义全局变量

	box_t bin;
	std::vector<Piece> pieces;
	std::vector<std::vector<Piece>> piecesCache;
	std::unordered_map<uint64_t, polygon_t> nfpsCache;
	std::unordered_map<uint64_t, polygon_t> ifpsCache;
	std::unordered_map<uint64_t, box_t> ifrsCache;


}