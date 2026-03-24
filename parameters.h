#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <vector>
#include <string>
#include <limits>
#include <cstdint>
#include <unordered_map>

#include "geometry.h"
#include "piece.h"

namespace MyNest
{
	struct Parameters
	{
		// double 转 Int, 将坐标变为整数的最小缩放倍数，clipper 函数的调用都需要乘该倍数
		static constexpr int scaleRate = 1000;

		// 曲线宽恕，删除与边距离 curveTolerance 以内的坐标点，如果凸多边形会造成面积减少
		static constexpr double curveTolerance = 0.025;

		// 常数 max double
		static constexpr double MAXDOUBLE = std::numeric_limits<double>::max();

		double minGap = 0;		 // 零件间距，默认为0
		double polygonScaleRate; // 多边形放大倍数
		std::string piecePath;	 // 零件数据的路径
		std::string nfpsPath;	 // nfp路径
		std::string ifpsPath;	 // ifr路径
		std::string resultPath;	 // 布局结果存放路径
		double maxRunTime;		 // 算法最大运行时间
		double maxIteration;
		size_t orientations; // 多边形旋转方向数

		double inc; // 板子每次增加的比率
		double dec; // 板子每次减少的比率
	};

	extern Parameters parameters;
	extern box_t bin;
	extern std::vector<Piece> pieces;
	extern std::vector<std::vector<Piece>> piecesCache;
	extern std::unordered_map<uint64_t, polygon_t> nfpsCache;
	extern std::unordered_map<uint64_t, polygon_t> ifpsCache;
	extern std::unordered_map<uint64_t, box_t> ifrsCache;

	inline uint64_t getNfpKey(const Piece &A, const Piece &B)
	{
		// 高32位编码A(typeId<<16 | rotation), 低32位编码B(typeId<<16 | rotation)
		uint32_t hi = (static_cast<uint32_t>(A.typeId) << 16) | static_cast<uint32_t>(static_cast<int>(A.rotation));
		uint32_t lo = (static_cast<uint32_t>(B.typeId) << 16) | static_cast<uint32_t>(static_cast<int>(B.rotation));
		return (static_cast<uint64_t>(hi) << 32) | lo;
	}

	inline uint64_t getIfrKey(const Piece &A)
	{
		return (static_cast<uint32_t>(A.typeId) << 16) | static_cast<uint32_t>(static_cast<int>(A.rotation));
	}

}

#endif // PARAMETERS_H
