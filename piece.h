#ifndef PIECE_H
#define PIECE_H

#include "geometry.h"
#include "vector.h"

namespace MyNest {

	class Piece {

	public:
		Piece();
		bool operator ==(const Piece &p) const;   // ����==

		void translate(double x, double y); // ƽ�����
		void rotate(Angle angle);           // ��ת���
		void offset(double scale);          // �������
		void clean();   // �򻯶����

		void getEnvelope();    // ��ȡ�����������<width, height>
		point_t grivaty()     const;    // �����������
		double  signedArea()  const;    // "ǩ�����",���ǩ�����Ϊ��������εĶ�������ʱ��˳���
										// ���ǩ�����Ϊ��������εĶ�����˳ʱ��˳��ġ�

	public:
		int id;        // �����ţ���1��ʼ��
		int typeId;    // ������ͺţ���1��ʼ��
		polygon_t polygon;      // �����
		Angle rotation;         // ��ת�Ƕȣ�0~360��
		double area;            // ������
		box_t bounding;         // �����Ӿ���
		bool defect;            // �Ƿ���ȱ�ݣ��׶���

		Vector transVector;		// �����ƽ������
	};
}
#endif // PIECE_H
