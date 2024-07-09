#include "vector.h"
#include <cmath>

using namespace MyNest;

Vector::Vector() :x(0), y(0) {

}


Vector::Vector(double _x, double _y) : x(_x), y(_y) {

}

bool Vector::operator==(const Vector &vec) const {
	return std::abs(x - vec.x) <= eps && std::abs(y - vec.y) <= eps;
}

double Vector::operator *(const Vector &vec) const {
	return vec.x * x + vec.y * y;
}

Vector Vector::operator*(const double a) const {
	return Vector(x*a, y*a);
}

Vector Vector::operator +(const Vector &vec) const {
	return Vector(vec.x + x, vec.y + y);
}

Vector Vector::operator -(const Vector &vec) const {
	return Vector(x - vec.x, y - vec.y);
}

double Vector::cross(const Vector &vec) const {
	return x * vec.y - y * vec.x;
}
