#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "include/clipper.hpp"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include "libnfporb-master/src/geometry.hpp"


namespace MyNest {
	using Angle = double;
	constexpr double PI = 3.14159265358979323846;

	namespace bg = boost::geometry;
	// n ά������
	template<size_t dimension = 2>
	using point_base = bg::model::point<double, dimension, bg::cs::cartesian>;


	/*****************************
		* ���¶���ȫ����Զ�ά������ *
		******************************/
		// ��ά�����
	using point_t = bg::model::d2::point_xy<double>;

	// ����
	using linestring_t = bg::model::linestring<point_t>;

	// ����Σ���ʱ�룬���=�յ㣬����0�����߶���ڻ� inner rings��
	using polygon_t = bg::model::polygon<point_t, false, true>;

	// ������ʱ�룬���=�յ㣩
	//using ring_t = bg::model::ring<point_t, false, true>;
	using ring_t = polygon_t::ring_type;

	// �㼯��
	using multi_point_t = bg::model::multi_point<point_t>;

	// ���߼���
	using multi_linestring_t = bg::model::multi_linestring<linestring_t>;

	// ����μ���
	using multi_polygon_t = bg::model::multi_polygon<polygon_t>;

	// ���Σ�min_corner(), max_corner()��
	using box_t = bg::model::box<point_t>;

	// �߶�(���㹹���߶�)
	using segment_t = bg::model::segment<point_t>;

	// ClipperLib��������
	using cInt = ClipperLib::cInt;
	using IntPoint = ClipperLib::IntPoint;
	using Path = ClipperLib::Path;
	using Paths = ClipperLib::Paths;

	class GeometryConvert {
	public:
		static GeometryConvert *geometryConverter;
		static GeometryConvert* getInstance();

		// clipper lib to boost lib
		point_t      clipper2BoostPoint(const IntPoint &point) const;
		linestring_t clipper2BoostLine(const Path &path)       const;
		ring_t       clipper2BoostRing(const Path &path)       const;
		polygon_t    clipper2BoostPolygon(const Paths &poly)   const;

		// boost lib to clipper lib
		IntPoint boost2ClipperPoint(const point_t &point)     const;
		Path     boost2ClipperLine(const linestring_t &path)  const;
		Path     boost2ClipperRing(const ring_t &path)        const;
		Paths    boost2ClipperPolygon(const polygon_t &poly)  const;

		// libnfporb to boost lib
		point_t libNfp2BoostPoint(const libnfporb::point_t &p)        const;
		linestring_t libNfp2BoostLine(const libnfporb::linestring_t&) const;
		ring_t libNfp2BoostRing(const libnfporb::polygon_t::ring_type&) const;
		polygon_t libNfp2BoostPolygon(const libnfporb::polygon_t&)  const;

		polygon_t libNfp2BoostPolygon(const libnfporb::nfp_t&) const;

		// boost lib to libnfporb
		libnfporb::point_t boost2LibNfpPoint(const point_t &p)        const;
		libnfporb::linestring_t boost2LibNfp2Line(const linestring_t&) const;
		libnfporb::polygon_t::ring_type boost2LibNfpRing(const ring_t &) const;
		libnfporb::polygon_t boost2LibNfpPolygon(const polygon_t&) const;

	private:
		GeometryConvert();
		GeometryConvert(const GeometryConvert&) = delete;
		void operator=(const GeometryConvert&) = delete;
	};
}


#endif // GEOMETRY_H

