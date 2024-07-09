#ifndef PIECE_H
#define PIECE_H

#include "geometry.h"
#include "vector.h"

namespace MyNest {

	class Piece {

	public:
		Piece();
		bool operator ==(const Piece &p) const;   // 重载==

		void translate(double x, double y); // 平移零件
		void rotate(Angle angle);           // 旋转零件
		void offset(double scale);          // 放缩零件
		void clean();   // 简化多边形

		void getEnvelope();    // 获取零件的外界举行<width, height>
		point_t grivaty()     const;    // 计算零件重心
		double  signedArea()  const;    // "签名面积",如果签名面积为正，多边形的顶点是逆时针顺序的
										// 如果签名面积为负，多边形的顶点是顺时针顺序的。

	public:
		int id;        // 零件编号（从1开始）
		int typeId;    // 零件类型号（从1开始）
		polygon_t polygon;      // 多边形
		Angle rotation;         // 旋转角度（0~360）
		double area;            // 零件面积
		box_t bounding;         // 零件外接矩形
		bool defect;            // 是否有缺陷（孔洞）

		Vector transVector;		// 零件的平移向量
	};
}
#endif // PIECE_H
