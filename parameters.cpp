#include "parameters.h"

namespace MyNest {
	Parameters parameters;  // 定义全局变量

	box_t bin;
	std::vector<Piece> pieces;
	std::vector<std::vector<Piece>> piecesCache;
	std::unordered_map<std::string, polygon_t> nfpsCache;
	std::unordered_map<std::string, polygon_t> ifpsCache;
	std::unordered_map<std::string, box_t> ifrsCache;


}