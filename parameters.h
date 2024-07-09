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
	struct Parameters {
		// double ת Int, �������Ϊ��������С���ű�����clipper �����ĵ��ö���Ҫ�˸ñ���
		static constexpr int scaleRate = 1000;

		// ���߿�ˡ��ɾ����߾��� curveTolerance ���ڵ�����㣬���͹����λ�����������
		static constexpr double curveTolerance = 0.025;
		
		// ���� max double
		static constexpr double MAXDOUBLE = std::numeric_limits<double>::max();
		
		double minGap = 0;			// �����࣬Ĭ��Ϊ0
		double polygonScaleRate;	// ����ηŴ���
		std::string piecePath;		// ������ݵ�·��
		std::string nfpsPath;		// nfp·��
		std::string ifpsPath;		// ifr·��
		std::string resultPath;		 // ���ֽ�����·��
		double maxRunTime;			 // �㷨�������ʱ��
		double maxIteration;
		size_t orientations;		 // �������ת������

		double inc;		// ����ÿ�����ӵı���
		double dec;		// ����ÿ�μ��ٵı���
	};


	extern Parameters parameters;
	extern box_t bin;
	extern std::vector<Piece> pieces;
	extern std::vector<std::vector<Piece>> piecesCache;
	extern std::unordered_map<std::string, polygon_t> nfpsCache;
	extern std::unordered_map<std::string, polygon_t> ifpsCache;
	extern std::unordered_map<std::string, box_t> ifrsCache;

	inline std::string getNfpKey(const Piece &A, const Piece &B) {
		return std::to_string(A.typeId) + "_" + std::to_string(A.rotation) + "-" +
			std::to_string(B.typeId) + "-" + std::to_string(B.rotation);
	}

	inline std::string getIfrKey(const Piece &A) {
		return std::to_string(A.typeId) + "_" + std::to_string(A.rotation);
	}

}

#endif // PARAMETERS_H
