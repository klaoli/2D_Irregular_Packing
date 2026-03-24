#pragma once

#include "packing.h"
#include "lbfgs.hpp"
#include <functional>

namespace MyNest
{
	using Eigen::VectorXd;

	class ILSQN
	{
	public:
		static ILSQN *ilsqn;
		static ILSQN *getInstance();
		static std::vector<Piece> lbfgsPieces;
		static std::vector<Vector> lbfgsVectors;
		static constexpr double eps = 10e-8;
		static Packing *packing;

		static double getPenetrationDepth(const Piece &p1, const Piece &p2, const Vector &v1, const Vector &v2); // 计算两个零件之间的穿透深度
		static double getPenetrationDepth(const Piece &p, const Vector &vec);
		static double getPenetrationDepth(const Piece &p1, const Piece &p2, const Vector &v1, const Vector &v2, Vector &seperateVector);
		static double getPenetrationDepth(const Piece &p, const Vector &vec, Vector &seperateVector);
		static double costFunction(void *instance, const Eigen::VectorXd &x, Eigen::VectorXd &g);
		inline int generateRandomNumber(int n);
		// 生成指定范围内的随机双精度浮点数
		inline double generateRandomDouble(double min, double max);
		double getTotalOverlap();							  // 计算当前布局总的重叠量
		double getOneTotalOverlap(const Piece &piece, const Vector &vec); // 计算piece与其他零件之间的重叠量

		void getInnerFitPolygons();	 // 依据当前currentBin,获取内靠接矩形
		double getIniaialSolution(); // 生成初始布局

		void minimizeOverlap();	   // 最小化重叠
		void movePolygon(int idx); // 移动
		void findBestPosition(int idx);
		void swapPolygons(int idx1, int idx2); // 交换两个零件的位置

		double seperate(const int N, double totalOverlap); // 分离算法
		double run();									   // 执行ilsqn

	public:
		double currentLength;
		box_t currentBin;
		std::vector<Piece> currentPieces;
		std::vector<Vector> currentVectors;

		double bestLength;
		box_t bestBin;
		std::vector<Piece> bestPieces;
		std::vector<Vector> bestVectors;

		// Qt GUI 回调：当布局更新时通知外部
		std::function<void(const box_t&, const std::vector<Piece>&, const std::vector<Vector>&, double)> onLayoutUpdated;
		bool stopRequested = false; // GUI 请求停止

	private:
		int numPieces;
		double inc;
		double dec;
		bool feasible = false;
		double allPiecesArea = 0;

		void searchBestPosition(int idx, bool useMiddlePoints); // 统一的搜索最佳位置逻辑
		void ruinAndRecreate(int k); // 大规模排样扰动策略（破坏与重建）

	private:
		ILSQN();
		ILSQN(double _inc, double _dec);
		ILSQN(const ILSQN &) = delete;
		void operator=(const ILSQN &) = delete;
	};
}
