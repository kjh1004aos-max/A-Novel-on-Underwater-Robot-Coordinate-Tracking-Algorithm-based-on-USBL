
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace usbl {

// Cross-platform serial port wrapper for Unity native plugins.
// - Windows: uses Win32 COM APIs.
// - POSIX (Linux/macOS): uses termios.
//
// This implementation intentionally keeps dependencies minimal.
// It supports:
// - Open/Close
// - Non-blocking read of currently available bytes
// - Write bytes (ASCII commands)
class SerialPort final {
public:
    SerialPort();
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool Open(const std::string& portName, int baudRate);
    void Close();
    bool IsOpen() const;

    // Read any bytes currently available (non-blocking).
    // Returns number of bytes read (0 means no data available).
    int ReadAvailable(uint8_t* dst, int dstCapacity);

    // Write bytes. Returns true on success.
    bool Write(const uint8_t* data, int size);

    // Convenience for ASCII lines.
    bool WriteString(const std::string& s);

    std::string LastError() const;

private:
    std::string lastError_;

#if defined(_WIN32)
    void* handle_; // HANDLE, but avoid including windows.h in header
#else
    int fd_;
#endif
};

} // namespace usbl
