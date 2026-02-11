
#pragma once
#include <optional>
#include <string>
#include <unordered_map>

namespace usbl {

struct Measurement {
    // Spherical coordinates (relative to the master beacon / transceiver)
    // azimuthDeg: bearing on the horizontal plane (typically 0=N, 90=E)
    double azimuthDeg = 0.0;
    // elevationDeg: angle from horizontal plane (+down or +up depends on config)
    double elevationDeg = 0.0;
    // distanceMeters: either slant range or horizontal range depending on config
    double distanceMeters = 0.0;

    // AHRS attitude from the vehicle (slave beacon)
    double yawDeg = 0.0;
    double rollDeg = 0.0;
    double pitchDeg = 0.0;

    bool hasSpherical = false;
    bool hasAttitude  = false;
};

struct UsbLSoundInfo {
    // Phase differences (radians) between reference sensor and adjacent sensors.
    // For a minimal planar (x,y) array you typically have two phase differences.
    double phaseDiffXRad = 0.0;
    double phaseDiffYRad = 0.0;
    bool hasPhaseDiffX = false;
    bool hasPhaseDiffY = false;

    double carrierHz = 0.0;
    double soundSpeed = 1500.0; // m/s typical seawater
    bool hasCarrierHz = false;
    bool hasSoundSpeed = false;

    // Sensor spacing in meters (baseline distance between elements).
    double sensorSpacingX = 0.0;
    double sensorSpacingY = 0.0;
    bool hasSensorSpacingX = false;
    bool hasSensorSpacingY = false;

    // Optional time-of-flight (seconds) for range estimation: range = c * tof
    double timeOfFlightSec = 0.0;
    bool hasTof = false;
};

struct ParsedPacket {
    int slaveId = 0; // 1..14 typically
    bool hasSlaveId = false;

    Measurement meas;
    UsbLSoundInfo usbl;

    bool valid = false; // true if at least one useful field was parsed
};

// Parses a single ASCII line into a packet using a tolerant key=value format.
// Example line:
//   ID=1,AZ=12.3,EL=5.0,DST=14.2,YAW=180,ROLL=0.1,PITCH=-0.2,PHX=0.12,PHY=-0.05,FREQ=25000,C=1500,DX=0.05,DY=0.05,TOF=0.0093
//
// Notes:
// - Keys are case-insensitive.
// - Separators: ',' between fields, '=' between key and value.
// - Unknown keys are ignored.
class PacketParser final {
public:
    PacketParser() = default;

    std::optional<ParsedPacket> ParseLine(const std::string& line) const;

private:
    static std::unordered_map<std::string, std::string> ParseKeyValueMap(const std::string& line);
    static bool TryParseDouble(const std::string& s, double& out);
    static bool TryParseInt(const std::string& s, int& out);
    static std::string ToUpper(std::string s);
};

} // namespace usbl
