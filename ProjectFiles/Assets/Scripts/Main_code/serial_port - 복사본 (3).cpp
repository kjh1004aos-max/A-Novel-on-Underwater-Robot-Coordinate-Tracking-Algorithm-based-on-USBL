#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "serial_port.hpp"
#include "seatrac.hpp"
#include "ts_queue.hpp"
#include "usbl_control.hpp"

namespace {
    std::atomic<bool> g_stop{ false };

    void handle_signal(int) {
        g_stop.store(true);
    }

    uint8_t parse_u8(const std::string& s) {
        const int v = std::stoi(s);
        if (v < 0 || v > 255) {
            throw std::runtime_error("Invalid u8: " + s);
        }
        return static_cast<uint8_t>(v);
    }

    struct Args {
        std::string port = "/dev/ttyUSB0";
        uint32_t serial_read_timeout_ms = 50;

        // Optional: ID sanity check
        uint8_t expected_local_id = 0; // 0 means don't check
    };

    Args parse_args(int argc, char** argv) {
        Args a;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            auto next = [&]() -> std::string {
                if (i + 1 >= argc) throw std::runtime_error("Missing value for " + key);
                return argv[++i];
            };

            if (key == "--port") {
                a.port = next();
            }
            else if (key == "--expected_local_id") {
                a.expected_local_id = parse_u8(next());
            }
            else {
                throw std::runtime_error("Unknown arg: " + key);
            }
        }
        return a;
    }

    // This is a placeholder actuator interface.
    // Integrate with your motor controller / fin controller here.
    class ActuatorInterface {
    public:
        void set_steering_deg(double steering_deg) {
            // TODO: map to servo PWM / fin frequency etc.
            last_steering_deg_ = steering_deg;
        }

        void set_speed_cms(double speed_cms) {
            // TODO: map to thruster command or tail-fin frequency etc.
            last_speed_cms_ = speed_cms;
        }

        void stop() {
            set_speed_cms(0.0);
            set_steering_deg(0.0);
        }

        void print_state() const {
            std::cout << "[actuator] steering=" << last_steering_deg_
                << " deg, speed=" << last_speed_cms_ << " cm/s\n";
        }

    private:
        double last_steering_deg_ = 0.0;
        double last_speed_cms_ = 0.0;
    };

} // namespace

int main(int argc, char** argv) {
    try {
#ifdef SIGINT
        std::signal(SIGINT, handle_signal);
#endif
#ifdef SIGTERM
        std::signal(SIGTERM, handle_signal);
#endif

        const Args args = parse_args(argc, argv);

        serial::SerialPort port;
        serial::SerialConfig cfg;
        cfg.baud_rate = 115200;
        cfg.data_bits = 8;
        cfg.parity_enable = false;
        cfg.stop_bits = 2;
        cfg.rtscts = false;

        port.open(args.port, cfg);

        util::TSQueue<seatrac::Frame> rx_frames;
        seatrac::StreamParser parser([&](const seatrac::Frame& f) { rx_frames.push(f); });

        std::atomic<bool> reader_ok{ true };
        std::thread reader([&] {
            try {
                std::vector<uint8_t> buf(512);
                while (!g_stop.load()) {
                    const size_t n = port.read(buf.data(), buf.size(), args.serial_read_timeout_ms);
                    if (n > 0) {
                        parser.feed(buf.data(), n);
                    }
                }
            }
            catch (const std::exception& e) {
                reader_ok.store(false);
                std::cerr << "[reader] fatal: " << e.what() << "\n";
            }
            });

        ActuatorInterface actuator;

        while (!g_stop.load()) {
            if (!reader_ok.load()) {
                throw std::runtime_error("Reader thread failed");
            }

            const auto f = rx_frames.pop_wait(200);
            if (!f) {
                continue;
            }

            if (f->sync != '$') {
                continue;
            }

            try {
                if (f->cid == seatrac::cid::DAT_RECEIVE) {
                    const auto msg = seatrac::decode_dat_receive(*f);
                    if (!msg.local_flag) {
                        continue; // sniffed traffic not for us
                    }

                    if (args.expected_local_id != 0 && msg.fix.dest_id != args.expected_local_id && msg.fix.dest_id != 0) {
                        // If destination is not local id and not broadcast, ignore.
                        continue;
                    }

                    if (msg.data.size() >= usbl::ControlPacket::kSize) {
                        const auto cmd = usbl::ControlPacket::decode(msg.data);
                        if (cmd.version != 1 || cmd.msg_type != 1) {
                            std::cerr << "[warn] Unsupported ControlPacket version/type\n";
                            continue;
                        }

                        const double steering_deg = static_cast<double>(cmd.steering_deci_deg) / 10.0;
                        const double speed_cms = static_cast<double>(cmd.speed_cms);

                        actuator.set_steering_deg(steering_deg);
                        actuator.set_speed_cms(speed_cms);

                        std::cout << "[cmd] seq=" << int(cmd.seq) << " steering=" << steering_deg
                            << " deg speed=" << speed_cms << " cm/s\n";
                        actuator.print_state();
                    }
                    else {
                        std::cerr << "[warn] DAT payload too short: " << msg.data.size() << "\n";
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[warn] decode failed: " << e.what() << "\n";
            }
        }

        actuator.stop();

        g_stop.store(true);
        if (reader.joinable()) reader.join();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
