#pragma once

#include "packing.h"
#include "lbfgs.hpp"


namespace MyNest {
	using Eigen::VectorXd;

	class ILSQN
	{
	public:
		static ILSQN* ilsqn;
		static ILSQN* getInstance();
		static std::vector<Piece> lbfgsPieces;
		static std::vector<Vector> lbfgsVectors;
		static constexpr double eps = 10e-8;

		static double getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2);	// 计算两个零件之间的穿透深度
		static double getPenetrationDepth(Piece &p, Vector &vec);
		static double getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2, Vector &seperateVector);
		static double getPenetrationDepth(Piece &p, Vector &vec, Vector &seperateVector);
		static double costFunction(void* instance, const Eigen::VectorXd &x, Eigen::VectorXd &g);
		static double costGlsFunction(void* instance, const Eigen::VectorXd &x, Eigen::VectorXd &g);

		inline int generateRandomNumber(int n);
		// 生成指定范围内的随机双精度浮点数
		inline double generateRandomDouble(double min, double max);
		double getTotalOverlap();		// 计算当前布局总的重叠量
		double getOneTotalOverlap(Piece &piece, Vector &vec);	// 计算piece与其他零件之间的重叠量
		double getIniaialSolution();	// 生成初始布局
		void minimizeOverlap();			// 最小化重叠
		void movePolygon(int idx);		// 移动
		void swapPolygons(int idx1, int idx2);	// 交换两个零件的位置
		
		double seperate(const int N, double totalOverlap); // 分离算法
		void run();		// 执行ilsqn
		
	public:
		double currentLength;
		box_t currentBin;
		std::vector<Piece> currentPieces;	
		std::vector<Vector> currentVectors;

		double bestLength;
		box_t bestBin;
		std::vector<Piece> bestPieces;		
		std::vector<Vector> bestVectors;

	private:
		int numPieces;
		double inc;
		double dec;
		bool feasible = false;
		double allPiecesArea = 0;
		
	private:
		ILSQN();
		ILSQN(double _inc, double _dec);
		ILSQN(const ILSQN& ) = delete;
		void operator=(const ILSQN& ) = delete;


	public:
		// static std::vector<std::vector<double>> miuMatrix;
		// std::vector<std::vector<double>> overlapMatrix;
		// void resetMiu();
		// void updateMiu(double maxOverlap);
		// double getGlsOneOverlap(Piece &piece, Vector &vec, int idx);
		// double getGlsTotalOverlap();
		// void moveGlsPolygon(int idx);
		// void minimizeGlsOverlap(); 
		// double seperateGls(const int N, double currentOverlap);
		// double getMaxOverlap();
	};
}


