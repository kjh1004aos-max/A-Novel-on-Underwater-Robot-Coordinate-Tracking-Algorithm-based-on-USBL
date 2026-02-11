#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "seatrac.hpp"

namespace usbl {

    struct Vec3 {
        // NED: North (m), East (m), Depth (m; positive down)
        double n = 0.0;
        double e = 0.0;
        double d = 0.0;
    };

    inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.n + b.n, a.e + b.e, a.d + b.d }; }
    inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.n - b.n, a.e - b.e, a.d - b.d }; }
    inline Vec3 operator*(double s, const Vec3& v) { return { s * v.n, s * v.e, s * v.d }; }

    inline double norm(const Vec3& v) { return std::sqrt(v.n * v.n + v.e * v.e + v.d * v.d); }

    inline double wrap_deg_0_360(double deg) {
        double out = std::fmod(deg, 360.0);
        if (out < 0.0) out += 360.0;
        return out;
    }

    inline double wrap_deg_pm180(double deg) {
        double out = std::fmod(deg + 180.0, 360.0);
        if (out < 0.0) out += 360.0;
        return out - 180.0;
    }

    inline double deg2rad(double deg) { return deg * M_PI / 180.0; }

    // Computes a "virtual" relative NED position from Range + (Azimuth, Elevation).
    // - Uses the SeaTrac definition where azimuth is 0..360 and elevation is -90..+90.
    // - Converts to NED where Depth is positive down.
    //
    // Notes:
    // - This does not apply AHRS attitude corrections. It is intentionally a "raw geometry" conversion
    //   that can be compared against the beacon-provided POSITION_* fields.
    inline std::optional<Vec3> virtual_ned_from_fix(const seatrac::AcousticFix& fix) {
        if (!fix.range_dist_m.has_value() || !fix.usbl_azimuth_deg.has_value() || !fix.usbl_elevation_deg.has_value()) {
            return std::nullopt;
        }

        const double r = *fix.range_dist_m;
        const double az = deg2rad(*fix.usbl_azimuth_deg);
        const double el = deg2rad(*fix.usbl_elevation_deg);

        // Horizontal projection
        const double horiz = r * std::cos(el);

        Vec3 out;
        out.n = horiz * std::cos(az); // 0 deg azimuth = North
        out.e = horiz * std::sin(az);

        // Elevation positive above horizontal plane; for targets below, el is negative.
        // Depth positive down => d = -r * sin(el)
        out.d = -r * std::sin(el);

        // Convert to depth-from-surface if desired (approx): add local depth.
        // The paper uses relative coordinates; we keep relative NED by default.
        // Uncomment if you want absolute depth: out.d = fix.local_depth_m + out.d;
        return out;
    }

    inline std::optional<Vec3> measured_ned_from_fix(const seatrac::AcousticFix& fix) {
        if (!fix.pos_northing_m.has_value() || !fix.pos_easting_m.has_value() || !fix.pos_depth_m.has_value()) {
            return std::nullopt;
        }
        return Vec3{ *fix.pos_northing_m, *fix.pos_easting_m, *fix.pos_depth_m };
    }

    struct TrackingDecision {
        Vec3 chosen;
        bool used_measured = false;
        bool used_virtual = false;
        bool rejected_measured = false;
        bool rejected_virtual = false;
        double measured_vs_virtual_rel_error = 0.0;
    };

    // Implements the paper's core correction logic:
    // - Compute measured position from beacon reply (Pm)
    // - Compute virtual position from USBL signal info (Pv)
    // - If error >= threshold, choose the one closer to previous path point.
    //
    // Practical extensions:
    // - If the SeaTrac POSITION_FLT_ERROR bit is set, we treat the measured fix as "suspect".
    // - If USBL fit-error is large, we treat virtual estimate as suspect.
    class PathTracker {
    public:
        struct Params {
            double rel_error_threshold;
            double max_usbl_fit_error;
            size_t max_path_len;
            Params() : rel_error_threshold(0.05), max_usbl_fit_error(3.0), max_path_len(100000) {}
        };

        PathTracker() : PathTracker(Params()) {}

        explicit PathTracker(Params p) : p_(p) {}

        void reset() {
            path_.clear();
            have_prev_ = false;
            prev_ = {};
        }

        const std::vector<Vec3>& path() const noexcept { return path_; }
        bool has_prev() const noexcept { return have_prev_; }
        Vec3 prev() const {
            if (!have_prev_) throw std::runtime_error("PathTracker: no previous point");
            return prev_;
        }

        // Update from a fix and return the decision.
        TrackingDecision update(const seatrac::AcousticFix& fix) {
            const auto measured = measured_ned_from_fix(fix);
            const auto virt = virtual_ned_from_fix(fix);

            TrackingDecision out;

            // If both missing, hold last.
            if (!measured && !virt) {
                if (have_prev_) {
                    out.chosen = prev_;
                }
                else {
                    out.chosen = { 0, 0, 0 };
                }
                out.rejected_measured = true;
                out.rejected_virtual = true;
                append(out.chosen);
                return out;
            }

            // Pre-screen quality
            bool measured_ok = measured.has_value();
            bool virt_ok = virt.has_value();

            if (fix.position_filter_error()) {
                // SeaTrac position filter indicates the position may be invalid.
                measured_ok = false;
            }

            if (fix.usbl_fit_error.has_value() && (*fix.usbl_fit_error > p_.max_usbl_fit_error)) {
                virt_ok = false;
            }

            // Decide.
            if (measured_ok && !virt_ok) {
                out.chosen = *measured;
                out.used_measured = true;
                out.rejected_virtual = virt.has_value();
            }
            else if (!measured_ok && virt_ok) {
                out.chosen = *virt;
                out.used_virtual = true;
                out.rejected_measured = measured.has_value();
            }
            else {
                // Both candidates are available (or both suspect). Apply paper logic.
                const Vec3 pm = measured.value_or(Vec3{});
                const Vec3 pv = virt.value_or(Vec3{});

                const double denom = std::max(norm(pm), 1e-6);
                const double rel_err = norm(pm - pv) / denom;
                out.measured_vs_virtual_rel_error = rel_err;

                if (!have_prev_) {
                    // No history yet: prefer measured if present.
                    if (measured.has_value()) {
                        out.chosen = pm;
                        out.used_measured = true;
                    }
                    else {
                        out.chosen = pv;
                        out.used_virtual = true;
                    }
                }
                else if (rel_err >= p_.rel_error_threshold) {
                    // Choose whichever is closer to previous point (paper).
                    const double d_pm = norm(prev_ - pm);
                    const double d_pv = norm(prev_ - pv);
                    if (d_pm <= d_pv) {
                        out.chosen = pm;
                        out.used_measured = true;
                    }
                    else {
                        out.chosen = pv;
                        out.used_virtual = true;
                    }
                }
                else {
                    // Error is small: use measured (paper).
                    out.chosen = pm;
                    out.used_measured = true;
                }
            }

            append(out.chosen);
            return out;
        }

    private:
        void append(const Vec3& p) {
            if (path_.size() >= p_.max_path_len) {
                // Keep the newest points.
                path_.erase(path_.begin(), path_.begin() + static_cast<long>(path_.size() / 2));
            }
            path_.push_back(p);
            prev_ = p;
            have_prev_ = true;
        }

        Params p_;
        std::vector<Vec3> path_;
        Vec3 prev_{};
        bool have_prev_ = false;
    };

    // A minimal heading controller: compute desired heading to a destination (in NED),
    // compare against current yaw, output a steering command.
    class HeadingController {
    public:
        struct Params {
            double kp = 1.0;                 // proportional gain (deg->deg or deg->normalized)
            double max_cmd = 30.0;           // maximum steering command (degrees)
            double deadband_deg = 2.0;       // no actuation within this error
        };

        HeadingController() : HeadingController(Params()) {}

        explicit HeadingController(Params p) : p_(p) {}

        // Returns steering command in degrees (e.g., rudder angle). Positive -> turn right.
        double compute_steering_deg(const Vec3& current, const Vec3& destination, double current_yaw_deg) const {
            const Vec3 diff = destination - current;
            const double desired_heading = wrap_deg_0_360(std::atan2(diff.e, diff.n) * 180.0 / M_PI);
            const double err = wrap_deg_pm180(desired_heading - current_yaw_deg);

            if (std::abs(err) <= p_.deadband_deg) {
                return 0.0;
            }

            double cmd = p_.kp * err;
            cmd = std::clamp(cmd, -p_.max_cmd, p_.max_cmd);
            return cmd;
        }

    private:
        Params p_;
    };

    // Application-level control packet for the robot (sent over SeaTrac DAT protocol).
    // We design a compact, fixed-size payload to keep acoustic transmission time low.
    struct ControlPacket {
        uint8_t version = 1;
        uint8_t msg_type = 1;  // 1 = steering/speed command
        uint8_t seq = 0;
        uint8_t flags = 0;
        int16_t steering_deci_deg = 0; // -300..+300 => -30.0..+30.0 deg
        uint16_t speed_cms = 0;        // forward speed setpoint (cm/s)

        static constexpr size_t kSize = 8;

        std::vector<uint8_t> encode() const {
            std::vector<uint8_t> b;
            b.reserve(kSize);
            b.push_back(version);
            b.push_back(msg_type);
            b.push_back(seq);
            b.push_back(flags);

            auto push_i16 = [&b](int16_t v) {
                b.push_back(static_cast<uint8_t>(v & 0xFF));
                b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            };
            auto push_u16 = [&b](uint16_t v) {
                b.push_back(static_cast<uint8_t>(v & 0xFF));
                b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            };

            push_i16(steering_deci_deg);
            push_u16(speed_cms);

            if (b.size() != kSize) {
                throw std::runtime_error("ControlPacket encode: size mismatch");
            }
            return b;
        }

        static ControlPacket decode(const std::vector<uint8_t>& b) {
            if (b.size() < kSize) {
                throw std::runtime_error("ControlPacket decode: packet too short");
            }
            ControlPacket p;
            p.version = b[0];
            p.msg_type = b[1];
            p.seq = b[2];
            p.flags = b[3];

            auto read_i16 = [&b](size_t off) {
                uint16_t u = static_cast<uint16_t>(b[off] | (static_cast<uint16_t>(b[off + 1]) << 8));
                return static_cast<int16_t>(u);
            };
            auto read_u16 = [&b](size_t off) {
                return static_cast<uint16_t>(b[off] | (static_cast<uint16_t>(b[off + 1]) << 8));
            };

            p.steering_deci_deg = read_i16(4);
            p.speed_cms = read_u16(6);
            return p;
        }
    };

} // namespace usbl
