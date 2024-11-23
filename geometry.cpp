#include "geometry.h"
#include "parameters.h"


using namespace MyNest;

GeometryConvert* GeometryConvert::geometryConverter = nullptr;

GeometryConvert::GeometryConvert() {

}

GeometryConvert* GeometryConvert::getInstance() {
	if (geometryConverter == nullptr) {
		geometryConverter = new GeometryConvert;
	}
	return geometryConverter;
}

IntPoint GeometryConvert::boost2ClipperPoint(const point_t &point) const {
	if (typeid(cInt) == typeid(point.x())) {
		return IntPoint(point.x(), point.y());
	}

	return IntPoint(
		static_cast<cInt>(point.x() * Parameters::scaleRate),
		static_cast<cInt>(point.y() * Parameters::scaleRate)
	);
}

Path GeometryConvert::boost2ClipperLine(const linestring_t &path) const {
	Path retPath;
	for (auto iter = path.begin(); iter != path.end(); ++iter) {
		retPath << boost2ClipperPoint(*iter);
	}
	return retPath;
}

Path GeometryConvert::boost2ClipperRing(const ring_t &path) const {
	Path retPath;
	for (auto iter = path.begin(); iter != path.end() - 1; ++iter) {
		retPath << boost2ClipperPoint(*iter);
	}
	return retPath;
}

Paths GeometryConvert::boost2ClipperPolygon(const polygon_t &poly) const {
	Paths retPaths;
	retPaths.reserve(poly.inners().size() + 1);

	retPaths.push_back(boost2ClipperRing(poly.outer()));

	for (auto & innerRing : poly.inners()) {
		retPaths.push_back(boost2ClipperRing(innerRing));
	}
	return retPaths;
}


point_t GeometryConvert::clipper2BoostPoint(const IntPoint &point) const {
	if (typeid(double) == typeid(point.X)) {
		return point_t(point.X, point.Y);
	}
	return point_t(
		static_cast<double>(point.X / Parameters::scaleRate),
		static_cast<double>(point.Y / Parameters::scaleRate)
	);
}


linestring_t GeometryConvert::clipper2BoostLine(const Path &path) const {
	linestring_t retPath;
	for (auto iter = path.begin(); iter != path.end(); ++iter) {
		bg::append(retPath, clipper2BoostPoint(*iter));
	}
	return retPath;
}


ring_t GeometryConvert::clipper2BoostRing(const Path &path) const {
	ring_t retPath;
	for (auto iter = path.begin(); iter != path.end(); ++iter) {
		bg::append(retPath, clipper2BoostPoint(*iter));
	}
	bg::append(retPath, clipper2BoostPoint(path[0]));
	return retPath;
}


polygon_t GeometryConvert::clipper2BoostPolygon(const Paths &poly) const {
	polygon_t retPoly;
	int biggestIndex = 0;  // 面积最大的 Path 作为外环

	double biggestArea = ClipperLib::Area(poly[0]);
	for (int i = 1; i < poly.size(); ++i) {
		double iArea = ClipperLib::Area(poly[i]);
		biggestIndex = iArea > biggestArea ? i : biggestIndex;
		biggestArea = iArea > biggestArea ? iArea : biggestArea;
	}

	retPoly.outer() = clipper2BoostRing(poly[biggestIndex]);

	retPoly.inners().reserve(poly.size() - 1);
	for (int i = 0; i < poly.size() && i != biggestIndex; ++i) {
		retPoly.inners().push_back(clipper2BoostRing(poly[i]));
	}
	return retPoly;
}


point_t GeometryConvert::libNfp2BoostPoint(const libnfporb::point_t &p) const {
	if (typeid(double) == typeid(p.x_.val())) {
		return point_t(p.x_.val(), p.y_.val());
	}

	double x = boost::numeric::raw_converter<boost::numeric::conversion_traits<double, libnfporb::LongDouble>>::low_level_convert(p.x_);
	double y = boost::numeric::raw_converter<boost::numeric::conversion_traits<double, libnfporb::LongDouble>>::low_level_convert(p.y_);
	return point_t(x, y);
}


linestring_t GeometryConvert::libNfp2BoostLine(const libnfporb::linestring_t& line) const {
	linestring_t res;
	for (auto &p : line) {
		res.push_back(libNfp2BoostPoint(p));
	}
	return res;
}


ring_t GeometryConvert::libNfp2BoostRing(const libnfporb::polygon_t::ring_type &ring) const {
	ring_t res;
	for (auto& p : ring) {
		res.push_back(libNfp2BoostPoint(p));
	}

	return res;
}


polygon_t GeometryConvert::libNfp2BoostPolygon(const libnfporb::polygon_t& polygon)  const {
	polygon_t res;

	res.outer() = libNfp2BoostRing(polygon.outer());

	for (auto& inner : polygon.inners()) {
		res.inners().push_back(libNfp2BoostRing(inner));
	}
	return res;
}


libnfporb::point_t GeometryConvert::boost2LibNfpPoint(const point_t &p) const {
	if (typeid(double) == typeid(libnfporb::coord_t)) {
		return libnfporb::point_t(p.x(), p.y());
	}
	return libnfporb::point_t(static_cast<libnfporb::coord_t>(p.x()), static_cast<libnfporb::coord_t>(p.y()));
}


libnfporb::linestring_t GeometryConvert::boost2LibNfp2Line(const linestring_t& line) const {
	libnfporb::linestring_t res;
	for (auto& p : line) {
		res.push_back(boost2LibNfpPoint(p));
	}
	return res;
}


libnfporb::polygon_t::ring_type GeometryConvert::boost2LibNfpRing(const ring_t &ring) const {
	libnfporb::polygon_t::ring_type res;
	for (auto& p : ring) {
		res.push_back(boost2LibNfpPoint(p));
	}
	return res;
}


libnfporb::polygon_t GeometryConvert::boost2LibNfpPolygon(const polygon_t& polygon) const {
	libnfporb::polygon_t res;
	res.outer() = boost2LibNfpRing(polygon.outer());

	for (auto& inner : polygon.inners()) {
		res.inners().push_back(boost2LibNfpRing(inner));
	}
	return res;
}


// 检测三个点是否共线
//static bool isCollinear(const libnfporb::point_t & p1, const libnfporb::point_t& p2, const libnfporb::point_t& p3, double epsilon = 1e-9) {
//	double area = std::abs((p1.x_ * (p2.y_ - p3.y_) +	p2.x_ * (p3.y_ - p1.y_) + p3.x_ * (p1.y_ - p2.y_)).val());
//
//	return area < epsilon;
//}
//
//// 删除多边形中的共线点
//static void removeCollinearPoints(libnfporb::polygon_t& polygon, double epsilon = 1e-9) {
//	if (polygon.outer().size() < 4) return; // 至少需要4个点，包括首尾重复点
//
//	libnfporb::polygon_t result;
//	result.outer().push_back(polygon.outer()[0]);
//
//	for (size_t i = 1; i < polygon.outer().size() - 1; ++i) {
//		if (!isCollinear(polygon.outer()[i - 1], polygon.outer()[i], polygon.outer()[i + 1], epsilon)) {
//			result.outer().push_back(polygon.outer()[i]);
//		}
//	}
//
//	result.outer().push_back(polygon.outer().back());
//
//	// 如果首尾点也需要判断是否共线（首尾点应当相同）
//	if (isCollinear(result.outer()[result.outer().size() - 2], result.outer().back(), result.outer()[0], epsilon)) {
//		result.outer().pop_back();
//		result.outer().push_back(result.outer()[0]); // 保证首尾点相同
//	}
//	if (isCollinear(result.outer().back(), result.outer()[0], result.outer()[1], epsilon)) {
//		result.outer().erase(result.outer().begin());
//	}
//
//	polygon = result;
//}

polygon_t GeometryConvert::libNfp2BoostPolygon(const libnfporb::nfp_t& nfp) const {
	// 无内环 nfp.size() == 1
	libnfporb::polygon_t nfpPoly;
	for (const auto& pt : nfp.front()) {
		nfpPoly.outer().push_back(pt);
	}
	for (size_t i = 1; i < nfp.size(); ++i) {
		nfpPoly.inners().push_back({});
		for (const auto &pt : nfp[i]) {
			nfpPoly.inners().back().push_back(pt);
		}
	}
	// removeCollinearPoints(nfpPoly);
	return libNfp2BoostPolygon(nfpPoly);
}


