#include "ilsqn.h"
#include "dataloader.h"
#include "parameters.h"
#include "nofitpolygon.h"
#include "datawriter.h"

#include <eigen3/Eigen/Core>
#include <random>
#include <omp.h>
#include <chrono>

using namespace MyNest;
using Eigen::VectorXd;

ILSQN *ILSQN::ilsqn = nullptr;
std::vector<Piece> ILSQN::lbfgsPieces;
std::vector<Vector> ILSQN::lbfgsVectors;
// std::vector<std::vector<double>> ILSQN::miuMatrix;

ILSQN::ILSQN(double _inc, double _dec)
{
	inc = _inc;
	dec = _dec;
	numPieces = pieces.size();
	for (auto &piece : pieces)
	{
		allPiecesArea += piece.area;
	}
	// miuMatrix.resize(numPieces+1, std::vector<double>(numPieces+1,1));
	// overlapMatrix.resize(numPieces+1, std::vector<double>(numPieces+1,0));
}

ILSQN *ILSQN::getInstance()
{

	if (ilsqn == nullptr)
	{
		ilsqn = new ILSQN(parameters.inc, parameters.dec);
	}
	return ilsqn;
}

inline bool boxIsOverlap(box_t &b1, box_t &b2)
{
	return (std::min(b1.max_corner().x(), b2.max_corner().x()) > std::max(b1.min_corner().x(), b2.min_corner().x()) &&
			std::min(b1.max_corner().y(), b2.max_corner().y()) > std::max(b1.min_corner().y(), b2.min_corner().y()));
}

inline Vector shortestTranslationVector(point_t &p, segment_t &s) {
	Vector p1(s.first.x(), s.first.y());
	Vector p2(s.second.x(), s.second.y());
	Vector p0(p.x(), p.y());

	Vector v = p2 - p1; // 线段方向向量
	Vector w = p0 - p1; // 从p1到点p的向量

	double c1 = w * v; // 点p在方向向量v上的投影长度
	if (c1 <= 0) {
		return p1 - p0;	// 投影点在p1之前，最近点是p1
	}

	double c2 = v * v; // 方向向量v的长度平方
	if (c2 <= c1) {
		return p2 - p0; // 投影点在p2之后，最近点是p2
	}

	// 投影点在线段上
	double b = c1 / c2;
	Vector pb = p1 + v * b; // 投影点
	return pb - p0;
}

// 计算点到矩形的最短距离
inline double pointToRectangleDistance(const point_t &p, const box_t &rect)
{
	double dx = std::max(rect.min_corner().x() - p.x(), 0.0);
	dx = std::max(dx, p.x() - rect.max_corner().x());

	double dy = std::max(rect.min_corner().y() - p.y(), 0.0);
	dy = std::max(dy, p.y() - rect.max_corner().y());

	return dx > 0 || dy > 0 ? dx * dx + dy * dy : 0.0;
}

//
double ILSQN::getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2)
{
	double overlap = 0.0;
	box_t box1(point_t(p1.bounding.min_corner().x() + v1.x, p1.bounding.min_corner().y() + v1.y),
			   point_t(p1.bounding.max_corner().x() + v1.x, p1.bounding.max_corner().y() + v1.y));
	box_t box2(point_t(p2.bounding.min_corner().x() + v2.x, p2.bounding.min_corner().y() + v2.y),
			   point_t(p2.bounding.max_corner().x() + v2.x, p2.bounding.max_corner().y() + v2.y));

	if (boxIsOverlap(box1, box2))
	{ // //判断两个多边形是否有相交的可能，可能的话返回真
		std::string key = getNfpKey(p1, p2);
		polygon_t nfp = nfpsCache[key];
		point_t referPoint(v2.x - v1.x, v2.y - v1.y); // v2也可表达参考点坐标，将参考点沿v1的反方向平移可减少计算量

		if (bg::within(referPoint, nfp))
		{
			double distance = Parameters::MAXDOUBLE;

			for (int i = 0; i < nfp.outer().size() - 1; ++i)
			{
				double tempDistance;
				segment_t segment(nfp.outer()[i], nfp.outer()[i + 1]);
				Vector tempVector = shortestTranslationVector(referPoint, segment);
				tempDistance = tempVector * tempVector;
				if (tempDistance < distance)
				{
					distance = tempDistance;
				}
			}
			return distance;
		}
	}
	return overlap;
}

double ILSQN::getPenetrationDepth(Piece &p, Vector &vec)
{
	std::string key = getIfrKey(p);
	box_t ifr = ifrsCache[key];
	point_t referPoint(vec.x, vec.y);
	return pointToRectangleDistance(referPoint, ifr);
}

double ILSQN::getPenetrationDepth(Piece &p1, Piece &p2, Vector &v1, Vector &v2, Vector &seperateVector)
{
	double overlap = 0.0;
	box_t box1(point_t(p1.bounding.min_corner().x() + v1.x, p1.bounding.min_corner().y() + v1.y),
			   point_t(p1.bounding.max_corner().x() + v1.x, p1.bounding.max_corner().y() + v1.y));
	box_t box2(point_t(p2.bounding.min_corner().x() + v2.x, p2.bounding.min_corner().y() + v2.y),
			   point_t(p2.bounding.max_corner().x() + v2.x, p2.bounding.max_corner().y() + v2.y));

	if (boxIsOverlap(box1, box2))
	{ // 判断两个多边形是否有相交的可能，可能的话返回真
		std::string key = getNfpKey(p1, p2);
		polygon_t nfp = nfpsCache[key];
		point_t referPoint(v2.x - v1.x, v2.y - v1.y); // v2也可表达参考点坐标，将参考点沿v1的反方向平移可减少计算量

		if (bg::within(referPoint, nfp))
		{
			double distance = Parameters::MAXDOUBLE;

			for (int i = 0; i < nfp.outer().size() - 1; ++i)
			{
				double tempDistance;
				segment_t segment(nfp.outer()[i], nfp.outer()[i + 1]);
				Vector tempVector = shortestTranslationVector(referPoint, segment);
				tempDistance = tempVector * tempVector;
				if (tempDistance < distance)
				{
					distance = tempDistance;
					seperateVector = tempVector;
				}
			}
			return distance;
		}
	}
	seperateVector.x = 0, seperateVector.y = 0;
	return overlap;
}

double ILSQN::getPenetrationDepth(Piece &p, Vector &v, Vector &seperateVector)
{
	std::string key = getIfrKey(p);
	box_t ifr = ifrsCache[key];
	polygon_t ifp = ifpsCache[key];
	point_t referPoint(v.x, v.y);
	// 如果点在矩形内部，返回零向量
	if (referPoint.x() >= ifr.min_corner().x() && referPoint.x() <= ifr.max_corner().x() &&
		referPoint.y() >= ifr.min_corner().y() && referPoint.y() <= ifr.max_corner().y())
	{
		seperateVector.x = 0, seperateVector.y = 0;
		return 0.0;
	}
	double minDistanceSquared = Parameters::MAXDOUBLE;
	// 遍历矩形的每一条边，找到最短的平移向量
	for (int i = 0; i < ifp.outer().size() - 1; ++i)
	{
		segment_t segment(ifp.outer()[i], ifp.outer()[i + 1]);
		Vector translation = shortestTranslationVector(referPoint, segment);
		double distanceSquared = translation * translation;
		if (distanceSquared < minDistanceSquared)
		{
			minDistanceSquared = distanceSquared;
			seperateVector = translation;
		}
	}
	return minDistanceSquared;
}

double ILSQN::getTotalOverlap()
{ // 计算整个布局的重叠量
	double ret = 0.0;
	for (int i = 0; i < currentPieces.size(); ++i)
	{
		ret += getPenetrationDepth(currentPieces[i], currentVectors[i]);

		for (int j = i + 1; j < currentPieces.size(); ++j)
		{
			ret += getPenetrationDepth(currentPieces[i], currentPieces[j], currentVectors[i], currentVectors[j]);
		}
	}
	return ret;
}

double ILSQN::getOneTotalOverlap(Piece &piece, Vector &vec)
{
	double ret = 0.0;
	ret += getPenetrationDepth(piece, vec);
	for (int i = 0; i < lbfgsPieces.size(); ++i)
	{
		ret += getPenetrationDepth(lbfgsPieces[i], piece, lbfgsVectors[i], vec);
	}
	return ret;
}

double ILSQN::costFunction(void *instance, const Eigen::VectorXd &x, Eigen::VectorXd &grad)
{
	double ret = 0.0;
	static int numPieces = x.size() / 2;
	static std::vector<Vector> vectors(numPieces);

	for (int i = 0, j = 0; i < x.size(); i += 2, ++j)
	{
		vectors[j].x = x(i);
		vectors[j].y = x(i + 1);
	}
	grad = VectorXd::Zero(x.size());
	Vector seperateVec;
	for (int i = 0; i < numPieces; ++i)
	{
		ret += (getPenetrationDepth(lbfgsPieces[i], vectors[i], seperateVec));
		grad[2 * i] += (-2 * seperateVec.x);
		grad[2 * i + 1] += (-2 * seperateVec.y);

		for (int j = i + 1; j < numPieces; ++j)
		{
			ret += (getPenetrationDepth(lbfgsPieces[j], lbfgsPieces[i], vectors[j], vectors[i], seperateVec));
			grad[2 * i] += (-2 * seperateVec.x);
			grad[2 * i + 1] += (-2 * seperateVec.y);
			grad[2 * j] += (2 * seperateVec.x);
			grad[2 * j + 1] += (2 * seperateVec.y);
		}
	}
	return ret;
}

double ILSQN::seperate(const int N, double currentOverlap)
{
	double finalCost;
	Eigen::VectorXd x(N);

	/* Set the initial guess */
	for (int i = 0, j = 0; i < N; i += 2, j++)
	{
		x(i) = lbfgsVectors[j].x;
		x(i + 1) = lbfgsVectors[j].y;
	}

	/* Set the minimization parameters */
	lbfgs::lbfgs_parameter_t params;
	params.g_epsilon = 1.0e-8;
	params.past = 6;
	params.delta = 1.0e-8;

	/* Start minimization */
	int ret = lbfgs::lbfgs_optimize(x,
									finalCost,
									costFunction,
									nullptr,
									nullptr,
									this,
									params);

	if (currentOverlap > finalCost)
	{
		for (int i = 0, j = 0; i < x.size(); i += 2, ++j)
		{
			lbfgsVectors[j].x = x(i);
			lbfgsVectors[j].y = x(i + 1);
		}
	}
	return finalCost;
}

void ILSQN::movePolygon(int idx)
{
	std::vector<Vector> vecVectors(parameters.orientations);
	std::vector<double> overlaps(parameters.orientations, parameters.MAXDOUBLE);

#pragma omp parallel for num_threads(parameters.orientations)
	for (int k = 0; k < parameters.orientations; ++k)
	{
		Piece &piece = piecesCache[k][idx];
		std::vector<polygon_t> nfps;
		std::string key = getIfrKey(piece);
		nfps.push_back(ifpsCache[key]);
		for (int i = 0; i < lbfgsPieces.size(); ++i)
		{
			std::string key = getNfpKey(lbfgsPieces[i], piece);
			const polygon_t &nfp = nfpsCache[key];
			polygon_t transNfp;
			bg::strategy::transform::translate_transformer<double, 2, 2> translate(lbfgsVectors[i].x, lbfgsVectors[i].y);
			bg::transform(nfp, transNfp, translate);
			nfps.push_back(transNfp);
		}

		std::vector<point_t> middlePoints;
		for (int i = 0; i < nfps.size(); ++i)
		{
			for (int j = 0; j < nfps[i].outer().size() - 1; ++j)
			{
				Vector vec(nfps[i].outer()[j].x(), nfps[i].outer()[j].y());
				double overlap = getOneTotalOverlap(piece, vec);
				if (overlap < overlaps[k])
				{
					overlaps[k] = overlap;
					vecVectors[k] = vec;
				}
				point_t p(
					(nfps[i].outer()[j].x() + nfps[i].outer()[j + 1].x()) / 2,
					(nfps[i].outer()[j].y() + nfps[i].outer()[j + 1].y()) / 2);
				middlePoints.push_back(p);
			}
		}
		for (int i = 0; i < middlePoints.size(); ++i)
		{
			Vector vec(middlePoints[i].x(), middlePoints[i].y());
			double overlap = getOneTotalOverlap(piece, vec);
			if (overlap < overlaps[k])
			{
				overlaps[k] = overlap;
				vecVectors[k] = vec;
			}
		}
	}
	int index = std::min_element(overlaps.begin(), overlaps.end()) - overlaps.begin();
	lbfgsPieces[idx] = piecesCache[index][idx];
	lbfgsVectors[idx] = vecVectors[index];
}

void ILSQN::swapPolygons(int idx1, int idx2)
{
	lbfgsVectors[idx2].x = Parameters::MAXDOUBLE, lbfgsVectors[idx2].y = Parameters::MAXDOUBLE;
	// FindBestPosition(idx1);
	// FindBestPosition(idx2);
	movePolygon(idx1);
	movePolygon(idx2);
}

int ILSQN::generateRandomNumber(int n)
{
	static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
	std::uniform_int_distribution<int> distribution(0, n - 1);
	return distribution(rng);
}

double ILSQN::generateRandomDouble(double min, double max)
{
	std::random_device rd;								  // 随机数种子，仅用于种子生成
	std::mt19937 gen(rd());								  // Mersenne Twister 生成器
	std::uniform_real_distribution<double> dis(min, max); // 均匀分布

	return dis(gen); // 返回生成的随机数
}

void ILSQN::minimizeOverlap()
{
	double totalOverlap = getTotalOverlap(); // 计算当前布局总的重叠量
	// std::cout << "TotalOverlap = " << totalOverlap << std::endl;
	int iter = 0;
	while (iter++ < parameters.maxIteration)
	{
		int i = generateRandomNumber(numPieces),
			j = generateRandomNumber(numPieces);

		while (i == j)
		{
			j = generateRandomNumber(numPieces);
		}

		swapPolygons(i, j);

		double tempTotalOverlap = seperate(numPieces * 2, totalOverlap);

		if (tempTotalOverlap < totalOverlap)
		{
			totalOverlap = tempTotalOverlap;
			currentPieces[i] = lbfgsPieces[i];
			currentPieces[j] = lbfgsPieces[j];
			currentVectors = lbfgsVectors;
			// iter = 0;
		}
		else
		{
			lbfgsPieces[i] = currentPieces[i];
			lbfgsPieces[j] = currentPieces[j];
			lbfgsVectors[i] = currentVectors[i];
			lbfgsVectors[j] = currentVectors[j];
		}

		if (totalOverlap < eps)
		{
			feasible = true;
			break;
		}
		// std::cout << "当前总的重叠量 = " << totalOverlap << std::endl;
	}
}

double ILSQN::getIniaialSolution()
{
	Packing *packing = Packing::getInstance();
	packing->preprocess();
	packing->checkIfps();
	packing->checkNfps();
	currentBin = bin;
	currentPieces = piecesCache[0];
	std::vector<Piece> placedPieces;
	return packing->run(placedPieces, currentVectors);
}

void ILSQN::run()
{
	currentLength = getIniaialSolution(); // 生成初始布局
	currentBin.max_corner().set<0>(currentLength);
	std::cout << "初始利用率为 = " << allPiecesArea / bg::area(currentBin) << std::endl;

	bestPieces = currentPieces;
	bestVectors = currentVectors;
	bestBin = currentBin;
	bestLength = currentLength;

	currentLength = (1 - dec) * currentLength; // 按比例缩短板材边界
	currentBin.max_corner().set<0>(currentLength);

	lbfgsPieces = currentPieces;
	lbfgsVectors = currentVectors;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	auto end = start;
	std::chrono::duration<double> time_taken = end - start;
	static NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();

	while (time_taken.count() < parameters.maxRunTime)
	{

		for (int i = 0; i < piecesCache.size(); ++i)
		{
			for (int j = 0; j < piecesCache[i].size(); ++j)
			{
				std::string key = getIfrKey(piecesCache[i][j]);
				polygon_t ifp = nfpGenerator->generateIfp(currentBin, piecesCache[i][j].polygon);
				ifpsCache[key] = ifp;
				box_t ifr;
				bg::envelope(ifp, ifr);
				ifrsCache[key] = ifr;
			}
		}
		minimizeOverlap(); // 执行最小化重叠
		// minimizeGlsOverlap();
		if (feasible)
		{
			std::cout << "当前利用率 = " << allPiecesArea / bg::area(currentBin) << std::endl;
			static DataWrite *datawriter = DataWrite::getInstance();
			datawriter->plotPieces(currentBin, currentPieces, currentVectors);

			bestPieces = currentPieces;
			bestVectors = currentVectors;
			bestBin = currentBin;
			bestLength = currentLength;

			currentLength = (1 - dec) * currentLength; // 缩减板材的长度
			currentBin.max_corner().set<0>(currentLength);
			feasible = false;
		}
		else
		{
			currentLength = (1 + inc) * currentLength;
			if (currentLength >= bestLength)
			{
				currentLength = (1 - dec) * bestLength;
				currentVectors = bestVectors;
				currentPieces = bestPieces;
			}
			currentBin.max_corner().set<0>(currentLength);
		}
		end = std::chrono::steady_clock::now();
		time_taken = end - start;
	}
	std::cout << "达到最大搜索时间，最好利用率 = " << allPiecesArea / bg::area(bestBin) << std::endl;
}

// void ILSQN::moveGlsPolygon(int idx) {
// 	std::vector<Vector> vecVectors(parameters.orientations);
// 	std::vector<double> glsOverlaps(parameters.orientations, parameters.MAXDOUBLE);

// #pragma omp parallel for num_threads(parameters.orientations)
// 	for (int k = 0; k < parameters.orientations; ++k) {
// 		Piece &piece = piecesCache[k][idx];
// 		std::vector<polygon_t> nfps;
// 		std::string key = getIfrKey(piece);
// 		nfps.push_back(ifpsCache[key]);
// 		for (int i = 0; i < lbfgsPieces.size(); ++i) {
// 			std::string key = getNfpKey(lbfgsPieces[i], piece);
// 			const polygon_t &nfp = nfpsCache[key];
// 			polygon_t transNfp;
// 			bg::strategy::transform::translate_transformer<double, 2, 2> translate(lbfgsVectors[i].x, lbfgsVectors[i].y);
// 			bg::transform(nfp, transNfp, translate);
// 			nfps.push_back(transNfp);
// 		}

// 		std::vector<point_t> middlePoints;
// 		for (int i = 0; i < nfps.size(); ++i) {
// 			for (int j = 0; j < nfps[i].outer().size() - 1; ++j) {
// 				Vector vec(nfps[i].outer()[j].x(), nfps[i].outer()[j].y());
// 				// double overlap = getOneTotalOverlap(piece, vec);
// 				double glsOverlap = getGlsOneOverlap(piece, vec, idx);
// 				if (glsOverlap < glsOverlaps[k]) {
// 					glsOverlaps[k] = glsOverlap;
// 					vecVectors[k] = vec;
// 				}
// 				point_t p(
// 					(nfps[i].outer()[j].x() + nfps[i].outer()[j + 1].x()) / 2,
// 					(nfps[i].outer()[j].y() + nfps[i].outer()[j + 1].y()) / 2);
// 				middlePoints.push_back(p);
// 			}
// 		}
// 		for (int i = 0; i < middlePoints.size(); ++i) {
// 			Vector vec(middlePoints[i].x(), middlePoints[i].y());
// 			// double overlap = getOneTotalOverlap(piece, vec);
// 			double glsOverlap = getGlsOneOverlap(piece, vec, idx);
// 			if (glsOverlap < glsOverlaps[k]) {
// 				glsOverlaps[k] = glsOverlap;
// 				vecVectors[k] = vec;
// 			}
// 		}
// 	}
// 	int index = std::min_element(glsOverlaps.begin(), glsOverlaps.end()) - glsOverlaps.begin();
// 	lbfgsPieces[idx] = piecesCache[index][idx];
// 	lbfgsVectors[idx] = vecVectors[index];
// }

// double ILSQN::costGlsFunction(void* instance, const Eigen::VectorXd &x, Eigen::VectorXd &grad) {
// 	double ret = 0.0;
// 	static int numPieces = x.size() / 2;
// 	static std::vector<Vector> vectors(numPieces);

// 	for (int i = 0, j = 0; i < x.size(); i += 2, ++j) {
// 		vectors[j].x = x(i);
// 		vectors[j].y = x(i + 1);
// 	}
// 	grad = VectorXd::Zero(x.size());
// 	Vector seperateVec;
// 	for (int i = 0; i < numPieces; ++i) {
// 		ret += (getPenetrationDepth(lbfgsPieces[i], vectors[i], seperateVec) * miuMatrix[i+1][0]);
// 		grad[2 * i] += ((-2 * seperateVec.x) * miuMatrix[i+1][0]);
// 		grad[2 * i + 1] += (-2 * seperateVec.y * miuMatrix[i+1][0]);

// 		for (int j = i + 1; j < numPieces; ++j) {
// 			ret += (getPenetrationDepth(lbfgsPieces[j], lbfgsPieces[i], vectors[j], vectors[i], seperateVec) * miuMatrix[j+1][i+1]);
// 			grad[2 * i] += (-2 * seperateVec.x)  * miuMatrix[j+1][i+1];
// 			grad[2 * i + 1] += (-2 * seperateVec.y) * miuMatrix[j+1][i+1];
// 			grad[2 * j] += (2 * seperateVec.x) * miuMatrix[j+1][i+1];
// 			grad[2 * j + 1] += (2 * seperateVec.y) * miuMatrix[j+1][i+1];
// 		}
// 	}
// 	return ret;
// }

// double ILSQN::seperateGls(const int N, double currentOverlap) {
// 	double finalCost;
// 	Eigen::VectorXd x(N);

// 	/* Set the initial guess */
// 	for (int i = 0, j = 0; i < N; i += 2, j++) {
// 		x(i) = lbfgsVectors[j].x;
// 		x(i + 1) = lbfgsVectors[j].y;
// 	}

// 	/* Set the minimization parameters */
// 	lbfgs::lbfgs_parameter_t params;
// 	params.g_epsilon = 1.0e-8;
// 	params.past = 6;
// 	params.delta = 1.0e-8;

// 	/* Start minimization */
// 	int ret = lbfgs::lbfgs_optimize(x,
// 		finalCost,
// 		costGlsFunction,
// 		nullptr,
// 		nullptr,
// 		this,
// 		params);

// 	if (currentOverlap > finalCost) {
// 		for (int i = 0, j = 0; i < x.size(); i += 2, ++j) {
// 			lbfgsVectors[j].x = x(i);
// 			lbfgsVectors[j].y = x(i + 1);
// 		}
// 	}
// 	return finalCost;
// }

// void ILSQN::minimizeGlsOverlap() {
// 	// double totalOverlap = getTotalOverlap();	// ��ǰ�����ܵ��ص���
// 	// std::cout << "TotalOverlap = " << totalOverlap << std::endl;
// 	resetMiu();
// 	// for(int i = 0;i < miuMatrix.size();++i){
// 	// 	for(int j = 0;j<miuMatrix.size();++j) {
// 	// 		std::cout << miuMatrix[i][j] <<",";
// 	// 	}
// 	// 	std::cout<<std::endl;
// 	// }
// 	double glsTotalOverlap = getGlsTotalOverlap();
// 	// std::cout << "glsTotalOverlap = " << glsTotalOverlap << std::endl;

// 	int iter = 0;
// 	while (iter++ < parameters.maxIteration) {
// 		int i = generateRandomNumber(numPieces),
// 			j = generateRandomNumber(numPieces);

// 		while (i == j) {
// 			j = generateRandomNumber(numPieces);
// 		}
// 		swapPolygons(i, j);
// 		double tempGlsTotalOverlap = seperateGls(numPieces * 2, glsTotalOverlap);
// 		// std::cout << "tempGlsTotalOverlap = " << glsTotalOverlap << std::endl;

// 		if (tempGlsTotalOverlap < glsTotalOverlap) {
// 			glsTotalOverlap = tempGlsTotalOverlap;
// 			currentPieces[i] = lbfgsPieces[i];
// 			currentPieces[j] = lbfgsPieces[j];
// 			currentVectors = lbfgsVectors;
// 			iter = 0;
// 		}
// 		else {
// 			lbfgsPieces[i] = currentPieces[i];
// 			lbfgsPieces[j] = currentPieces[j];
// 			lbfgsVectors[i] = currentVectors[i];
// 			lbfgsVectors[j] = currentVectors[j];
// 		}

// 		if (glsTotalOverlap < eps) {
// 			feasible = true;
// 			break;
// 		}
// 		// update miuMatrix
// 		double maxOverlap = getMaxOverlap();
// 		updateMiu(maxOverlap);
// 		// for(int i = 0;i < miuMatrix.size();++i){
// 		// 	for(int j = 0;j<miuMatrix.size();++j) {
// 		// 		std::cout << miuMatrix[i][j] <<",";
// 		// 	}
// 		// 	std::cout<<std::endl;
// 		// }
// 	}
// }

// void ILSQN::resetMiu() {
// 	// 将所有元素设置为1
//     for (int i = 0; i < miuMatrix.size(); ++i) {
//         for (int j = 0; j < miuMatrix[i].size(); ++j) {
//             miuMatrix[i][j] = 1.0;
//         }
//     }
// }

// void ILSQN::updateMiu(double maxOverlap) {
// 	for (int i = 1; i < miuMatrix.size(); ++i) {
// 		miuMatrix[i][0] = miuMatrix[0][i] = (overlapMatrix[i][0] / maxOverlap + miuMatrix[i][0]);
// 		for (int j = 1; j < i; ++j) {
// 			miuMatrix[i][j] = miuMatrix[j][i] = (overlapMatrix[i][j] / maxOverlap + miuMatrix[i][j]);
// 		}
// 	}
// }

// double ILSQN::getGlsOneOverlap(Piece &piece, Vector &vec, int idx) {
// 	double ret = 0;
// 	int j = idx+1;
// 	ret += miuMatrix[0][j] * getPenetrationDepth(piece, vec);
// 	for (int i = 0; i < currentPieces.size(); ++i) {
// 		ret += miuMatrix[i + 1][j] * getPenetrationDepth(currentPieces[i], piece, currentVectors[i], vec);
// 	}
// 	return ret;
// }

// double ILSQN::getGlsTotalOverlap() {
// 	double ret = 0;
// 	for (int i = 0; i < currentPieces.size(); ++i) {
// 		ret += miuMatrix[i + 1][0] * (getPenetrationDepth(currentPieces[i], currentVectors[i]));
// 		for (int j = i+1; j < currentPieces.size(); ++j) {
// 			ret += miuMatrix[i + 1][j + 1] * getPenetrationDepth(currentPieces[i], currentPieces[j], currentVectors[i], currentVectors[j]);
// 		}
// 	}
// 	return ret;
// }

// double ILSQN::getMaxOverlap() {		// �����������ֵ��ص���
// double ret = 0.0;
// double temp;
// for (int i = 0; i < currentPieces.size(); ++i)
// {
// 	temp = getPenetrationDepth(currentPieces[i], currentVectors[i]);
// 	if (temp > ret)
// 	{
// 		ret = temp;
// 	}
// 	overlapMatrix[i + 1][0] = overlapMatrix[0][i + 1] = temp;

// 	for (int j = i + 1; j < currentPieces.size(); ++j)
// 	{
// 		temp = getPenetrationDepth(currentPieces[i], currentPieces[j], currentVectors[i], currentVectors[j]);
// 		if (temp > ret)
// 		{
// 			ret = temp;
// 		}
// 		overlapMatrix[i + 1][j + 1] = overlapMatrix[j + 1][i + 1] = temp;
// 	}
// }
// return ret;
// }