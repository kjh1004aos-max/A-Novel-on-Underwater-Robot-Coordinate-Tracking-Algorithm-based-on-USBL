
#include "usbl_plugin.h"

#include "controller.h"
#include "line_framer.h"
#include "parser.h"
#include "serial_port.h"
#include "tracker.h"

#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace usbl {

class Context final {
public:
    Context()
        : fleet_(TrackerConfig{}),
          controller_(ControllerConfig{}) {}

    // Non-copyable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    bool OpenSerial(const std::string& port, int baud) {
        std::lock_guard<std::mutex> lock(mu_);
        lastError_.clear();
        framer_.Clear();
        return serial_.Open(port, baud);
    }

    void CloseSerial() {
        std::lock_guard<std::mutex> lock(mu_);
        serial_.Close();
        framer_.Clear();
    }

    bool IsSerialOpen() const {
        // serial_ is thread-safe enough for this read
        return serial_.IsOpen();
    }

    int UpdateFromSerial() {
        std::lock_guard<std::mutex> lock(mu_);
        lastError_.clear();

        if (!serial_.IsOpen()) return 0;

        uint8_t buf[4096];
        int totalPackets = 0;

        for (;;) {
            const int n = serial_.ReadAvailable(buf, static_cast<int>(sizeof(buf)));
            if (n <= 0) break;
            framer_.PushBytes(buf, n);

            for (const std::string& line : framer_.PopLines()) {
                totalPackets += HandleLine_NoLock(line) ? 1 : 0;
            }
        }

        if (!serial_.LastError().empty()) {
            lastError_ = serial_.LastError();
        }
        return totalPackets;
    }

    int FeedLine(const std::string& line) {
        std::lock_guard<std::mutex> lock(mu_);
        return HandleLine_NoLock(line) ? 1 : 0;
    }

    void SetTrackerConfig(const UsbLTrackerConfigC& c) {
        std::lock_guard<std::mutex> lock(mu_);
        TrackerConfig cfg{};
        cfg.azimuthZeroIsNorth = (c.azimuthZeroIsNorth != 0);
        cfg.elevationPositiveDown = (c.elevationPositiveDown != 0);
        cfg.distanceMode = (c.distanceMode == 0) ? DistanceMode::SlantRange : DistanceMode::HorizontalRange;
        cfg.phaseSign = (c.phaseSign >= 0) ? 1 : -1;
        cfg.errorThresholdRatio = (c.errorThresholdRatio > 0.0) ? c.errorThresholdRatio : 0.05;
        cfg.maxPathPoints = (c.maxPathPoints > 0) ? static_cast<size_t>(c.maxPathPoints) : cfg.maxPathPoints;
        fleet_.SetConfig(cfg);
    }

    void SetControllerConfig(const UsbLControllerConfigC& c) {
        std::lock_guard<std::mutex> lock(mu_);
        ControllerConfig cfg{};
        cfg.kpYaw = c.kpYaw;
        cfg.kdYaw = c.kdYaw;
        cfg.maxSteerDeg = c.maxSteerDeg;
        cfg.throttle = c.throttle;
        cfg.yawZeroIsNorth = (c.yawZeroIsNorth != 0);
        controller_.SetConfig(cfg);
        controller_.Reset();
    }

    bool GetLatestPose(int slaveId, UsbLPoseC& out) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!fleet_.HasLatest(slaveId)) return false;
        const PoseState st = fleet_.Latest(slaveId);
        out.positionENU = UsbLVec3C{st.positionENU.x, st.positionENU.y, st.positionENU.z};
        out.yawDeg = st.yawDeg;
        out.rollDeg = st.rollDeg;
        out.pitchDeg = st.pitchDeg;
        out.hasPosition = st.hasPosition ? 1 : 0;
        out.hasAttitude = st.hasAttitude ? 1 : 0;
        return true;
    }

    int GetPathPoints(int slaveId, UsbLVec3C* outPoints, int maxPoints) {
        std::lock_guard<std::mutex> lock(mu_);
        const auto& path = fleet_.Path(slaveId);
        if (!outPoints || maxPoints <= 0) return 0;

        const int n = std::min<int>(static_cast<int>(path.size()), maxPoints);
        for (int i = 0; i < n; ++i) {
            outPoints[i] = UsbLVec3C{path[i].x, path[i].y, path[i].z};
        }
        return n;
    }

    void SetDestination(int slaveId, const Vec3& destENU) {
        std::lock_guard<std::mutex> lock(mu_);
        destENU_[slaveId] = destENU;
    }

    bool ComputeControl(int slaveId, double dtSeconds, UsbLControlC& out) {
        std::lock_guard<std::mutex> lock(mu_);
        out.valid = 0;

        if (!fleet_.HasLatest(slaveId)) return false;
        const PoseState st = fleet_.Latest(slaveId);

        auto it = destENU_.find(slaveId);
        if (it == destENU_.end()) return false;

        ControlOutput ctrl{};
        if (!controller_.Compute(st, it->second, dtSeconds, ctrl)) return false;

        out.desiredYawDeg = ctrl.desiredYawDeg;
        out.headingErrorDeg = ctrl.headingErrorDeg;
        out.steeringDeg = ctrl.steeringDeg;
        out.throttle = ctrl.throttle;
        out.valid = 1;
        return true;
    }

    bool SendDefaultCommand(int slaveId, const UsbLControlC& control) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!serial_.IsOpen()) return false;
        if (!control.valid) return false;

        // NOTE:
        // The manuscript describes that the master beacon (X150) sends "command signals"
        // to the slave beacon (X110) to correct steering toward the destination.
        // The exact binary/ASCII protocol is hardware-specific and not defined in the paper.
        //
        // Therefore, we provide a safe *template* command string that you must adapt
        // to your beacon's actual command format.
        //
        // Example format (ASCII):
        //   $CMD,ID=<id>,STEER_DEG=<deg>,THR=<0..1>\r\n
        //
        // This is intentionally human-readable so you can capture it with a serial sniffer
        // and adjust it to match your device documentation.
        char msg[256];
        std::snprintf(msg, sizeof(msg),
                      "$CMD,ID=%d,STEER_DEG=%.3f,THR=%.3f\r\n",
                      slaveId,
                      control.steeringDeg,
                      control.throttle);

        const bool ok = serial_.WriteString(std::string(msg));
        if (!ok) lastError_ = serial_.LastError();
        return ok;
    }

    bool SendRawAscii(const std::string& ascii) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!serial_.IsOpen()) return false;
        const bool ok = serial_.WriteString(ascii);
        if (!ok) lastError_ = serial_.LastError();
        return ok;
    }

    std::string LastError() const {
        std::lock_guard<std::mutex> lock(mu_);
        if (!lastError_.empty()) return lastError_;
        if (!serial_.LastError().empty()) return serial_.LastError();
        return {};
    }

private:
    bool HandleLine_NoLock(const std::string& line) {
        const auto pktOpt = parser_.ParseLine(line);
        if (!pktOpt.has_value()) return false;

        ParsedPacket pkt = pktOpt.value();
        if (!pkt.hasSlaveId) {
            // If the hardware doesn't embed ID in each line, you can:
            // - fix it to 1, or
            // - parse a different field
            pkt.slaveId = 1;
        }

        fleet_.Update(pkt.slaveId, pkt.meas, pkt.usbl);
        return true;
    }

    mutable std::mutex mu_;
    mutable std::string lastError_;

    SerialPort serial_;
    LineFramer framer_;
    PacketParser parser_;

    UsbLFleetTracker fleet_;
    HeadingController controller_;

    std::unordered_map<int, Vec3> destENU_;
};

} // namespace usbl

// ---- C ABI wrappers ----
extern "C" {

USBL_API void* usbl_create() {
    return new usbl::Context();
}

USBL_API void usbl_destroy(void* ctx) {
    delete reinterpret_cast<usbl::Context*>(ctx);
}

USBL_API int usbl_open_serial(void* ctx, const char* portName, int baudRate) {
    if (!ctx || !portName) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->OpenSerial(portName, baudRate) ? 1 : 0;
}

USBL_API void usbl_close_serial(void* ctx) {
    if (!ctx) return;
    reinterpret_cast<usbl::Context*>(ctx)->CloseSerial();
}

USBL_API int usbl_is_serial_open(void* ctx) {
    if (!ctx) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->IsSerialOpen() ? 1 : 0;
}

USBL_API int usbl_update(void* ctx) {
    if (!ctx) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->UpdateFromSerial();
}

USBL_API int usbl_feed_line(void* ctx, const char* line) {
    if (!ctx || !line) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->FeedLine(line);
}

USBL_API void usbl_set_tracker_config(void* ctx, const UsbLTrackerConfigC* cfg) {
    if (!ctx || !cfg) return;
    reinterpret_cast<usbl::Context*>(ctx)->SetTrackerConfig(*cfg);
}

USBL_API void usbl_set_controller_config(void* ctx, const UsbLControllerConfigC* cfg) {
    if (!ctx || !cfg) return;
    reinterpret_cast<usbl::Context*>(ctx)->SetControllerConfig(*cfg);
}

USBL_API int usbl_get_latest_pose(void* ctx, int slaveId, UsbLPoseC* outPose) {
    if (!ctx || !outPose) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->GetLatestPose(slaveId, *outPose) ? 1 : 0;
}

USBL_API int usbl_get_path_points(void* ctx, int slaveId, UsbLVec3C* outPoints, int maxPoints) {
    if (!ctx || !outPoints) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->GetPathPoints(slaveId, outPoints, maxPoints);
}

USBL_API void usbl_set_destination(void* ctx, int slaveId, double xENU, double yENU, double zENU) {
    if (!ctx) return;
    reinterpret_cast<usbl::Context*>(ctx)->SetDestination(slaveId, usbl::Vec3{xENU, yENU, zENU});
}

USBL_API int usbl_compute_control(void* ctx, int slaveId, double dtSeconds, UsbLControlC* outControl) {
    if (!ctx || !outControl) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->ComputeControl(slaveId, dtSeconds, *outControl) ? 1 : 0;
}

USBL_API int usbl_send_default_command(void* ctx, int slaveId, const UsbLControlC* control) {
    if (!ctx || !control) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->SendDefaultCommand(slaveId, *control) ? 1 : 0;
}

USBL_API int usbl_send_raw_ascii(void* ctx, const char* ascii) {
    if (!ctx || !ascii) return 0;
    return reinterpret_cast<usbl::Context*>(ctx)->SendRawAscii(ascii) ? 1 : 0;
}

USBL_API int usbl_get_last_error(void* ctx, char* outUtf8, int outCapacity) {
    if (!ctx || !outUtf8 || outCapacity <= 0) return 0;
    const std::string err = reinterpret_cast<usbl::Context*>(ctx)->LastError();
    if (err.empty()) {
        outUtf8[0] = '\0';
        return 0;
    }
    const int n = static_cast<int>(std::min<size_t>(err.size(), static_cast<size_t>(outCapacity - 1)));
    std::memcpy(outUtf8, err.data(), static_cast<size_t>(n));
    outUtf8[n] = '\0';
    return n;
}

} // extern "C"
