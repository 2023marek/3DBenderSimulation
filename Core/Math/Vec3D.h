#pragma once
#include <cmath>

struct Vec3D
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    // =========================
    // BASIC OPERATORS
    // =========================
    Vec3D operator+(const Vec3D& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3D operator-(const Vec3D& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3D operator*(double s) const { return { x * s, y * s, z * s }; }

    Vec3D& operator+=(const Vec3D& v)
    {
        x += v.x; y += v.y; z += v.z;
        return *this;
    }
};

// =========================
// MATH FUNCTIONS
// =========================
inline double dot(const Vec3D& a, const Vec3D& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3D cross(const Vec3D& a, const Vec3D& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline double length(const Vec3D& v)
{
    return std::sqrt(dot(v, v));
}

inline Vec3D normalize(const Vec3D& v)
{
    double len = length(v);
    if (len == 0.0) return { 0,0,0 };
    return { v.x / len, v.y / len, v.z / len };
}