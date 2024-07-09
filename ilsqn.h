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

		static double getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2);	// ��͸����������ص���
		static double getPenetrationDepth(Piece &p, Vector &vec);
		static double getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2, Vector &seperateVector);
		static double getPenetrationDepth(Piece &p, Vector &vec, Vector &seperateVector);
		static double costFunction(void* instance, const Eigen::VectorXd &x, Eigen::VectorXd &g);
		static double costGlsFunction(void* instance, const Eigen::VectorXd &x, Eigen::VectorXd &g);

		inline int generateRandomNumber(int n);
		double getTotalOverlap();		// ��ȡ��ǰ���ֵ��ܵ��ص���
		double getOneTotalOverlap(Piece &piece, Vector &vec);	// ��ȡһ���������������������ص���
		double getIniaialSolution();	// ����һ����ʼ��
		void minimizeOverlap();			// ��С���ص�
		void movePolygon(int idx);		// �ڵ�ǰ�����У�ѡȡ��idx��������ƶ������ص�����С��λ��
		void swapPolygons(int idx1, int idx2);
		
		double seperate(const int N, double totalOverlap); // �����㷨
		void run();		// ʹ�÷������Ż��㷨�Ż���ǰ����
		
	public:
		double currentLength;
		box_t currentBin;
		std::vector<Piece> currentPieces;	// ��ǰ��
		std::vector<Vector> currentVectors;

		double bestLength;
		box_t bestBin;
		std::vector<Piece> bestPieces;		// ��ý�
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
		static std::vector<std::vector<double>> miuMatrix;
		std::vector<std::vector<double>> overlapMatrix;
		void resetMiu();
		void updateMiu(double maxOverlap);
		double getGlsOneOverlap(Piece &piece, Vector &vec, int idx);
		double getGlsTotalOverlap();
		void moveGlsPolygon(int idx);
		void minimizeGlsOverlap(); 
		double seperateGls(const int N, double currentOverlap);
		double getMaxOverlap();
		





	};
}


