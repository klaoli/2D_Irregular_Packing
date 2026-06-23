#include "packing.h"
#include "datawriter.h"
#include "parameters.h"
#include "nofitpolygon.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <unordered_set>
using namespace MyNest;
Packing *Packing::packing = nullptr;

static size_t expectedNfpKeyCount()
{
	std::unordered_set<uint64_t> uniquePieces;
	for (const auto &rotationPieces : piecesCache)
	{
		for (const auto &piece : rotationPieces)
		{
			uniquePieces.insert(getIfrKey(piece));
		}
	}
	return uniquePieces.size() * uniquePieces.size();
}

Packing::Packing()
{
}

Packing *Packing::getInstance()
{
	if (packing == nullptr)
	{
		packing = new Packing;
	}
	return packing;
}

void Packing::preprocess()
{
	// 放缩 平移到原点
	std::sort(pieces.begin(), pieces.end(), [&](Piece &l, Piece &r)
			  { return l.area > r.area; });
	piecesCache.clear();
	int deltaAngle = 360 / parameters.orientations;
	Geometry *geo = Geometry::getInstance();

	for (int angle = 0; angle < 360; angle += deltaAngle)
	{
		std::vector<Piece> temp;
		for (auto &origPiece : pieces)
		{
			Piece piece = origPiece;
			piece.polygon = geo->rotate(piece.polygon, (double)angle);	   // 旋转
			piece.polygon = geo->offset(piece.polygon, parameters.minGap); // 伸缩
			double dx = -piece.polygon.outer().front().x();
			double dy = -piece.polygon.outer().front().y();
			piece.polygon = geo->translate(piece.polygon, dx, dy); // 平移

			piece.bounding = geo->getEnvelope(piece.polygon); // 外接矩形
			piece.area = bg::area(piece.polygon);			  // 面积
			piece.rotation = angle;
			temp.push_back(piece);
		}
		piecesCache.push_back(temp);
	}
}

int Packing::checkNfps()
{
	size_t expectedCount = expectedNfpKeyCount();
	if (expectedCount > 0 && nfpsCache.size() >= expectedCount)
	{
		return nfpsCache.size();
	}
	if (!nfpsCache.empty())
	{
		std::cout << "Warning: incomplete NFP cache, regenerating." << std::endl;
		nfpsCache.clear();
	}

	std::vector<Piece> allRotationPieces; // 将所有角度的零件放入容器中，便于遍历
	for (int i = 0; i < piecesCache.size(); ++i)
	{
		for (int j = 0; j < piecesCache[i].size(); ++j)
			allRotationPieces.push_back(piecesCache[i][j]);
	}

	NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();
	GeometryConvert *converter = GeometryConvert::getInstance();
	Geometry *geo = Geometry::getInstance();
	for (int i = 0; i < allRotationPieces.size(); i++)
	{
		for (int j = i; j < allRotationPieces.size(); j++)
		{

			auto nfpKey = getNfpKey(allRotationPieces[j], allRotationPieces[i]); // no fit polygon
			if (nfpsCache.find(nfpKey) == nfpsCache.end())
			{
				// 两种方法构造nfp: 滑动法、闵可夫斯基矢量差方法

				polygon_t nfp = nfpGenerator->minkowskiDifNfp(allRotationPieces[j].polygon, allRotationPieces[i].polygon);

				// libnfporb::polygon_t pA = converter->boost2LibNfpPolygon(allRotationPieces[j].polygon);
				// libnfporb::polygon_t pB = converter->boost2LibNfpPolygon(allRotationPieces[i].polygon);
				// polygon_t nfp = nfpGenerator->slideNfp(pA, pB);

				nfp = geo->simplifyPolygon(nfp, 1e-6, 0.1);

				nfpsCache.emplace(nfpKey, nfp);
			}

			auto nfpKey2 = getNfpKey(allRotationPieces[i], allRotationPieces[j]);
			if (nfpsCache.find(nfpKey2) == nfpsCache.end())
			{

				polygon_t nfp = nfpGenerator->minkowskiDifNfp(allRotationPieces[i].polygon, allRotationPieces[j].polygon);
				// libnfporb::polygon_t pA = converter->boost2LibNfpPolygon(allRotationPieces[j].polygon);
				// libnfporb::polygon_t pB = converter->boost2LibNfpPolygon(allRotationPieces[i].polygon);
				// polygon_t nfp = nfpGenerator->slideNfp(pB, pA);

				nfp = geo->simplifyPolygon(nfp, 1e-6, 0.1);

				nfpsCache.emplace(nfpKey2, nfp);
			}
		}
	}
	DataWrite *dataWriter = DataWrite::getInstance();
	dataWriter->writeNfps(nfpsCache, parameters.nfpsPath);
	return nfpsCache.size();
}

int Packing::checkIfps()
{
	if (ifrsCache.size() != 0)
	{
		return ifrsCache.size();
	}

	std::vector<Piece> allRotationPieces; // 将所有角度的零件放入容器中，便于遍历
	for (int i = 0; i < piecesCache.size(); ++i)
	{
		for (int j = 0; j < piecesCache[i].size(); ++j)
			allRotationPieces.push_back(piecesCache[i][j]);
	}

	NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();
	for (int i = 0; i < allRotationPieces.size(); ++i)
	{

		auto ifpKey = getIfrKey(allRotationPieces[i]); // 内接临界矩形

		if (ifpsCache.find(ifpKey) == ifpsCache.end())
		{
			polygon_t ifp = nfpGenerator->generateIfp(bin, allRotationPieces[i].polygon);
			ifpsCache.emplace(ifpKey, ifp);
			box_t ifr;
			bg::envelope(ifp, ifr);
			ifrsCache.emplace(ifpKey, ifr);
		}
	}
	DataWrite *dataWriter = DataWrite::getInstance();
	dataWriter->writeNfps(ifpsCache, parameters.ifpsPath);
	return ifpsCache.size();
}

point_t Packing::findMostLeftPoint(std::vector<ring_t> &finalNfp)
{
	point_t mostLeftPoint(Parameters::MAXDOUBLE, Parameters::MAXDOUBLE);

	for (auto &poly : finalNfp)
	{
		for (int i = 0; i < poly.size(); ++i)
		{
			if (poly[i].x() < mostLeftPoint.x())
			{
				mostLeftPoint.set<0>(poly[i].x());
				mostLeftPoint.set<1>(poly[i].y());
			}
			if (poly[i].x() == mostLeftPoint.x() && poly[i].y() < mostLeftPoint.y())
			{
				mostLeftPoint.set<1>(poly[i].y());
			}
		}
	}
	return mostLeftPoint;
}

double Packing::run(std::vector<Piece> &placedPieces, std::vector<Vector> &placedVectors)
{
	placedPieces.clear();
	placedVectors.clear();
	double placedMaxX = 0.0;
	const int numPieces = piecesCache.empty() ? 0 : static_cast<int>(piecesCache[0].size());
	for (int i = 0; i < numPieces; ++i)
	{
		struct PlacementCandidate
		{
			bool valid = false;
			Piece piece;
			Vector vector;
			double maxX = std::numeric_limits<double>::max();
			double minY = std::numeric_limits<double>::max();
			double maxY = std::numeric_limits<double>::max();
		};

		PlacementCandidate bestCandidate;
		for (size_t orientation = 0; orientation < piecesCache.size(); ++orientation)
		{
			const Piece &candidatePiece = piecesCache[orientation][i];
			auto ifrKey = getIfrKey(candidatePiece);
			const polygon_t &ifp = ifpsCache[ifrKey];
			point_t referPoint = candidatePiece.polygon.outer().front();

			auto considerPoint = [&](const point_t &point)
			{
				Vector curVector(point.x() - referPoint.x(), point.y() - referPoint.y());
				double candidateMaxX = candidatePiece.bounding.max_corner().x() + curVector.x;
				double candidateMinY = candidatePiece.bounding.min_corner().y() + curVector.y;
				double candidateMaxY = candidatePiece.bounding.max_corner().y() + curVector.y;
				double layoutMaxX = std::max(placedMaxX, candidateMaxX);
				if (!bestCandidate.valid ||
					layoutMaxX < bestCandidate.maxX ||
					(layoutMaxX == bestCandidate.maxX && candidateMinY < bestCandidate.minY) ||
					(layoutMaxX == bestCandidate.maxX && candidateMinY == bestCandidate.minY && candidateMaxY < bestCandidate.maxY))
				{
					bestCandidate.valid = true;
					bestCandidate.piece = candidatePiece;
					bestCandidate.vector = curVector;
					bestCandidate.maxX = layoutMaxX;
					bestCandidate.minY = candidateMinY;
					bestCandidate.maxY = candidateMaxY;
				}
			};

			if (placedPieces.empty())
			{
				for (auto &point : ifp.outer())
				{
					considerPoint(point);
				}
				continue;
			}
#pragma region ClipperExecute
			Paths clipperUnionNfp;
			Paths clipperFinalNfp;
			ClipperLib::Clipper clipperUnion;
			ClipperLib::Clipper clipperDifference;
			// bin_nfp 转换成 clipper paths，即 clipperBinNfp.
			GeometryConvert *converter = GeometryConvert::getInstance();
			Paths clipperBinNfp = converter->boost2ClipperPolygon(ifp);

			// nfp 转换成 clipper paths, 求并集得到 clipperUnionNfp.
			for (int j = 0; j < placedPieces.size(); ++j)
			{
				auto key = getNfpKey(placedPieces[j], candidatePiece);
				Paths clipperNfp = converter->boost2ClipperPolygon(nfpsCache[key]);
				for (auto &path : clipperNfp)
				{
					for (auto &point : path)
					{
						point.X += static_cast<cInt>(placedVectors[j].x * Parameters::scaleRate);
						point.Y += static_cast<cInt>(placedVectors[j].y * Parameters::scaleRate);
					}
				}
				clipperUnion.AddPaths(clipperNfp, ClipperLib::PolyType::ptSubject, true);
			}
			if (!clipperUnion.Execute(ClipperLib::ClipType::ctUnion, clipperUnionNfp, ClipperLib::PolyFillType::pftNonZero, ClipperLib::PolyFillType::pftNonZero))
			{
				std::cout << "clipperUnion Execute Failed: " << candidatePiece.id << std::endl;
				continue;
			}

			// clipperBinNfp 经 clipperUnionNfp 裁剪（求差集）得到 clipperFinalNfp.
			clipperDifference.AddPaths(clipperBinNfp, ClipperLib::PolyType::ptSubject, true);
			clipperDifference.AddPaths(clipperUnionNfp, ClipperLib::PolyType::ptClip, true);
			if (!clipperDifference.Execute(ClipperLib::ClipType::ctDifference, clipperFinalNfp, ClipperLib::PolyFillType::pftEvenOdd, ClipperLib::PolyFillType::pftNonZero))
			{
				std::cout << "clipperDifference Execute Failed: " << candidatePiece.id << std::endl;
				continue;
			}

			// clean clipperFinalNfp
			CleanPolygons(clipperFinalNfp, 0.0001 * Parameters::scaleRate);

			clipperFinalNfp.erase(std::remove_if(clipperFinalNfp.begin(), clipperFinalNfp.end(),
												 [](const Path &path)
												 {
													 return path.size() < 3 || ClipperLib::Area(path) < 0.1 * Parameters::scaleRate * Parameters::scaleRate;
												 }),
								  clipperFinalNfp.end());

			if (clipperFinalNfp.empty())
			{
				continue;
			}
#pragma endregion ClipperExecute

#pragma region Placement
			for (auto &path : clipperFinalNfp)
			{
				ring_t ring = converter->clipper2BoostRing(path);
				for (auto &point : ring)
				{
					considerPoint(point);
				}
			}
#pragma endregion Placement
		}

		if (!bestCandidate.valid)
		{
			std::cout << "Warning: no initial placement candidate for piece " << i << std::endl;
			continue;
		}
		placedPieces.push_back(bestCandidate.piece);
		placedVectors.push_back(bestCandidate.vector);
		placedMaxX = bestCandidate.maxX;
	}

	double minX = Parameters::MAXDOUBLE, maxX = 0;
	Geometry *geo = Geometry::getInstance();

	for (int i = 0; i < placedPieces.size(); ++i)
	{
		polygon_t translatedPolygon = geo->translate(placedPieces[i].polygon, placedVectors[i].x, placedVectors[i].y);
		box_t translatedBounding = geo->getEnvelope(translatedPolygon);
		if (translatedBounding.min_corner().x() < minX)
		{
			minX = translatedBounding.min_corner().x();
		}

		if (translatedBounding.max_corner().x() > maxX)
		{
			maxX = translatedBounding.max_corner().x();
		}
	}
	if (placedPieces.empty())
	{
		return 0.0;
	}
	return maxX - minX;
}
