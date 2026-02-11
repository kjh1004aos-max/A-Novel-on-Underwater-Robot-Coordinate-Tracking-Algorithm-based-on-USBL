
#include "tracker.h"

#include <cmath>

namespace usbl {

UsbLTracker::UsbLTracker(TrackerConfig cfg) : cfg_(cfg) {}

void UsbLTracker::Reset() {
    latest_.reset();
    path_.clear();
}

static Vec3 SphericalToENU(const TrackerConfig& cfg, double azDeg, double elDeg, double distMeters) {
    // Convert spherical coordinates (azimuth, elevation, distance) into ENU Cartesian.
    //
    // Assumptions:
    // - ENU frame: +X = East, +Y = North, +Z = Up.
    // - Azimuth convention:
    //     * If azimuthZeroIsNorth: az=0 -> +Y (North), az=90 -> +X (East).
    //     * Else: az=0 -> +X, az=90 -> +Y.
    // - Elevation:
    //     * If elevationPositiveDown: el>0 -> downwards. We convert to Up by negating.
    //
    // Distance:
    // - If distanceMode == SlantRange: distMeters is 3D range.
    // - If distanceMode == HorizontalRange: distMeters is horizontal range.

    const double az = DegToRad(azDeg);
    double el = DegToRad(elDeg);
    if (cfg.elevationPositiveDown) {
        // Convert "down-positive" elevation to "up-positive" for ENU
        el = -el;
    }

    double horiz = 0.0;
    double up = 0.0;

    if (cfg.distanceMode == DistanceMode::SlantRange) {
        horiz = distMeters * std::cos(el);
        up    = distMeters * std::sin(el);
    } else {
        // Horizontal range given
        horiz = distMeters;
        up    = distMeters * std::tan(el);
    }

    double east = 0.0;
    double north = 0.0;

    if (cfg.azimuthZeroIsNorth) {
        // Bearing-like: az=0->North (+Y), az=90->East (+X)
        east  = horiz * std::sin(az);
        north = horiz * std::cos(az);
    } else {
        // Mathematical: az=0->East (+X), az=90->North (+Y)
        east  = horiz * std::cos(az);
        north = horiz * std::sin(az);
    }

    return Vec3{east, north, up};
}

bool UsbLTracker::ComputeMeasuredPosition(const Measurement& meas, Vec3& outENU) const {
    if (!meas.hasSpherical) return false;
    outENU = SphericalToENU(cfg_, meas.azimuthDeg, meas.elevationDeg, meas.distanceMeters);
    return true;
}

bool UsbLTracker::ComputeVirtualPosition(const UsbLSoundInfo& usblInfo, double fallbackRangeMeters, Vec3& outENU) const {
    // This implements the idea from the paper:
    // - Use phase differences between closely spaced elements to estimate arrival angles (Eq. (4))
    // - Use time-of-flight to estimate range (range = c * t)
    // - Convert to Cartesian position (Eq. (5)), then compare against measured coordinates for validation
    //
    // Because the exact X150/X110 raw protocol isn't part of the manuscript, we implement
    // a generic 2D baseline array variant:
    //   ux ≈ phaseSign * (Δφ_x * c) / (2π f d_x)
    //   uy ≈ phaseSign * (Δφ_y * c) / (2π f d_y)
    //   uz = -sqrt(1 - ux^2 - uy^2)   (default: assume source is below; ENU z is Up)
    // Then position = range * [ux, uy, uz]
    //
    // You should calibrate phaseSign and sensor spacings to match your hardware.
    if (!usblInfo.hasCarrierHz || !usblInfo.hasSoundSpeed) return false;
    if ((!usblInfo.hasPhaseDiffX || !usblInfo.hasSensorSpacingX) &&
        (!usblInfo.hasPhaseDiffY || !usblInfo.hasSensorSpacingY)) {
        return false;
    }

    const double f = usblInfo.carrierHz;
    const double c = usblInfo.soundSpeed;
    if (f <= 0.0 || c <= 0.0) return false;

    // Compute direction cosines in ENU x/y using available baselines.
    double ux = 0.0;
    double uy = 0.0;

    if (usblInfo.hasPhaseDiffX && usblInfo.hasSensorSpacingX && usblInfo.sensorSpacingX > 0.0) {
        ux = static_cast<double>(cfg_.phaseSign) * (usblInfo.phaseDiffXRad * c) / (2.0 * kPi * f * usblInfo.sensorSpacingX);
        ux = Clamp(ux, -1.0, 1.0);
    }

    if (usblInfo.hasPhaseDiffY && usblInfo.hasSensorSpacingY && usblInfo.sensorSpacingY > 0.0) {
        uy = static_cast<double>(cfg_.phaseSign) * (usblInfo.phaseDiffYRad * c) / (2.0 * kPi * f * usblInfo.sensorSpacingY);
        uy = Clamp(uy, -1.0, 1.0);
    }

    // Determine uz magnitude
    const double xy2 = ux * ux + uy * uy;
    double uz = 0.0;
    if (xy2 >= 1.0) {
        // Direction is essentially horizontal (or numeric overflow). Set uz=0.
        uz = 0.0;
        const double inv = 1.0 / std::sqrt(xy2);
        ux *= inv;
        uy *= inv;
    } else {
        const double uzAbs = std::sqrt(std::max(0.0, 1.0 - xy2));
        // Default assumption: slave is underwater -> position z should be negative in ENU (down).
        uz = -uzAbs;
    }

    // Range estimation
    double range = fallbackRangeMeters;
    if (usblInfo.hasTof) {
        const double r = c * usblInfo.timeOfFlightSec;
        if (r > 0.0) range = r;
    }
    if (range <= 0.0) return false;

    outENU = Vec3{ux, uy, uz} * range;
    return true;
}

double UsbLTracker::ErrorRatio(const Vec3& measured, const Vec3& virt) {
    const double denom = std::max(Norm(measured), 1e-6);
    return Norm(measured - virt) / denom;
}

bool UsbLTracker::Update(const Measurement& meas, const UsbLSoundInfo& usblInfo) {
    Vec3 measuredENU{};
    Vec3 virtualENU{};
    const bool hasMeasured = ComputeMeasuredPosition(meas, measuredENU);

    // Use measured slant range as fallback for virtual if no tof.
    double fallbackRange = 0.0;
    if (hasMeasured) {
        if (cfg_.distanceMode == DistanceMode::SlantRange) {
            fallbackRange = meas.distanceMeters;
        } else {
            // Convert horizontal range to an approximate slant (if elevation exists)
            double el = DegToRad(meas.elevationDeg);
            if (cfg_.elevationPositiveDown) el = -el;
            const double cosEl = std::cos(el);
            if (std::abs(cosEl) > 1e-6) {
                fallbackRange = meas.distanceMeters / cosEl;
            } else {
                fallbackRange = meas.distanceMeters;
            }
        }
    }

    const bool hasVirtual = ComputeVirtualPosition(usblInfo, fallbackRange, virtualENU);

    // If we have no position data at all, just update attitude if present.
    if (!hasMeasured && !hasVirtual) {
        if (latest_.has_value() && meas.hasAttitude) {
            PoseState st = latest_.value();
            st.yawDeg = meas.yawDeg;
            st.rollDeg = meas.rollDeg;
            st.pitchDeg = meas.pitchDeg;
            st.hasAttitude = true;
            latest_ = st;
        }
        return false;
    }

    // Choose P based on Algorithm 1 described in the manuscript.
    Vec3 chosen = hasMeasured ? measuredENU : virtualENU;

    if (hasMeasured && hasVirtual) {
        const double err = ErrorRatio(measuredENU, virtualENU);
        if (err >= cfg_.errorThresholdRatio) {
            if (!path_.empty()) {
                const Vec3 prev = path_.back();
                const double dM = Norm(prev - measuredENU);
                const double dV = Norm(prev - virtualENU);
                chosen = (dM <= dV) ? measuredENU : virtualENU;
            } else {
                // First sample: prefer measured
                chosen = measuredENU;
            }
        } else {
            chosen = measuredENU;
        }
    }

    // Update latest state
    PoseState st{};
    st.positionENU = chosen;
    st.hasPosition = true;

    if (meas.hasAttitude) {
        st.yawDeg = meas.yawDeg;
        st.rollDeg = meas.rollDeg;
        st.pitchDeg = meas.pitchDeg;
        st.hasAttitude = true;
    } else if (latest_.has_value() && latest_->hasAttitude) {
        st.yawDeg = latest_->yawDeg;
        st.rollDeg = latest_->rollDeg;
        st.pitchDeg = latest_->pitchDeg;
        st.hasAttitude = true;
    }

    latest_ = st;

    // Append to path
    path_.push_back(chosen);
    while (path_.size() > cfg_.maxPathPoints) {
        path_.pop_front();
    }
    return true;
}

// Fleet tracker

UsbLFleetTracker::UsbLFleetTracker(TrackerConfig cfg) : cfg_(cfg) {}

void UsbLFleetTracker::SetConfig(const TrackerConfig& cfg) {
    cfg_ = cfg;
    for (auto& kv : trackers_) {
        kv.second.SetConfig(cfg_);
    }
}

UsbLTracker& UsbLFleetTracker::GetOrCreate(int slaveId) {
    auto it = trackers_.find(slaveId);
    if (it != trackers_.end()) return it->second;
    auto inserted = trackers_.emplace(slaveId, UsbLTracker(cfg_));
    return inserted.first->second;
}

bool UsbLFleetTracker::Update(int slaveId, const Measurement& meas, const UsbLSoundInfo& usblInfo) {
    return GetOrCreate(slaveId).Update(meas, usblInfo);
}

bool UsbLFleetTracker::HasLatest(int slaveId) const {
    auto it = trackers_.find(slaveId);
    if (it == trackers_.end()) return false;
    return it->second.HasLatest();
}

PoseState UsbLFleetTracker::Latest(int slaveId) const {
    auto it = trackers_.find(slaveId);
    if (it == trackers_.end()) return PoseState{};
    return it->second.Latest();
}

const std::deque<Vec3>& UsbLFleetTracker::Path(int slaveId) const {
    static const std::deque<Vec3> kEmpty;
    auto it = trackers_.find(slaveId);
    if (it == trackers_.end()) return kEmpty;
    return it->second.Path();
}

void UsbLFleetTracker::Reset(int slaveId) {
    auto it = trackers_.find(slaveId);
    if (it != trackers_.end()) it->second.Reset();
}

void UsbLFleetTracker::ResetAll() {
    trackers_.clear();
}

} // namespace usbl
