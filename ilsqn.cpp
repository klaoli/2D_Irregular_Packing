#include "ilsqn.h"
#include "dataloader.h"
#include "parameters.h"
#include "nofitpolygon.h"
#include "datawriter.h"

#include <eigen3/Eigen/Core>
#include <algorithm>
#include <random>
#include <omp.h>
#include <chrono>
#include <cmath>
#include <numeric>
#include <utility>

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
	if (parameters.hasRandomSeed)
	{
		baseSeed = parameters.randomSeed;
	}
	else
	{
		baseSeed = std::random_device{}();
	}
	rng.seed(baseSeed);
	if (numPieces > static_cast<int>(parameters.largeInstanceThreshold))
	{
		parameters.maxIteration = std::max(parameters.maxIteration, static_cast<double>(parameters.minLargeIterations));
	}
	if (numPieces > static_cast<int>(parameters.largeInstanceThreshold * 1.6))
	{
		parameters.maxIteration = std::max(parameters.maxIteration, static_cast<double>(parameters.minVeryLargeIterations));
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

bool ILSQN::isLargeInstance() const
{
	return numPieces > static_cast<int>(parameters.largeInstanceThreshold);
}

int ILSQN::effectiveMaxIteration() const
{
	return std::max(1, static_cast<int>(parameters.maxIteration));
}

int ILSQN::effectiveRuinSize() const
{
	double ratio = isLargeInstance() ? parameters.ruinRatio : 0.15;
	return std::max(3, static_cast<int>(std::ceil(numPieces * ratio)));
}

uint32_t ILSQN::makeLocalSeed(int idx, int orientation, uint64_t round) const
{
	uint64_t x = static_cast<uint64_t>(baseSeed) + 0x9e3779b97f4a7c15ULL;
	x ^= static_cast<uint64_t>(idx + 1) * 0xbf58476d1ce4e5b9ULL;
	x ^= static_cast<uint64_t>(orientation + 1) * 0x94d049bb133111ebULL;
	x ^= (round + 1) * 0x2545f4914f6cdd1dULL;
	x ^= x >> 30;
	x *= 0xbf58476d1ce4e5b9ULL;
	x ^= x >> 27;
	x *= 0x94d049bb133111ebULL;
	x ^= x >> 31;
	return static_cast<uint32_t>(x);
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

struct IndexedBounds
{
	int idx;
	double minX;
	double maxX;
	double minY;
	double maxY;
};

inline IndexedBounds makeBounds(const Piece &piece, const Vector &vector, int idx)
{
	return IndexedBounds{
		idx,
		piece.bounding.min_corner().x() + vector.x,
		piece.bounding.max_corner().x() + vector.x,
		piece.bounding.min_corner().y() + vector.y,
		piece.bounding.max_corner().y() + vector.y};
}

inline bool yOverlaps(const IndexedBounds &a, const IndexedBounds &b)
{
	return a.minY < b.maxY && b.minY < a.maxY;
}

template <typename PieceGetter, typename VectorGetter, typename PairVisitor>
void visitPotentialPairs(int count, PieceGetter pieceGetter, VectorGetter vectorGetter, PairVisitor visitor)
{
	std::vector<IndexedBounds> bounds;
	bounds.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		const Vector &v = vectorGetter(i);
		if (v.x == Parameters::MAXDOUBLE || v.y == Parameters::MAXDOUBLE)
		{
			continue;
		}
		bounds.push_back(makeBounds(pieceGetter(i), v, i));
	}
	std::sort(bounds.begin(), bounds.end(), [](const IndexedBounds &a, const IndexedBounds &b)
			  { return a.minX < b.minX; });
	for (size_t a = 0; a < bounds.size(); ++a)
	{
		for (size_t b = a + 1; b < bounds.size(); ++b)
		{
			if (bounds[b].minX >= bounds[a].maxX)
			{
				break;
			}
			if (yOverlaps(bounds[a], bounds[b]))
			{
				visitor(bounds[a].idx, bounds[b].idx);
			}
		}
	}
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
	}
	visitPotentialPairs(
		static_cast<int>(currentPieces.size()),
		[&](int i) -> const Piece& { return currentPieces[i]; },
		[&](int i) -> const Vector& { return currentVectors[i]; },
		[&](int i, int j)
		{
			ret += getPenetrationDepth(currentPieces[i], currentPieces[j], currentVectors[i], currentVectors[j]);
		});
	return ret;
}

std::vector<double> ILSQN::computePieceOverlapContributions()
{
	std::vector<double> contributions(lbfgsPieces.size(), 0.0);
	for (int i = 0; i < lbfgsPieces.size(); ++i)
	{
		if (lbfgsVectors[i].x == Parameters::MAXDOUBLE || lbfgsVectors[i].y == Parameters::MAXDOUBLE)
		{
			contributions[i] = Parameters::MAXDOUBLE;
			continue;
		}
		contributions[i] += getPenetrationDepth(lbfgsPieces[i], lbfgsVectors[i]);
	}
	visitPotentialPairs(
		static_cast<int>(lbfgsPieces.size()),
		[&](int i) -> const Piece& { return lbfgsPieces[i]; },
		[&](int i) -> const Vector& { return lbfgsVectors[i]; },
		[&](int i, int j)
		{
			double overlap = getPenetrationDepth(lbfgsPieces[i], lbfgsPieces[j], lbfgsVectors[i], lbfgsVectors[j]);
			contributions[i] += overlap;
			contributions[j] += overlap;
		});
	return contributions;
}

double ILSQN::getOneTotalOverlap(const Piece &piece, const Vector &vec)
{
	double ret = 0.0;
	ret += getPenetrationDepth(piece, vec);
	for (int i = 0; i < lbfgsPieces.size(); ++i)
	{
		if (lbfgsPieces[i].id == piece.id)
		{
			continue;
		}
		ret += getPenetrationDepth(lbfgsPieces[i], piece, lbfgsVectors[i], vec);
	}
	return ret;
}

double ILSQN::costFunction(void *instance, const Eigen::VectorXd &x, Eigen::VectorXd &grad)
{
	double ret = 0.0;
	int numPieces = x.size() / 2;

	grad = VectorXd::Zero(x.size());
	std::vector<Vector> vectors;
	vectors.reserve(numPieces);
	for (int i = 0; i < numPieces; ++i)
	{
		vectors.emplace_back(x(2 * i), x(2 * i + 1));
	}
	std::vector<std::pair<int, int>> pairs;
	visitPotentialPairs(
		numPieces,
		[&](int i) -> const Piece& { return lbfgsPieces[i]; },
		[&](int i) -> const Vector& { return vectors[i]; },
		[&](int i, int j)
		{
			pairs.emplace_back(i, j);
		});

	if (parameters.parallelCost && (numPieces > 32 || pairs.size() > 128))
	{
		const int threadCount = std::max(1, omp_get_max_threads());
		std::vector<double> localCosts(threadCount, 0.0);
		std::vector<std::vector<double>> localGrads(threadCount, std::vector<double>(x.size(), 0.0));

#pragma omp parallel
		{
			const int tid = omp_get_thread_num();
			double localCost = 0.0;
			std::vector<double> &localGrad = localGrads[tid];

#pragma omp for nowait
			for (int i = 0; i < numPieces; ++i)
			{
				Vector seperateVec;
				localCost += getPenetrationDepth(lbfgsPieces[i], vectors[i], seperateVec);
				localGrad[2 * i] += (-2 * seperateVec.x);
				localGrad[2 * i + 1] += (-2 * seperateVec.y);
			}

#pragma omp for nowait
			for (int p = 0; p < static_cast<int>(pairs.size()); ++p)
			{
				const int i = pairs[p].first;
				const int j = pairs[p].second;
				Vector seperateVec;
				localCost += getPenetrationDepth(lbfgsPieces[j], lbfgsPieces[i], vectors[j], vectors[i], seperateVec);
				localGrad[2 * i] += (-2 * seperateVec.x);
				localGrad[2 * i + 1] += (-2 * seperateVec.y);
				localGrad[2 * j] += (2 * seperateVec.x);
				localGrad[2 * j + 1] += (2 * seperateVec.y);
			}

			localCosts[tid] = localCost;
		}

		for (int t = 0; t < threadCount; ++t)
		{
			ret += localCosts[t];
			for (int i = 0; i < grad.size(); ++i)
			{
				grad[i] += localGrads[t][i];
			}
		}
	}
	else
	{
		Vector seperateVec;
		for (int i = 0; i < numPieces; ++i)
		{
			ret += getPenetrationDepth(lbfgsPieces[i], vectors[i], seperateVec);
			grad[2 * i] += (-2 * seperateVec.x);
			grad[2 * i + 1] += (-2 * seperateVec.y);
		}
		for (const auto &pair : pairs)
		{
			const int i = pair.first;
			const int j = pair.second;
			ret += getPenetrationDepth(lbfgsPieces[j], lbfgsPieces[i], vectors[j], vectors[i], seperateVec);
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
	const int orientationCount = static_cast<int>(std::min(parameters.orientations, piecesCache.size()));
	if (orientationCount == 0)
	{
		return;
	}
	std::vector<Vector> vecVectors(orientationCount);
	std::vector<double> overlaps(orientationCount, parameters.MAXDOUBLE);
	const uint64_t samplingRound = samplingCounter++;

#pragma omp parallel for num_threads(orientationCount)
	for (int k = 0; k < orientationCount; ++k)
	{
		const Piece &piece = piecesCache[k][idx];
		std::vector<polygon_t> nfps;
		auto key = getIfrKey(piece);
		nfps.push_back(ifpsCache[key]);
		for (int i = 0; i < lbfgsPieces.size(); ++i)
		{
			if (lbfgsPieces[i].id == piece.id)
			{
				continue;
			}
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
				if (nfps[i].outer().size() < 2)
				{
					continue;
				}
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
			std::vector<box_t> nfpBoxs(nfps.size());
			std::vector<char> validNfps(nfps.size(), 0);
			for (size_t i = 0; i < nfps.size(); ++i)
			{
				if (nfps[i].outer().size() < 2)
				{
					continue;
				}
				bg::envelope(nfps[i], nfpBoxs[i]);
				validNfps[i] = 1;
			}
			for (size_t i = 0; i < nfps.size(); ++i)
			{
				if (!validNfps[i])
				{
					continue;
				}
				candidatePoints.insert(candidatePoints.end(), nfps[i].outer().begin(), nfps[i].outer().end() - 1);
				for (size_t j = i + 1; j < nfps.size(); ++j)
				{
					if (!validNfps[j])
					{
						continue;
					}
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

		const size_t maxSamplePoints = std::min<size_t>(
			8000,
			std::max<size_t>(parameters.candidateSampleLimit, static_cast<size_t>(numPieces) * 120));
		std::vector<int> selectedIndices;
		selectedIndices.reserve(std::min(maxSamplePoints, candidatePoints.size()));
		std::vector<char> selected(candidatePoints.size(), 0);
		auto addIndex = [&](int index)
		{
			if (index < 0 || index >= static_cast<int>(candidatePoints.size()) || selected[index])
			{
				return;
			}
			selected[index] = 1;
			selectedIndices.push_back(index);
		};
		if (candidatePoints.size() <= maxSamplePoints)
		{
			for (int i = 0; i < candidatePoints.size(); ++i)
			{
				addIndex(i);
			}
		}
		else
		{
			std::vector<int> numbers(candidatePoints.size());
			std::iota(numbers.begin(), numbers.end(), 0);
			const size_t deterministicBudget = std::max<size_t>(1, maxSamplePoints / 2);
			const size_t perHeuristicBudget = std::max<size_t>(1, deterministicBudget / 3);

			std::sort(numbers.begin(), numbers.end(), [&](int a, int b)
					  { return candidatePoints[a].x() < candidatePoints[b].x(); });
			for (size_t j = 0; j < numbers.size() && selectedIndices.size() < perHeuristicBudget; ++j)
			{
				addIndex(numbers[j]);
			}
			std::sort(numbers.begin(), numbers.end(), [&](int a, int b)
					  { return candidatePoints[a].y() < candidatePoints[b].y(); });
			for (size_t j = 0; j < numbers.size() && selectedIndices.size() < perHeuristicBudget * 2; ++j)
			{
				addIndex(numbers[j]);
			}
			const point_t referPoint = piece.polygon.outer().front();
			std::sort(numbers.begin(), numbers.end(), [&](int a, int b)
					  {
						  double aMaxX = candidatePoints[a].x() - referPoint.x() + piece.bounding.max_corner().x();
						  double bMaxX = candidatePoints[b].x() - referPoint.x() + piece.bounding.max_corner().x();
						  return aMaxX < bMaxX;
					  });
			for (size_t j = 0; j < numbers.size() && selectedIndices.size() < deterministicBudget; ++j)
			{
				addIndex(numbers[j]);
			}

			std::mt19937 localRng(makeLocalSeed(idx, k, samplingRound));
			std::shuffle(numbers.begin(), numbers.end(), localRng);
			for (size_t j = 0; j < numbers.size() && selectedIndices.size() < maxSamplePoints; ++j)
			{
				addIndex(numbers[j]);
			}
		}

		for (int i : selectedIndices)
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
	auto bestIt = std::min_element(overlaps.begin(), overlaps.end());
	if (bestIt == overlaps.end() || *bestIt == Parameters::MAXDOUBLE)
	{
		std::cout << "Warning: no relocation candidate for piece " << idx << std::endl;
		if (idx >= 0 && idx < static_cast<int>(currentPieces.size()) && idx < static_cast<int>(currentVectors.size()))
		{
			lbfgsPieces[idx] = currentPieces[idx];
			lbfgsVectors[idx] = currentVectors[idx];
		}
		return;
	}
	int index = bestIt - overlaps.begin();
	lbfgsPieces[idx] = piecesCache[index][idx];
	lbfgsVectors[idx] = vecVectors[index];
}

void ILSQN::ruinAndRecreate(int k)
{
	if (k > numPieces) k = numPieces;
	if (k < 1) return;

	std::vector<int> ruined;
	ruined.reserve(k);
	std::vector<char> selected(numPieces, 0);
	auto addRuined = [&](int idx)
	{
		if (idx < 0 || idx >= numPieces || selected[idx])
		{
			return false;
		}
		selected[idx] = 1;
		ruined.push_back(idx);
		return true;
	};

	if (isLargeInstance())
	{
		std::vector<double> contributions = computePieceOverlapContributions();
		std::vector<int> order(numPieces);
		std::iota(order.begin(), order.end(), 0);
		std::sort(order.begin(), order.end(), [&](int a, int b)
				  { return contributions[a] > contributions[b]; });
		int conflictCount = std::min(k, std::max(1, static_cast<int>(std::ceil(k * parameters.conflictRuinRatio))));
		for (int idx : order)
		{
			if (static_cast<int>(ruined.size()) >= conflictCount)
			{
				break;
			}
			addRuined(idx);
		}
	}

	while (static_cast<int>(ruined.size()) < k)
	{
		addRuined(generateRandomNumber(numPieces));
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
	std::uniform_int_distribution<int> distribution(0, n - 1);
	return distribution(rng);
}

double ILSQN::generateRandomDouble(double min, double max)
{
	std::uniform_real_distribution<double> dis(min, max); // 均匀分布

	return dis(rng); // 返回生成的随机数
}

void ILSQN::minimizeOverlap()
{
	feasible = false;
	double totalOverlap = getTotalOverlap(); // 计算当前布局总的重叠量
	if (totalOverlap < eps)
	{
		feasible = true;
		return;
	}

	// === 精英保留策略：永远记住全局最优解 ===
	double bestOverlap = totalOverlap;
	std::vector<Piece> elitePieces = lbfgsPieces;
	std::vector<Vector> eliteVectors = lbfgsVectors;
	int stagnation = 0;                              // 连续无改善次数
	int patience = std::max(10, numPieces / 2);       // 耐心窗口
	const int maxIteration = effectiveMaxIteration();

	int iter = 0;
	while (iter++ < maxIteration)
	{
		int ruinSize = effectiveRuinSize();
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
				double progress = static_cast<double>(iter) / maxIteration;
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
			if (isLargeInstance())
			{
				ruinAndRecreate(std::max(3, effectiveRuinSize() / 2));
				double repairedOverlap = seperate(numPieces * 2, totalOverlap);
				if (repairedOverlap <= totalOverlap)
				{
					totalOverlap = repairedOverlap;
					currentPieces = lbfgsPieces;
					currentVectors = lbfgsVectors;
				}
				else
				{
					lbfgsPieces = currentPieces;
					lbfgsVectors = currentVectors;
				}
			}
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
	double initialLength = packing->run(placedPieces, currentVectors);
	currentPieces = placedPieces;
	return initialLength;
}

double ILSQN::run()
{
	currentLength = getIniaialSolution(); // 生成初始布局
	currentBin.max_corner().set<0>(currentLength);

	bestPieces = currentPieces;
	bestVectors = currentVectors;
	bestBin = currentBin;
	bestLength = currentLength;

	double feasibleLength = currentLength;
	double infeasibleLength = 0.0;
	bool hasInfeasibleLength = false;
	double lastPrintedUtil = -1.0;
	int lengthSearchIterations = 0;
	const int maxLengthSearchIterations = 1000;

	currentLength = (1 - dec) * currentLength; // 按比例缩短板材边界
	currentBin.max_corner().set<0>(currentLength);

	lbfgsPieces = currentPieces;
	lbfgsVectors = currentVectors;

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
	auto end = start;
	std::chrono::duration<double> time_taken = end - start;

	while (time_taken.count() < parameters.maxRunTime && !stopRequested)
	{
		if (++lengthSearchIterations > maxLengthSearchIterations)
		{
			std::cout << "达到最大外层搜索次数，提前结束。" << std::endl;
			break;
		}
		getInnerFitPolygons(); // 获取内靠接矩形

		minimizeOverlap(); // 执行最小化重叠

		if (feasible)
		{
			double current_util = allPiecesArea / bg::area(currentBin);
			if (lastPrintedUtil < 0 || std::abs(current_util - lastPrintedUtil) > 1e-6)
			{
				std::cout << "当前利用率 = " << current_util << std::endl;
				lastPrintedUtil = current_util;
			}

			bestPieces = currentPieces;
			bestVectors = currentVectors;
			bestBin = currentBin;
			bestLength = currentLength;
			feasibleLength = currentLength;

			// 回调通知 GUI 更新
			if (onLayoutUpdated) {
				onLayoutUpdated(bestBin, bestPieces, bestVectors, current_util);
			}

			if (current_util >= 0.99)
			{
				std::cout << ">>> 达到 99% 理想利用率，提前退出该轮搜索！" << std::endl;
				break;
			}

			double nextLength = hasInfeasibleLength
				? (feasibleLength + infeasibleLength) / 2.0
				: (1 - dec) * feasibleLength; // 缩减板材的长度
			double tolerance = std::max(1e-6, feasibleLength * 1e-8);
			if (hasInfeasibleLength && std::abs(infeasibleLength - feasibleLength) <= tolerance)
			{
				std::cout << "板材长度搜索区间已收敛，提前结束。" << std::endl;
				break;
			}
			if (std::abs(feasibleLength - nextLength) <= tolerance)
			{
				std::cout << "板材长度变化已低于阈值，提前结束。" << std::endl;
				break;
			}
			currentLength = nextLength;
			currentBin.max_corner().set<0>(currentLength);
			feasible = false;
		}
		else
		{
			infeasibleLength = currentLength;
			hasInfeasibleLength = true;
			currentVectors = bestVectors;
			currentPieces = bestPieces;
			double nextLength = (feasibleLength + infeasibleLength) / 2.0;
			double tolerance = std::max(1e-6, feasibleLength * 1e-8);
			if (std::abs(infeasibleLength - feasibleLength) <= tolerance)
			{
				std::cout << "板材长度搜索区间已收敛，提前结束。" << std::endl;
				break;
			}
			if (nextLength >= feasibleLength)
			{
				nextLength = (1 - dec) * feasibleLength;
			}
			currentLength = nextLength;
			currentBin.max_corner().set<0>(currentLength);
		}
		end = std::chrono::steady_clock::now();
		time_taken = end - start;

	}
	static DataWrite *datawriter = DataWrite::getInstance();
	datawriter->plotPieces(bestBin, bestPieces, bestVectors);

	std::cout << "搜索结束，最好利用率 = " << allPiecesArea / bg::area(bestBin) << std::endl;
	return allPiecesArea / bg::area(bestBin);
}
