#include "packing.h"
#include "datawriter.h"
#include "parameters.h"
#include "nofitpolygon.h"

#include <fstream>
using namespace MyNest;
Packing *Packing::packing = nullptr;

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
	// 放缩  简化  平移到原点
	std::sort(pieces.begin(), pieces.end(), [&](Piece &l, Piece &r)
			  { return l.area > r.area; });
	piecesCache.clear();
	int deltaAngle = 360 / parameters.orientations;
	for (int angle = 0; angle < 360; angle += deltaAngle)
	{
		std::vector<Piece> temp;
		for (auto piece : pieces)
		{
			piece.rotate((double)angle);
			piece.offset(parameters.minGap);
			piece.clean();
			double dx = -piece.polygon.outer().front().x();
			double dy = -piece.polygon.outer().front().y();
			piece.translate(dx, dy);
			temp.push_back(piece);
		}
		piecesCache.push_back(temp);
	}
}

int Packing::checkNfps()
{
	if (nfpsCache.size() != 0)
	{
		return nfpsCache.size();
	}

	std::vector<Piece> allRotationPieces; // 将所有角度的零件放入容器中，便于遍历
	for (int i = 0; i < piecesCache.size(); ++i)
	{
		for (int j = 0; j < piecesCache[i].size(); ++j)
			allRotationPieces.push_back(piecesCache[i][j]);
	}

	static NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();
	static GeometryConvert *converter = GeometryConvert::getInstance();

	for (int i = 0; i < allRotationPieces.size(); i++)
	{
		for (int j = i; j < allRotationPieces.size(); j++)
		{

			std::string nfpKey = getNfpKey(allRotationPieces[j], allRotationPieces[i]); // no fit polygon
			if (nfpsCache.find(nfpKey) == nfpsCache.end())
			{
				// 两种方法构造nfp: 滑动法、闵可夫斯基矢量差方法

				// polygon_t nfp = nfpGenerator->minkowskiDifNfp(allRotationPieces[j].polygon, allRotationPieces[i].polygon);

				libnfporb::polygon_t pA = converter->boost2LibNfpPolygon(allRotationPieces[j].polygon);
				libnfporb::polygon_t pB = converter->boost2LibNfpPolygon(allRotationPieces[i].polygon);
				polygon_t nfp = nfpGenerator->slideNfp(pA, pB);

				nfpsCache.insert(std::pair<std::string, polygon_t>(nfpKey, nfp));
			}

			nfpKey = getNfpKey(allRotationPieces[i], allRotationPieces[j]);
			if (nfpsCache.find(nfpKey) == nfpsCache.end())
			{

				// polygon_t nfp = nfpGenerator->minkowskiDifNfp(allRotationPieces[i].polygon, allRotationPieces[j].polygon);
				libnfporb::polygon_t pA = converter->boost2LibNfpPolygon(allRotationPieces[j].polygon);
				libnfporb::polygon_t pB = converter->boost2LibNfpPolygon(allRotationPieces[i].polygon);
				polygon_t nfp = nfpGenerator->slideNfp(pB, pA);

				nfpsCache.insert(std::pair<std::string, polygon_t>(nfpKey, nfp));
			}
		}
	}
	static DataWrite *dataWriter = DataWrite::getInstance();
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

	static NoFitPolygon *nfpGenerator = NoFitPolygon::getInstance();
	for (int i = 0; i < allRotationPieces.size(); ++i)
	{

		std::string ifpKey = getIfrKey(allRotationPieces[i]); // 内接临界矩形

		if (ifpsCache.find(ifpKey) == ifpsCache.end())
		{
			polygon_t ifp = nfpGenerator->generateIfp(bin, allRotationPieces[i].polygon);
			ifpsCache.insert(std::pair<std::string, polygon_t>(ifpKey, ifp));
			box_t ifr;
			bg::envelope(ifp, ifr);
			ifrsCache.insert(std::pair<std::string, box_t>(ifpKey, ifr));
		}
	}
	static DataWrite *dataWriter = DataWrite::getInstance();
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
	const auto &_pieces = piecesCache[0];
	for (int i = 0; i < _pieces.size(); ++i)
	{
		std::string ifrKey = getIfrKey(_pieces[i]);
		polygon_t ifp = ifpsCache[ifrKey];

		Vector curVector;
		if (placedPieces.size() == 0)
		{ // 排放第一个
			curVector.x = Parameters::MAXDOUBLE;
			point_t referPoint = _pieces[i].polygon.outer().front(); // 参考点
			for (auto &point : ifp.outer())
			{
				if (point.x() - referPoint.x() < curVector.x)
				{ // 寻找 ifr 最左边的位置
					curVector = Vector(point.x() - referPoint.x(), point.y() - referPoint.y());
				}
			}
			placedPieces.push_back(_pieces[i]);
			placedVectors.push_back(curVector);
			continue;
		}
#pragma region ClipperExecute
		Paths clipperUnionNfp;
		Paths clipperFinalNfp;
		ClipperLib::Clipper clipperUnion;
		ClipperLib::Clipper clipperDifference;
		// bin_nfp 转换成 clipper paths，即 clipperBinNfp.
		static GeometryConvert *converter = GeometryConvert::getInstance();
		Paths clipperBinNfp = converter->boost2ClipperPolygon(ifp);

		// nfp 转换成 clipper paths, 求并集得到 clipperUnionNfp.
		for (int j = 0; j < placedPieces.size(); ++j)
		{
			std::string key = getNfpKey(placedPieces[j], _pieces[i]);
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
			std::cout << "clipperUnion Execute Failed: " << _pieces[i].id << std::endl;
			continue;
		}

		// clipperBinNfp 经 clipperUnionNfp 裁剪（求差集）得到 clipperFinalNfp.
		clipperDifference.AddPaths(clipperBinNfp, ClipperLib::PolyType::ptSubject, true);
		clipperDifference.AddPaths(clipperUnionNfp, ClipperLib::PolyType::ptClip, true);
		if (!clipperDifference.Execute(ClipperLib::ClipType::ctDifference, clipperFinalNfp, ClipperLib::PolyFillType::pftEvenOdd, ClipperLib::PolyFillType::pftNonZero))
		{
			std::cout << "clipperDifference Execute Failed: " << _pieces[i].id << std::endl;
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
			std::cout << "clipperFinalNfp is empty: " << _pieces[i].id << std::endl;
			continue;
		}
#pragma endregion ClipperExecute

#pragma region Placement
		std::vector<ring_t> finalNfp; // 在 final_nfp 的每个顶点上放置零件
		finalNfp.reserve(clipperFinalNfp.size());
		for (auto &path : clipperFinalNfp)
		{
			finalNfp.push_back(converter->clipper2BoostRing(path));
		}
		point_t mostLeftPoint = findMostLeftPoint(finalNfp); // 计算最左点
		point_t referPoint = _pieces[i].polygon.outer().front();
		curVector = Vector(mostLeftPoint.x() - referPoint.x(), mostLeftPoint.y() - referPoint.y());
		placedPieces.push_back(_pieces[i]);
		placedVectors.push_back(curVector);
#pragma endregion Placement
	}

	double minX = Parameters::MAXDOUBLE, maxX = 0;
	std::vector<polygon_t> nfps;
	for (int i = 0; i < placedPieces.size(); ++i)
	{
		placedPieces[i].translate(placedVectors[i].x, placedVectors[i].y);
		if (placedPieces[i].bounding.min_corner().x() < minX)
		{
			minX = placedPieces[i].bounding.min_corner().x();
		}

		if (placedPieces[i].bounding.max_corner().x() > maxX)
		{
			maxX = placedPieces[i].bounding.max_corner().x();
		}
	}
	return maxX - minX;
}
