// Cross-platform serial port wrapper (POSIX termios / Windows WinAPI).
// Designed to be sufficient for SeaTrac beacons over RS232.

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace serial {

    struct SerialConfig {
        uint32_t baud_rate = 115200;
        uint8_t data_bits = 8;
        bool parity_enable = false;
        bool parity_odd = false;
        uint8_t stop_bits = 2; // SeaTrac uses 2 stop bits per developer guide.
        bool rtscts = false;
    };

    class SerialPort {
    public:
        SerialPort() = default;
        SerialPort(const SerialPort&) = delete;
        SerialPort& operator=(const SerialPort&) = delete;

        ~SerialPort() { close(); }

        void open(const std::string& port_name, const SerialConfig& cfg);
        void close() noexcept;

        bool is_open() const noexcept;

        // Blocking read with a soft timeout.
        // Returns the number of bytes actually read (0 means timeout/no data).
        size_t read(uint8_t* dst, size_t dst_len, uint32_t timeout_ms);

        // Writes the full buffer or throws on failure.
        void write_all(const uint8_t* data, size_t len);

        void write_all(std::string_view s) { write_all(reinterpret_cast<const uint8_t*>(s.data()), s.size()); }

    private:
#ifdef _WIN32
        void* handle_ = nullptr; // HANDLE
#else
        int fd_ = -1;
#endif
    };

} // namespace serial
