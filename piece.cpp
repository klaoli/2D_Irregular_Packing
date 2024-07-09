#include "piece.h"
#include "parameters.h"
using namespace MyNest;

Piece::Piece() :
	id(-1), typeId(-1), rotation(0), defect(false) {

}


bool Piece::operator ==(const Piece &p) const {
	return this->id == p.id;
}


void Piece::getEnvelope() {
	double minX = polygon.outer().front().x(), maxX = polygon.outer().front().x(),
		minY = polygon.outer().front().y(), maxY = polygon.outer().front().y();
	for (int i = 1; i < polygon.outer().size(); ++i) {
		if (polygon.outer()[i].x() < minX) {
			minX = polygon.outer()[i].x();
		}
		else if (polygon.outer()[i].x() > maxX) {
			maxX = polygon.outer()[i].x();
		}

		if (polygon.outer()[i].y() < minY) {
			minY = polygon.outer()[i].y();
		}
		else if (polygon.outer()[i].y() > maxY) {
			maxY = polygon.outer()[i].y();
		}
	}
	bounding = box_t(point_t(minX, minY), point_t(maxX, maxY));
}



double Piece::signedArea() const {
	if (polygon.outer().size() < 3) return 0.0;

	double area = 0.0;
	for (size_t i = 0; i < polygon.outer().size() - 1; ++i) {
		area += polygon.outer()[i].x() * polygon.outer()[i + 1].y();
		area -= polygon.outer()[i + 1].x() * polygon.outer()[i].y();
	}

	area += polygon.outer().back().x() * polygon.outer().front().y();
	area -= polygon.outer().front().x() * polygon.outer().back().y();

	return area / 2.0;
}


void Piece::translate(double dx, double dy) {
	polygon_t output;

	bg::strategy::transform::translate_transformer<double, 2, 2> translate(dx, dy);

	bg::transform(polygon, output, translate);

	polygon = output;

	// 处理外接矩形
	getEnvelope();
}


void Piece::rotate(Angle angle) {
	polygon_t output;

	bg::strategy::transform::rotate_transformer<bg::degree, double, 2, 2> rotate_strategy(angle);

	bg::transform(polygon, output, rotate_strategy);

	polygon = output;

	rotation = angle;

	// 处理外接矩形
	getEnvelope();
}



void Piece::offset(double scale) {
	static GeometryConvert *converter = GeometryConvert::getInstance();
	Paths paths = converter->boost2ClipperPolygon(polygon);

	Paths output;
	// JoinType = jtMiter，超出倍数截断尖角，MiterLimit = 2.0
	// JoinType = jtRound，使用弧线包裹尖角, ArcTolerance = Config::curveTolerance * Config::scaleRate
	ClipperLib::ClipperOffset co(2.0, Parameters::curveTolerance * Parameters::scaleRate);
	co.AddPaths(paths, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
	co.Execute(output, scale * Parameters::scaleRate);
	
	assert(output.size() == 1);

	polygon = converter->clipper2BoostPolygon(output);
	area = bg::area(polygon);

	// 处理外接矩形
	getEnvelope();

}


void Piece::clean() {
	static GeometryConvert *converter = GeometryConvert::getInstance();
	Paths paths = converter->boost2ClipperPolygon(polygon);

	// 删除自相交点
	ClipperLib::SimplifyPolygons(paths, ClipperLib::PolyFillType::pftEvenOdd);

	// 简化多边形，删除共线点
	CleanPolygons(paths, Parameters::curveTolerance * Parameters::scaleRate);
	assert(paths.size() == 1);
	polygon = converter->clipper2BoostPolygon(paths);
	area = bg::area(polygon);

	// 处理外接矩形
	getEnvelope();
}




