
#include "controller.h"

#include <cmath>

namespace usbl {

HeadingController::HeadingController(ControllerConfig cfg) : cfg_(cfg) {}

void HeadingController::Reset() {
    hasPrevError_ = false;
    prevErrorDeg_ = 0.0;
}

static double BearingDeg_ENU(const Vec3& from, const Vec3& to, bool zeroIsNorth) {
    // ENU: x=East, y=North
    const Vec3 d = to - from;
    const double east = d.x;
    const double north = d.y;

    // If zeroIsNorth: atan2(East, North) yields 0 at North, +90 at East.
    // Else: atan2(North, East) yields 0 at East, +90 at North.
    double yawRad = 0.0;
    if (zeroIsNorth) {
        yawRad = std::atan2(east, north);
    } else {
        yawRad = std::atan2(north, east);
    }
    return RadToDeg(yawRad);
}

bool HeadingController::Compute(const PoseState& current, const Vec3& destinationENU, double dtSeconds, ControlOutput& out) {
    if (!current.hasPosition || !current.hasAttitude) return false;
    if (dtSeconds <= 0.0) return false;

    const double desiredYaw = BearingDeg_ENU(current.positionENU, destinationENU, cfg_.yawZeroIsNorth);
    const double currentYaw = current.yawDeg;

    const double err = WrapAngleDeg(desiredYaw - currentYaw);
    const double derr = hasPrevError_ ? (err - prevErrorDeg_) / dtSeconds : 0.0;

    double steer = cfg_.kpYaw * err + cfg_.kdYaw * derr;
    steer = Clamp(steer, -cfg_.maxSteerDeg, cfg_.maxSteerDeg);

    out.desiredYawDeg = desiredYaw;
    out.headingErrorDeg = err;
    out.steeringDeg = steer;
    out.throttle = cfg_.throttle;

    prevErrorDeg_ = err;
    hasPrevError_ = true;
    return true;
}

} // namespace usbl
