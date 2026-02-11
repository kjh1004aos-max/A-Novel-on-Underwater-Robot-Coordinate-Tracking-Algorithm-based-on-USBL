
#include "parser.h"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <string_view>

namespace usbl {

static inline void TrimInPlace(std::string& s) {
    auto isSpace = [](unsigned char c){ return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
}

std::string PacketParser::ToUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool PacketParser::TryParseDouble(const std::string& s, double& out) {
    // Use strtod for robustness (handles scientific notation, leading/trailing spaces).
    char* end = nullptr;
    errno = 0;
    const double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    if (errno != 0) return false;
    out = v;
    return true;
}

bool PacketParser::TryParseInt(const std::string& s, int& out) {
    // Use from_chars when available for speed, fallback to strtol-style check.
    int v = 0;
    const char* begin = s.data();
    const char* end = s.data() + s.size();
    auto res = std::from_chars(begin, end, v);
    if (res.ec != std::errc() || res.ptr != end) return false;
    out = v;
    return true;
}

std::unordered_map<std::string, std::string> PacketParser::ParseKeyValueMap(const std::string& line) {
    std::unordered_map<std::string, std::string> kv;

    std::string token;
    std::stringstream ss(line);

    while (std::getline(ss, token, ',')) {
        TrimInPlace(token);
        if (token.empty()) continue;

        // Allow both KEY=VALUE and KEY:VALUE
        size_t eq = token.find('=');
        if (eq == std::string::npos) eq = token.find(':');
        if (eq == std::string::npos) continue;

        std::string key = token.substr(0, eq);
        std::string val = token.substr(eq + 1);
        TrimInPlace(key);
        TrimInPlace(val);
        if (key.empty()) continue;

        kv[ToUpper(key)] = val;
    }

    return kv;
}

std::optional<ParsedPacket> PacketParser::ParseLine(const std::string& line) const {
    ParsedPacket p;
    const auto kv = ParseKeyValueMap(line);
    if (kv.empty()) return std::nullopt;

    bool any = false;

    // Slave ID
    auto it = kv.find("ID");
    if (it != kv.end()) {
        int id = 0;
        if (TryParseInt(it->second, id)) {
            p.slaveId = id;
            p.hasSlaveId = true;
            any = true;
        }
    }

    // Measurement keys
    auto getD = [&](const char* key, double& dst, bool& flag) {
        auto it2 = kv.find(key);
        if (it2 == kv.end()) return;
        double v = 0.0;
        if (TryParseDouble(it2->second, v)) {
            dst = v;
            flag = true;
        }
    };

    bool hasAz = false, hasEl = false, hasDst = false;
    getD("AZ", p.meas.azimuthDeg, hasAz);
    getD("AZIMUTH", p.meas.azimuthDeg, hasAz);

    getD("EL", p.meas.elevationDeg, hasEl);
    getD("ELEV", p.meas.elevationDeg, hasEl);
    getD("ELEVATION", p.meas.elevationDeg, hasEl);

    getD("DST", p.meas.distanceMeters, hasDst);
    getD("DIST", p.meas.distanceMeters, hasDst);
    getD("DISTANCE", p.meas.distanceMeters, hasDst);
    getD("RANGE", p.meas.distanceMeters, hasDst);

    if (hasAz && hasEl && hasDst) {
        p.meas.hasSpherical = true;
        any = true;
    }

    bool hasYaw = false, hasRoll = false, hasPitch = false;
    getD("YAW", p.meas.yawDeg, hasYaw);
    getD("ROLL", p.meas.rollDeg, hasRoll);
    getD("PITCH", p.meas.pitchDeg, hasPitch);

    if (hasYaw || hasRoll || hasPitch) {
        p.meas.hasAttitude = true;
        any = true;
    }

    // USBL sound keys
    getD("PHX", p.usbl.phaseDiffXRad, p.usbl.hasPhaseDiffX);
    getD("PHASEX", p.usbl.phaseDiffXRad, p.usbl.hasPhaseDiffX);
    getD("PHI_X", p.usbl.phaseDiffXRad, p.usbl.hasPhaseDiffX);

    getD("PHY", p.usbl.phaseDiffYRad, p.usbl.hasPhaseDiffY);
    getD("PHASEY", p.usbl.phaseDiffYRad, p.usbl.hasPhaseDiffY);
    getD("PHI_Y", p.usbl.phaseDiffYRad, p.usbl.hasPhaseDiffY);

    getD("FREQ", p.usbl.carrierHz, p.usbl.hasCarrierHz);
    getD("FC", p.usbl.carrierHz, p.usbl.hasCarrierHz);
    getD("CARRIER", p.usbl.carrierHz, p.usbl.hasCarrierHz);

    getD("C", p.usbl.soundSpeed, p.usbl.hasSoundSpeed);
    getD("SOUNDSPEED", p.usbl.soundSpeed, p.usbl.hasSoundSpeed);
    getD("V", p.usbl.soundSpeed, p.usbl.hasSoundSpeed);

    getD("DX", p.usbl.sensorSpacingX, p.usbl.hasSensorSpacingX);
    getD("DY", p.usbl.sensorSpacingY, p.usbl.hasSensorSpacingY);
    getD("BASEX", p.usbl.sensorSpacingX, p.usbl.hasSensorSpacingX);
    getD("BASEY", p.usbl.sensorSpacingY, p.usbl.hasSensorSpacingY);

    getD("TOF", p.usbl.timeOfFlightSec, p.usbl.hasTof);
    getD("T", p.usbl.timeOfFlightSec, p.usbl.hasTof);

    if (p.usbl.hasPhaseDiffX || p.usbl.hasPhaseDiffY || p.usbl.hasCarrierHz || p.usbl.hasTof) {
        any = true;
    }

    p.valid = any;
    if (!p.valid) return std::nullopt;
    return p;
}

} // namespace usbl
