#include "piece.h"
#include "parameters.h"
using namespace MyNest;

Piece::Piece() : id(-1), typeId(-1), rotation(0), defect(false)
{
}

bool Piece::operator==(const Piece &p) const
{
	return this->id == p.id;
}

// void Piece::clean()
// {
//  GeometryConvert *converter = GeometryConvert::getInstance();
// 	Paths paths = converter->boost2ClipperPolygon(polygon);
// 	删除自相交点
// 	ClipperLib::SimplifyPolygons(paths, ClipperLib::PolyFillType::pftEvenOdd);
// 	简化多边形，删除共线点
// 	CleanPolygons(paths, Parameters::curveTolerance * Parameters::scaleRate);
// 	assert(paths.size() == 1);
// 	polygon = converter->clipper2BoostPolygon(paths);
// 	area = bg::area(polygon);
// 	处理外接矩形
// 	getEnvelope();
// }
