
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace usbl {

// Mathematical constants
constexpr double kPi = 3.1415926535897932384626433832795;

// Angle conversions
inline double DegToRad(double deg) { return deg * (kPi / 180.0); }
inline double RadToDeg(double rad) { return rad * (180.0 / kPi); }

// Numeric helpers
inline double Clamp(double v, double lo, double hi) { return std::max(lo, std::min(v, hi)); }

inline double WrapAngleDeg(double deg) {
    // Wrap to [-180, 180)
    double a = std::fmod(deg + 180.0, 360.0);
    if (a < 0.0) a += 360.0;
    return a - 180.0;
}

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return Vec3{x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return Vec3{x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return Vec3{x * s, y * s, z * s}; }
};

inline double Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline double Norm(const Vec3& v) { return std::sqrt(Dot(v, v)); }

inline Vec3 Normalize(const Vec3& v) {
    const double n = Norm(v);
    if (n <= 1e-12) return Vec3{0.0, 0.0, 0.0};
    return v * (1.0 / n);
}

} // namespace usbl
