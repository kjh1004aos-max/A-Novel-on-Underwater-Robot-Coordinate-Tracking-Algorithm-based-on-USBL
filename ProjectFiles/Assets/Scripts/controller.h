
#pragma once
#include "math_utils.h"
#include "tracker.h"

namespace usbl {

// Simple heading controller inspired by the manuscript's "azimuth-based steering correction":
// - Determine heading error between current yaw (from AHRS) and desired yaw toward destination
// - Output a steering command (degrees) with PD control
struct ControllerConfig {
    double kpYaw = 1.0;   // proportional gain (deg->deg)
    double kdYaw = 0.1;   // derivative gain (deg/s -> deg)
    double maxSteerDeg = 30.0; // clamp steering command
    double throttle = 0.5;     // normalized [0..1] (placeholder for your vehicle)
    bool use3D = false;        // if false, compute yaw in horizontal plane only
    bool yawZeroIsNorth = true; // match TrackerConfig azimuth convention
};

struct ControlOutput {
    double desiredYawDeg = 0.0;
    double headingErrorDeg = 0.0;
    double steeringDeg = 0.0;
    double throttle = 0.0;
};

class HeadingController final {
public:
    explicit HeadingController(ControllerConfig cfg);

    void SetConfig(const ControllerConfig& cfg) { cfg_ = cfg; }
    const ControllerConfig& Config() const { return cfg_; }

    void Reset();

    // Compute control output.
    // - current: fused pose (position + yaw)
    // - destinationENU: desired waypoint in ENU
    // - dtSeconds: update timestep (must be >0)
    bool Compute(const PoseState& current, const Vec3& destinationENU, double dtSeconds, ControlOutput& out);

private:
    ControllerConfig cfg_;
    bool hasPrevError_ = false;
    double prevErrorDeg_ = 0.0;
};

} // namespace usbl
