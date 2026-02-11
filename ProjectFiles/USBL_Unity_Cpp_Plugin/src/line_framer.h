
#pragma once
#include <string>
#include <vector>

namespace usbl {

// Splits a byte stream into text lines.
// Many acoustic beacons forward telemetry as ASCII lines over RS-232/serial.
// We keep this generic: push raw bytes, pop complete lines (without trailing CR/LF).
class LineFramer final {
public:
    LineFramer() = default;

    // Append bytes to internal buffer.
    void PushBytes(const uint8_t* data, int size);

    // Extract complete lines. Each returned line has no '\r' or '\n'.
    std::vector<std::string> PopLines();

    void Clear();

private:
    std::string buffer_;
};

} // namespace usbl
