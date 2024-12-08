#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <vector>
#include <string>
#include <limits>
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
	extern std::unordered_map<std::string, polygon_t> nfpsCache;
	extern std::unordered_map<std::string, polygon_t> ifpsCache;
	extern std::unordered_map<std::string, box_t> ifrsCache;

	inline std::string getNfpKey(const Piece &A, const Piece &B)
	{
		return std::to_string(A.typeId) + "_" + std::to_string(A.rotation) + "-" +
			   std::to_string(B.typeId) + "-" + std::to_string(B.rotation);
	}

	inline std::string getIfrKey(const Piece &A)
	{
		return std::to_string(A.typeId) + "_" + std::to_string(A.rotation);
	}

}

#endif // PARAMETERS_H
