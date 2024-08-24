#ifndef VECTOR_H
#define VECTOR_H


namespace MyNest {


	class Vector {
	public:
		Vector();
		Vector(double _x, double _y);

		bool operator == (const Vector &vec) const;
		double operator *(const Vector &vec) const;   // �ڻ�
		Vector operator*(const double a) const;
		Vector operator +(const Vector &vec) const;
		Vector operator -(const Vector &vec) const;
		double cross(const Vector &vec) const;  // ���

	public:
		double x;
		double y;
		static constexpr double eps = 1e-6;
	};

}


#endif // VECTOR_H
