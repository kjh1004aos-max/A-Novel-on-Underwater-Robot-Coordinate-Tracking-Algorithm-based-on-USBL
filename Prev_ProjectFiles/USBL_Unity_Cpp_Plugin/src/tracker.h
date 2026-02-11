
#pragma once
#include "math_utils.h"
#include "parser.h"

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace usbl {

// How to interpret the "distance" value coming from the beacon.
// Some devices output slant range (3D distance); others output horizontal range (planar).
enum class DistanceMode : int {
    SlantRange = 0,
    HorizontalRange = 1
};

struct TrackerConfig {
    // Coordinate conventions (master-frame)
    // - azimuthZeroIsNorth: if true, azimuth=0 points to +Y (North) and increases toward +X (East).
    //   This matches common "bearing" conventions.
    // - if false, azimuth=0 points to +X and increases toward +Y.
    bool azimuthZeroIsNorth = true;

    // Elevation sign convention:
    // - If elevationPositiveDown is true: elevation>0 means the slave is below the horizontal plane.
    // - If false: elevation>0 means the slave is above the horizontal plane.
    bool elevationPositiveDown = true;

    // Distance mode (see enum above).
    DistanceMode distanceMode = DistanceMode::SlantRange;

    // USBL phase sign:
    // If the device defines phase differences with opposite sign, flip this.
    // Typical plane-wave relation: phaseDiff = -2π f (d·u)/c (u points to source).
    int phaseSign = -1;

    // Threshold from the paper: if error >= 5%, choose the position closer to the previous point.
    // We interpret "error" as: ||Pm - Pv|| / max(||Pm||, eps).
    double errorThresholdRatio = 0.05;

    // Path buffer
    size_t maxPathPoints = 50'000; // adjust as needed
};

struct PoseState {
    Vec3 positionENU; // x=East, y=North, z=Up (meters)
    double yawDeg = 0.0;
    double rollDeg = 0.0;
    double pitchDeg = 0.0;

    bool hasPosition = false;
    bool hasAttitude = false;
};

class UsbLTracker final {
public:
    explicit UsbLTracker(TrackerConfig cfg);

    const TrackerConfig& Config() const { return cfg_; }
    void SetConfig(const TrackerConfig& cfg) { cfg_ = cfg; }

    // Update tracker with new packet parts.
    // Returns true if a new fused position was produced.
    bool Update(const Measurement& meas, const UsbLSoundInfo& usbl);

    bool HasLatest() const { return latest_.has_value(); }
    PoseState Latest() const { return latest_.value_or(PoseState{}); }

    // Access path (fused positions)
    const std::deque<Vec3>& Path() const { return path_; }

    // Clears path/history (does not reset config)
    void Reset();

private:
    TrackerConfig cfg_;
    std::optional<PoseState> latest_;
    std::deque<Vec3> path_;

    // Compute measured position from spherical fields
    bool ComputeMeasuredPosition(const Measurement& meas, Vec3& outENU) const;

    // Compute a virtual position from USBL phase-based DOA + range
    // Returns false if insufficient inputs exist.
    bool ComputeVirtualPosition(const UsbLSoundInfo& usbl, double fallbackRangeMeters, Vec3& outENU) const;

    static double ErrorRatio(const Vec3& measured, const Vec3& virt);
};

// Tracks multiple slaves (X110) under a single master beacon (X150).
class UsbLFleetTracker final {
public:
    explicit UsbLFleetTracker(TrackerConfig cfg);

    void SetConfig(const TrackerConfig& cfg);

    // Update a slave by ID. Creates the tracker on demand.
    bool Update(int slaveId, const Measurement& meas, const UsbLSoundInfo& usbl);

    bool HasLatest(int slaveId) const;
    PoseState Latest(int slaveId) const;

    const std::deque<Vec3>& Path(int slaveId) const;

    void Reset(int slaveId);
    void ResetAll();

private:
    TrackerConfig cfg_;
    std::unordered_map<int, UsbLTracker> trackers_;

    UsbLTracker& GetOrCreate(int slaveId);
};

} // namespace usbl
