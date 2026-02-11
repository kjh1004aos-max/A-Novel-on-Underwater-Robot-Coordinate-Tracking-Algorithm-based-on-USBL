// File: src/serial_port.cpp
#include "serial_port.hpp"

#include <chrono>
#include <cstring>
#include <thread>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace serial {

    static DCB make_dcb(const SerialConfig& cfg) {
        DCB dcb;
        std::memset(&dcb, 0, sizeof(dcb));
        dcb.DCBlength = sizeof(dcb);

        dcb.BaudRate = cfg.baud_rate;
        dcb.ByteSize = cfg.data_bits;

        if (cfg.parity_enable) {
            dcb.Parity = cfg.parity_odd ? ODDPARITY : EVENPARITY;
            dcb.fParity = TRUE;
        }
        else {
            dcb.Parity = NOPARITY;
            dcb.fParity = FALSE;
        }

        if (cfg.stop_bits == 2) {
            dcb.StopBits = TWOSTOPBITS;
        }
        else {
            dcb.StopBits = ONESTOPBIT;
        }

        dcb.fOutxCtsFlow = cfg.rtscts ? TRUE : FALSE;
        dcb.fRtsControl = cfg.rtscts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;

        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        dcb.fBinary = TRUE;

        return dcb;
    }

    void SerialPort::open(const std::string& port_name, const SerialConfig& cfg) {
        close();

        // Windows expects COM ports like "COM3" or "\\\\.\\COM10"
        std::string win_port = port_name;
        if (win_port.rfind("COM", 0) == 0 && win_port.size() > 4) {
            // COM10+ requires special prefix.
            win_port = std::string("\\\\.\\") + win_port;
        }

        HANDLE h = CreateFileA(
            win_port.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (h == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open serial port: " + port_name);
        }

        DCB dcb = make_dcb(cfg);
        if (!SetCommState(h, &dcb)) {
            CloseHandle(h);
            throw std::runtime_error("Failed to set serial port state");
        }

        COMMTIMEOUTS timeouts;
        std::memset(&timeouts, 0, sizeof(timeouts));
        // We'll implement timeouts ourselves (ReadFile with total timeout), but set sane defaults.
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 1000;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        if (!SetCommTimeouts(h, &timeouts)) {
            CloseHandle(h);
            throw std::runtime_error("Failed to set serial port timeouts");
        }

        handle_ = h;
    }

    void SerialPort::close() noexcept {
        if (handle_ != nullptr) {
            CloseHandle(reinterpret_cast<HANDLE>(handle_));
            handle_ = nullptr;
        }
    }

    bool SerialPort::is_open() const noexcept {
        return handle_ != nullptr;
    }

    size_t SerialPort::read(uint8_t* dst, size_t dst_len, uint32_t timeout_ms) {
        if (!is_open()) {
            throw std::runtime_error("SerialPort not open");
        }
        if (dst_len == 0) {
            return 0;
        }

        HANDLE h = reinterpret_cast<HANDLE>(handle_);

        const auto start = std::chrono::steady_clock::now();
        while (true) {
            DWORD bytes_read = 0;
            if (!ReadFile(h, dst, static_cast<DWORD>(dst_len), &bytes_read, nullptr)) {
                throw std::runtime_error("SerialPort read failed");
            }

            if (bytes_read > 0) {
                return static_cast<size_t>(bytes_read);
            }

            const auto now = std::chrono::steady_clock::now();
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed_ms >= static_cast<long long>(timeout_ms)) {
                return 0;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void SerialPort::write_all(const uint8_t* data, size_t len) {
        if (!is_open()) {
            throw std::runtime_error("SerialPort not open");
        }
        HANDLE h = reinterpret_cast<HANDLE>(handle_);
        size_t written_total = 0;
        while (written_total < len) {
            DWORD written = 0;
            if (!WriteFile(h, data + written_total, static_cast<DWORD>(len - written_total), &written, nullptr)) {
                throw std::runtime_error("SerialPort write failed");
            }
            if (written == 0) {
                throw std::runtime_error("SerialPort write returned 0 bytes");
            }
            written_total += static_cast<size_t>(written);
        }
    }

} // namespace serial

#else

#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace serial {

    static speed_t to_speed(uint32_t baud) {
        switch (baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
#ifdef B230400
        case 230400: return B230400;
#endif
        default:
            throw std::runtime_error("Unsupported baud rate: " + std::to_string(baud));
        }
    }

    void SerialPort::open(const std::string& port_name, const SerialConfig& cfg) {
        close();

        const int fd = ::open(port_name.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd < 0) {
            throw std::runtime_error("Failed to open serial port: " + port_name + " (" + std::strerror(errno) + ")");
        }

        termios tty;
        std::memset(&tty, 0, sizeof(tty));

        if (tcgetattr(fd, &tty) != 0) {
            ::close(fd);
            throw std::runtime_error("tcgetattr failed: " + std::string(std::strerror(errno)));
        }

        cfmakeraw(&tty);

        // Baud rate
        const speed_t sp = to_speed(cfg.baud_rate);
        cfsetispeed(&tty, sp);
        cfsetospeed(&tty, sp);

        // Data bits
        tty.c_cflag &= ~CSIZE;
        switch (cfg.data_bits) {
        case 8: tty.c_cflag |= CS8; break;
        case 7: tty.c_cflag |= CS7; break;
        default:
            ::close(fd);
            throw std::runtime_error("Unsupported data bits");
        }

        // Parity
        if (cfg.parity_enable) {
            tty.c_cflag |= PARENB;
            if (cfg.parity_odd) {
                tty.c_cflag |= PARODD;
            }
            else {
                tty.c_cflag &= ~PARODD;
            }
        }
        else {
            tty.c_cflag &= ~PARENB;
        }

        // Stop bits
        if (cfg.stop_bits == 2) {
            tty.c_cflag |= CSTOPB;
        }
        else {
            tty.c_cflag &= ~CSTOPB;
        }

        // Flow control
#ifdef CRTSCTS
        if (cfg.rtscts) {
            tty.c_cflag |= CRTSCTS;
        }
        else {
            tty.c_cflag &= ~CRTSCTS;
        }
#endif

        tty.c_cflag |= CLOCAL | CREAD;

        // Non-blocking-ish reads handled via select in read().
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            ::close(fd);
            throw std::runtime_error("tcsetattr failed: " + std::string(std::strerror(errno)));
        }

        fd_ = fd;
    }

    void SerialPort::close() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool SerialPort::is_open() const noexcept {
        return fd_ >= 0;
    }

    size_t SerialPort::read(uint8_t* dst, size_t dst_len, uint32_t timeout_ms) {
        if (!is_open()) {
            throw std::runtime_error("SerialPort not open");
        }
        if (dst_len == 0) {
            return 0;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_, &readfds);

        timeval tv;
        tv.tv_sec = static_cast<long>(timeout_ms / 1000);
        tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);

        const int rv = select(fd_ + 1, &readfds, nullptr, nullptr, &tv);
        if (rv < 0) {
            throw std::runtime_error("select failed: " + std::string(std::strerror(errno)));
        }
        if (rv == 0) {
            return 0; // timeout
        }

        const ssize_t n = ::read(fd_, dst, dst_len);
        if (n < 0) {
            throw std::runtime_error("read failed: " + std::string(std::strerror(errno)));
        }
        return static_cast<size_t>(n);
    }

    void SerialPort::write_all(const uint8_t* data, size_t len) {
        if (!is_open()) {
            throw std::runtime_error("SerialPort not open");
        }

        size_t written_total = 0;
        while (written_total < len) {
            const ssize_t n = ::write(fd_, data + written_total, len - written_total);
            if (n < 0) {
                throw std::runtime_error("write failed: " + std::string(std::strerror(errno)));
            }
            if (n == 0) {
                throw std::runtime_error("write returned 0 bytes");
            }
            written_total += static_cast<size_t>(n);
        }
    }

} // namespace serial

#endif
