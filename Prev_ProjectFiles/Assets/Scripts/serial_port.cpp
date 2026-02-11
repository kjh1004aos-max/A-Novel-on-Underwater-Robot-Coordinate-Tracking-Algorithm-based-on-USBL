
#include "serial_port.h"

#include <cerrno>
#include <cstring>

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

namespace usbl {

SerialPort::SerialPort()
#if defined(_WIN32)
    : handle_(nullptr)
#else
    : fd_(-1)
#endif
{
}

SerialPort::~SerialPort() {
    Close();
}

bool SerialPort::Open(const std::string& portName, int baudRate) {
    Close();
    lastError_.clear();

#if defined(_WIN32)
    // On Windows, COM ports above COM9 require the \\.\ prefix.
    std::string deviceName = portName;
    if (deviceName.rfind("\\\\.", 0) != 0) {
        if (deviceName.size() > 4 && (deviceName.substr(0, 3) == "COM" || deviceName.substr(0, 3) == "com")) {
            deviceName = "\\\\.\\" + deviceName;
        }
    }

    HANDLE h = CreateFileA(
        deviceName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (h == INVALID_HANDLE_VALUE) {
        lastError_ = "CreateFileA failed: " + std::to_string(GetLastError());
        handle_ = nullptr;
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) {
        lastError_ = "GetCommState failed: " + std::to_string(GetLastError());
        CloseHandle(h);
        return false;
    }

    dcb.BaudRate = static_cast<DWORD>(baudRate);
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(h, &dcb)) {
        lastError_ = "SetCommState failed: " + std::to_string(GetLastError());
        CloseHandle(h);
        return false;
    }

    // Non-blocking-ish: return immediately if no bytes are available.
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(h, &timeouts)) {
        lastError_ = "SetCommTimeouts failed: " + std::to_string(GetLastError());
        CloseHandle(h);
        return false;
    }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    handle_ = h;
    return true;

#else
    int fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        lastError_ = std::string("open() failed: ") + std::strerror(errno);
        return false;
    }

    termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        lastError_ = std::string("tcgetattr() failed: ") + std::strerror(errno);
        ::close(fd);
        return false;
    }

    // Raw mode
    cfmakeraw(&tty);

    // Baud rate mapping (common rates only)
    speed_t speed = B115200;
    switch (baudRate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 57600: speed = B57600; break;
        case 115200: speed = B115200; break;
        default:
            // Try to proceed with 115200 if unsupported
            speed = B115200;
            break;
    }

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CRTSCTS; // no hardware flow control
    tty.c_cflag &= ~PARENB;  // no parity
    tty.c_cflag &= ~CSTOPB;  // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;      // 8 data bits

    // Non-blocking read
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        lastError_ = std::string("tcsetattr() failed: ") + std::strerror(errno);
        ::close(fd);
        return false;
    }

    fd_ = fd;
    return true;
#endif
}

void SerialPort::Close() {
#if defined(_WIN32)
    if (handle_) {
        CloseHandle(reinterpret_cast<HANDLE>(handle_));
        handle_ = nullptr;
    }
#else
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

bool SerialPort::IsOpen() const {
#if defined(_WIN32)
    return handle_ != nullptr;
#else
    return fd_ >= 0;
#endif
}

int SerialPort::ReadAvailable(uint8_t* dst, int dstCapacity) {
    if (!IsOpen() || !dst || dstCapacity <= 0) return 0;

#if defined(_WIN32)
    HANDLE h = reinterpret_cast<HANDLE>(handle_);

    // Check how many bytes are in the queue.
    COMSTAT stat{};
    DWORD errors = 0;
    if (!ClearCommError(h, &errors, &stat)) {
        lastError_ = "ClearCommError failed: " + std::to_string(GetLastError());
        return 0;
    }

    const DWORD toRead = std::min<DWORD>(stat.cbInQue, static_cast<DWORD>(dstCapacity));
    if (toRead == 0) return 0;

    DWORD bytesRead = 0;
    if (!ReadFile(h, dst, toRead, &bytesRead, nullptr)) {
        lastError_ = "ReadFile failed: " + std::to_string(GetLastError());
        return 0;
    }
    return static_cast<int>(bytesRead);

#else
    int available = 0;
    if (ioctl(fd_, FIONREAD, &available) != 0) {
        lastError_ = std::string("ioctl(FIONREAD) failed: ") + std::strerror(errno);
        return 0;
    }
    if (available <= 0) return 0;

    const int toRead = std::min(available, dstCapacity);
    const int r = static_cast<int>(::read(fd_, dst, static_cast<size_t>(toRead)));
    if (r < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        lastError_ = std::string("read() failed: ") + std::strerror(errno);
        return 0;
    }
    return r;
#endif
}

bool SerialPort::Write(const uint8_t* data, int size) {
    if (!IsOpen() || !data || size <= 0) return false;

#if defined(_WIN32)
    HANDLE h = reinterpret_cast<HANDLE>(handle_);
    DWORD written = 0;
    if (!WriteFile(h, data, static_cast<DWORD>(size), &written, nullptr)) {
        lastError_ = "WriteFile failed: " + std::to_string(GetLastError());
        return false;
    }
    return written == static_cast<DWORD>(size);
#else
    const ssize_t w = ::write(fd_, data, static_cast<size_t>(size));
    if (w < 0) {
        lastError_ = std::string("write() failed: ") + std::strerror(errno);
        return false;
    }
    return w == size;
#endif
}

bool SerialPort::WriteString(const std::string& s) {
    return Write(reinterpret_cast<const uint8_t*>(s.data()), static_cast<int>(s.size()));
}

std::string SerialPort::LastError() const {
    return lastError_;
}

} // namespace usbl
