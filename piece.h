#ifndef PIECE_H
#define PIECE_H

#include "geometry.h"
#include "vector.h"

namespace MyNest
{

	class Piece
	{

	public:
		Piece();
		bool operator==(const Piece &p) const; // 重载==

	public:
		int id;			   // 零件编号（从0开始）
		int typeId;		   // 零件类型号（从0开始）
		polygon_t polygon; // 多边形
		Angle rotation;	   // 旋转角度（0~360）
		double area;	   // 零件面积
		box_t bounding;	   // 零件外接矩形
		bool defect;	   // 是否有缺陷（孔洞）(目前该算法不适用含有孔洞的零件)

		Vector transVector; // 平移向量
	};
}
#endif // PIECE_H
