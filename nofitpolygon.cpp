#include "nofitpolygon.h"
#include "libnfporb-master/src/libnfporb.hpp"
using namespace MyNest;

NoFitPolygon *NoFitPolygon::nfp = nullptr;

NoFitPolygon::NoFitPolygon()
{
}

NoFitPolygon *NoFitPolygon::getInstance()
{
	if (nfp == nullptr)
	{
		nfp = new NoFitPolygon;
	}
	return nfp;
}

polygon_t NoFitPolygon::minkowskiDifNfp(const polygon_t &pA, const polygon_t &pB) const
{
	static GeometryConvert *converter = GeometryConvert::getInstance();
	Path path1 = converter->boost2ClipperRing(pA.outer());
	Path path2 = converter->boost2ClipperRing(pB.outer());
	for (auto &p : path2)
	{
		p.X = -p.X;
		p.Y = -p.Y;
	}
	Paths clipperNfp;
	MinkowskiSum(path1, path2, clipperNfp, true);

	polygon_t boostNfp = converter->clipperNfp2Boost(clipperNfp);

	for (int i = 0; i < boostNfp.outer().size(); i++)
	{
		boostNfp.outer()[i].set<0>(boostNfp.outer()[i].x() + pB.outer().front().x());
		boostNfp.outer()[i].set<1>(boostNfp.outer()[i].y() + pB.outer().front().y());
	}
	return boostNfp;
}

polygon_t NoFitPolygon::slideNfp(libnfporb::polygon_t &polyA, libnfporb::polygon_t &polyB) const
{

	libnfporb::nfp_t nfp = libnfporb::generate_nfp(polyA, polyB);

	// libnfporb::write_svg("nfp.svg", polyA, polyB, nfp);

	static GeometryConvert *converter = GeometryConvert::getInstance();

	return converter->libNfp2BoostPolygon(nfp);
}

polygon_t NoFitPolygon::generateIfp(const box_t &bin, const polygon_t &poly) const
{
	box_t envelope;
	bg::envelope(poly, envelope);
	auto &refer_point = poly.outer().front();
	polygon_t ifr;
	bg::append(ifr.outer(), point_t(bin.min_corner().x() - envelope.min_corner().x() + refer_point.x(),
									bin.min_corner().y() - envelope.min_corner().y() + refer_point.y()));
	bg::append(ifr.outer(), point_t(bin.max_corner().x() - envelope.max_corner().x() + refer_point.x(),
									bin.min_corner().y() - envelope.min_corner().y() + refer_point.y()));
	bg::append(ifr.outer(), point_t(bin.max_corner().x() - envelope.max_corner().x() + refer_point.x(),
									bin.max_corner().y() - envelope.max_corner().y() + refer_point.y()));
	bg::append(ifr.outer(), point_t(bin.min_corner().x() - envelope.min_corner().x() + refer_point.x(),
									bin.max_corner().y() - envelope.max_corner().y() + refer_point.y()));
	bg::append(ifr.outer(), point_t(bin.min_corner().x() - envelope.min_corner().x() + refer_point.x(),
									bin.min_corner().y() - envelope.min_corner().y() + refer_point.y()));
	bg::correct(ifr);
	return ifr;
}
