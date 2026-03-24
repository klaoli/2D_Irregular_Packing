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
Packing *ILSQN::packing = Packing::getInstance();

ILSQN::ILSQN(double _inc, double _dec)
{
	inc = _inc;
	dec = _dec;
	numPieces = pieces.size();
	for (const auto &piece : pieces)
	{
		allPiecesArea += piece.area;
	}

	packing->preprocess(); // 零件预处理
	currentPieces = piecesCache[0];

	packing->checkNfps(); // 检查nfp
}

ILSQN *ILSQN::getInstance()
{

	if (ilsqn == nullptr)
	{
		ilsqn = new ILSQN(parameters.inc, parameters.dec);
	}
	return ilsqn;
}

// 计算点 p0 到线段 p1-p2 的最短平移向量
inline Vector shortestTranslationVector(const point_t &p0, const point_t &p1, const point_t &p2)
{
	double v_x = p2.x() - p1.x();
	double v_y = p2.y() - p1.y();

	double w_x = p0.x() - p1.x();
	double w_y = p0.y() - p1.y();

	double c1 = w_x * v_x + w_y * v_y; // 投影
	if (c1 <= 0)
	{
		return Vector(p1.x() - p0.x(), p1.y() - p0.y());
	}

	double c2 = v_x * v_x + v_y * v_y; // 线段长度平方
	if (c2 <= c1)
	{
		return Vector(p2.x() - p0.x(), p2.y() - p0.y());
	}

	double b = c1 / c2;
	return Vector(p1.x() + v_x * b - p0.x(), p1.y() + v_y * b - p0.y());
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

double ILSQN::getPenetrationDepth(const Piece &p1, const Piece &p2, const Vector &v1, const Vector &v2)
{
	// 快速包围盒重叠检测，避免构造 box_t
	if (p1.bounding.min_corner().x() + v1.x >= p2.bounding.max_corner().x() + v2.x ||
		p2.bounding.min_corner().x() + v2.x >= p1.bounding.max_corner().x() + v1.x ||
		p1.bounding.min_corner().y() + v1.y >= p2.bounding.max_corner().y() + v2.y ||
		p2.bounding.min_corner().y() + v2.y >= p1.bounding.max_corner().y() + v1.y) 
	{
		return 0.0;
	}

	auto key = getNfpKey(p1, p2);
	const polygon_t &nfp = nfpsCache[key];
	point_t referPoint(v2.x - v1.x, v2.y - v1.y); // v2也可表达参考点坐标，将参考点沿v1的反方向平移可减少计算量

	if (bg::within(referPoint, nfp))
	{
		double distance = Parameters::MAXDOUBLE;
		const auto& outer = nfp.outer();

		for (size_t i = 0; i < outer.size() - 1; ++i)
		{
			Vector tempVector = shortestTranslationVector(referPoint, outer[i], outer[i + 1]);
			double tempDistance = tempVector * tempVector;
			if (tempDistance < distance)
			{
				distance = tempDistance;
			}
		}
		return distance;
	}
	
	return 0.0;
}

double ILSQN::getPenetrationDepth(const Piece &p, const Vector &vec)
{
	auto key = getIfrKey(p);
	const box_t &ifr = ifrsCache[key];
	point_t referPoint(vec.x, vec.y);
	return pointToRectangleDistance(referPoint, ifr);
}

double ILSQN::getPenetrationDepth(const Piece &p1, const Piece &p2, const Vector &v1, const Vector &v2, Vector &seperateVector)
{
	if (p1.bounding.min_corner().x() + v1.x >= p2.bounding.max_corner().x() + v2.x ||
		p2.bounding.min_corner().x() + v2.x >= p1.bounding.max_corner().x() + v1.x ||
		p1.bounding.min_corner().y() + v1.y >= p2.bounding.max_corner().y() + v2.y ||
		p2.bounding.min_corner().y() + v2.y >= p1.bounding.max_corner().y() + v1.y) 
	{
		seperateVector.x = 0; seperateVector.y = 0;
		return 0.0;
	}

	auto key = getNfpKey(p1, p2);
	const polygon_t &nfp = nfpsCache[key];
	point_t referPoint(v2.x - v1.x, v2.y - v1.y); // v2也可表达参考点坐标，将参考点沿v1的反方向平移可减少计算量

	if (bg::within(referPoint, nfp))
	{
		double distance = Parameters::MAXDOUBLE;
		const auto& outer = nfp.outer();

		for (size_t i = 0; i < outer.size() - 1; ++i)
		{
			Vector tempVector = shortestTranslationVector(referPoint, outer[i], outer[i + 1]);
			double tempDistance = tempVector * tempVector;
			if (tempDistance < distance)
			{
				distance = tempDistance;
				seperateVector = tempVector;
			}
		}
		return distance;
	}
	
	seperateVector.x = 0; seperateVector.y = 0;
	return 0.0;
}

double ILSQN::getPenetrationDepth(const Piece &p, const Vector &v, Vector &seperateVector)
{
	auto key = getIfrKey(p);
	const box_t &ifr = ifrsCache[key];
	
	double dx = 0;
	if (v.x < ifr.min_corner().x()) dx = ifr.min_corner().x() - v.x;
	else if (v.x > ifr.max_corner().x()) dx = ifr.max_corner().x() - v.x;

	double dy = 0;
	if (v.y < ifr.min_corner().y()) dy = ifr.min_corner().y() - v.y;
	else if (v.y > ifr.max_corner().y()) dy = ifr.max_corner().y() - v.y;

	seperateVector.x = dx;
	seperateVector.y = dy;
	
	// dx 和 dy 可能同时为 0，即点在矩形内
	return dx * dx + dy * dy;
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

double ILSQN::getOneTotalOverlap(const Piece &piece, const Vector &vec)
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
	int numPieces = x.size() / 2;

	// 直接从 x 读取向量，避免创建临时 vector
	grad = VectorXd::Zero(x.size());
	Vector seperateVec;
	for (int i = 0; i < numPieces; ++i)
	{
		Vector vi(x(2 * i), x(2 * i + 1));
		ret += (getPenetrationDepth(lbfgsPieces[i], vi, seperateVec));
		grad[2 * i] += (-2 * seperateVec.x);
		grad[2 * i + 1] += (-2 * seperateVec.y);

		for (int j = i + 1; j < numPieces; ++j)
		{
			Vector vj(x(2 * j), x(2 * j + 1));
			ret += (getPenetrationDepth(lbfgsPieces[j], lbfgsPieces[i], vj, vi, seperateVec));
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

// 统一搜索最佳位置的核心逻辑
// useMiddlePoints = false: 使用顶点+交点 (findBestPosition)
// useMiddlePoints = true:  使用顶点+中点 (movePolygon)
void ILSQN::searchBestPosition(int idx, bool useMiddlePoints)
{
	std::vector<Vector> vecVectors(parameters.orientations);
	std::vector<double> overlaps(parameters.orientations, parameters.MAXDOUBLE);

#pragma omp parallel for num_threads(parameters.orientations)
	for (int k = 0; k < parameters.orientations; ++k)
	{
		Piece &piece = piecesCache[k][idx];
		std::vector<polygon_t> nfps;
		auto key = getIfrKey(piece);
		nfps.push_back(ifpsCache[key]);
		for (int i = 0; i < lbfgsPieces.size(); ++i)
		{
			if (lbfgsVectors[i].x == Parameters::MAXDOUBLE || lbfgsVectors[i].y == Parameters::MAXDOUBLE)
			{
				continue;
			}
			key = getNfpKey(lbfgsPieces[i], piece);
			const polygon_t &nfp = nfpsCache[key];
			polygon_t transNfp;
			bg::strategy::transform::translate_transformer<double, 2, 2> translate(lbfgsVectors[i].x, lbfgsVectors[i].y);
			bg::transform(nfp, transNfp, translate);
			nfps.push_back(std::move(transNfp));
		}

		std::vector<point_t> candidatePoints;

		if (useMiddlePoints)
		{
			// movePolygon 策略：取顶点 + 中点
			for (size_t i = 0; i < nfps.size(); ++i)
			{
				candidatePoints.insert(candidatePoints.end(), nfps[i].outer().begin(), nfps[i].outer().end() - 1);
				for (size_t j = 0; j < nfps[i].outer().size() - 1; ++j)
				{
					point_t p(
						(nfps[i].outer()[j].x() + nfps[i].outer()[j + 1].x()) / 2,
						(nfps[i].outer()[j].y() + nfps[i].outer()[j + 1].y()) / 2);
					candidatePoints.push_back(p);
				}
			}
		}
		else
		{
			// findBestPosition 策略：取顶点 + NFP交点
			std::vector<box_t> nfpBoxs;
			nfpBoxs.reserve(nfps.size());
			for (size_t i = 0; i < nfps.size(); ++i)
			{
				box_t box;
				bg::envelope(nfps[i], box);
				nfpBoxs.push_back(box);
			}
			for (size_t i = 0; i < nfps.size(); ++i)
			{
				candidatePoints.insert(candidatePoints.end(), nfps[i].outer().begin(), nfps[i].outer().end() - 1);
				for (size_t j = i + 1; j < nfps.size(); ++j)
				{
					if (nfpBoxs[i].min_corner().x() >= nfpBoxs[j].max_corner().x() ||
						nfpBoxs[j].min_corner().x() >= nfpBoxs[i].max_corner().x() ||
						nfpBoxs[i].min_corner().y() >= nfpBoxs[j].max_corner().y() ||
						nfpBoxs[j].min_corner().y() >= nfpBoxs[i].max_corner().y())
					{
						continue;
					}
					std::vector<point_t> output;
					bg::intersection(nfps[i], nfps[j], output);
					if (!output.empty())
					{
						candidatePoints.insert(candidatePoints.end(), output.begin(), output.end());
					}
				}
			}
		}

		const int maxSamplePoints = 800;
		if (candidatePoints.size() <= maxSamplePoints)
		{
			for (size_t i = 0; i < candidatePoints.size(); ++i)
			{
				Vector vec(candidatePoints[i].x(), candidatePoints[i].y());
				double overlap = getOneTotalOverlap(piece, vec);
				if (overlap < overlaps[k])
				{
					overlaps[k] = overlap;
					vecVectors[k] = vec;
				}
			}
		}
		else
		{
			std::vector<int> numbers(candidatePoints.size());
			std::iota(numbers.begin(), numbers.end(), 0);
			thread_local std::mt19937 rng(std::random_device{}());
			std::shuffle(numbers.begin(), numbers.end(), rng);

			for (int j = 0; j < maxSamplePoints; ++j)
			{
				int i = numbers[j];
				Vector vec(candidatePoints[i].x(), candidatePoints[i].y());
				double overlap = getOneTotalOverlap(piece, vec);
				if (overlap < overlaps[k])
				{
					overlaps[k] = overlap;
					vecVectors[k] = vec;
				}
			}
		}
	}
	int index = std::min_element(overlaps.begin(), overlaps.end()) - overlaps.begin();
	lbfgsPieces[idx] = piecesCache[index][idx];
	lbfgsVectors[idx] = vecVectors[index];
}

void ILSQN::ruinAndRecreate(int k)
{
	if (k > numPieces) k = numPieces;
	if (k < 1) return;

	std::vector<int> ruined;
	ruined.reserve(k);
	for (int i = 0; i < k; ++i)
	{
		int idx = generateRandomNumber(numPieces);
		while (std::find(ruined.begin(), ruined.end(), idx) != ruined.end())
		{
			idx = generateRandomNumber(numPieces);
		}
		ruined.push_back(idx);
	}

	// 1. 破坏：移到界外，防止搜索干涉
	for (int i = 0; i < k; ++i)
	{
		lbfgsVectors[ruined[i]].x = Parameters::MAXDOUBLE;
		lbfgsVectors[ruined[i]].y = Parameters::MAXDOUBLE;
	}

	// 2. 重建：按顺序贪心寻找最优位置重插
	for (int i = 0; i < k; ++i)
	{
		searchBestPosition(ruined[i], true);
	}
}

void ILSQN::findBestPosition(int idx)
{
	searchBestPosition(idx, false);
}

void ILSQN::movePolygon(int idx)
{
	searchBestPosition(idx, true);
}

void ILSQN::swapPolygons(int idx1, int idx2)
{
	lbfgsVectors[idx2].x = Parameters::MAXDOUBLE, lbfgsVectors[idx2].y = Parameters::MAXDOUBLE;
	movePolygon(idx1);
	movePolygon(idx2);
}

int ILSQN::generateRandomNumber(int n)
{
	static thread_local std::mt19937 gen(std::random_device{}());
	std::uniform_int_distribution<int> distribution(0, n - 1);
	return distribution(gen);
}

double ILSQN::generateRandomDouble(double min, double max)
{
	static thread_local std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<double> dis(min, max); // 均匀分布

	return dis(gen); // 返回生成的随机数
}

void ILSQN::minimizeOverlap()
{
	double totalOverlap = getTotalOverlap(); // 计算当前布局总的重叠量

	// === 精英保留策略：永远记住全局最优解 ===
	double bestOverlap = totalOverlap;
	std::vector<Piece> elitePieces = lbfgsPieces;
	std::vector<Vector> eliteVectors = lbfgsVectors;
	int stagnation = 0;                              // 连续无改善次数
	int patience = std::max(10, numPieces / 2);       // 耐心窗口

	int iter = 0;
	while (iter++ < parameters.maxIteration)
	{
		int ruinSize = std::max(3, static_cast<int>(numPieces * 0.15));
		// 当零件较多时，以 40% 的概率执行大规模"破坏与重建"扰动，否则普通互换
		if (numPieces >= 30 && generateRandomNumber(100) < 40)
		{
			ruinAndRecreate(ruinSize);
		}
		else
		{
			int i = generateRandomNumber(numPieces);
			int j = generateRandomNumber(numPieces);

			while (i == j)
			{
				j = generateRandomNumber(numPieces);
			}

			swapPolygons(i, j);
		}

		double tempTotalOverlap = seperate(numPieces * 2, totalOverlap);

		// === 精英保留 + 有限退火 接受准则 ===
		bool accept = false;
		if (tempTotalOverlap < totalOverlap)
		{
			accept = true;
		}
		else
		{
			// 只容忍微小恶化（delta 不超过当前重叠量的 3%）
			double delta = tempTotalOverlap - totalOverlap;
			double maxTolerable = totalOverlap * 0.03 + 1e-6;
			if (delta < maxTolerable)
			{
				// 进度衰减概率：越到末期越保守
				double progress = static_cast<double>(iter) / parameters.maxIteration;
				double prob = (1.0 - progress) * 0.3; // 最初 30% 概率，末期趋近 0
				if (generateRandomDouble(0.0, 1.0) < prob)
				{
					accept = true;
				}
			}
		}

		if (accept)
		{
			totalOverlap = tempTotalOverlap;
			currentPieces = lbfgsPieces;
			currentVectors = lbfgsVectors;

			// 更新全局最优（精英解）
			if (totalOverlap < bestOverlap)
			{
				bestOverlap = totalOverlap;
				elitePieces = lbfgsPieces;
				eliteVectors = lbfgsVectors;
				stagnation = 0; // 刷新了最优解，重置耐心
			}
			else
			{
				stagnation++;
			}
		}
		else
		{
			lbfgsPieces = currentPieces;
			lbfgsVectors = currentVectors;
			stagnation++;
		}

		// === 耐心回退：连续多轮无改善，回退到精英解重新出发 ===
		if (stagnation >= patience)
		{
			lbfgsPieces = elitePieces;
			lbfgsVectors = eliteVectors;
			currentPieces = elitePieces;
			currentVectors = eliteVectors;
			totalOverlap = bestOverlap;
			stagnation = 0;
		}

		if (totalOverlap < eps)
		{
			feasible = true;
			break;
		}
	}
}

void ILSQN::getInnerFitPolygons()
{
	static NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();
	for (size_t i = 0; i < piecesCache.size(); ++i)
	{
		for (size_t j = 0; j < piecesCache[i].size(); ++j)
		{
			auto key = getIfrKey(piecesCache[i][j]);
			polygon_t ifp = nfpGenerator->generateIfp(currentBin, piecesCache[i][j].polygon);
			ifpsCache[key] = ifp;
			box_t ifr;
			bg::envelope(ifp, ifr);
			ifrsCache[key] = ifr;
		}
	}
}

double ILSQN::getIniaialSolution()
{
	currentBin = bin;
	currentVectors.clear();
	getInnerFitPolygons();
	std::vector<Piece> placedPieces;
	return packing->run(placedPieces, currentVectors);
}

double ILSQN::run()
{
	currentLength = getIniaialSolution(); // 生成初始布局
	currentBin.max_corner().set<0>(currentLength);

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

	while (time_taken.count() < parameters.maxRunTime && !stopRequested)
	{
		getInnerFitPolygons(); // 获取内靠接矩形

		minimizeOverlap(); // 执行最小化重叠

		if (feasible)
		{
			double current_util = allPiecesArea / bg::area(currentBin);
			std::cout << "当前利用率 = " << current_util << std::endl;

			bestPieces = currentPieces;
			bestVectors = currentVectors;
			bestBin = currentBin;
			bestLength = currentLength;

			// 回调通知 GUI 更新
			if (onLayoutUpdated) {
				onLayoutUpdated(bestBin, bestPieces, bestVectors, current_util);
			}

			if (current_util >= 0.99)
			{
				std::cout << ">>> 达到 99% 理想利用率，提前退出该轮搜索！" << std::endl;
				break;
			}

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
	static DataWrite *datawriter = DataWrite::getInstance();
	datawriter->plotPieces(bestBin, bestPieces, bestVectors);

	std::cout << "达到最大搜索时间，最好利用率 = " << allPiecesArea / bg::area(bestBin) << std::endl;
	return allPiecesArea / bg::area(bestBin);
}