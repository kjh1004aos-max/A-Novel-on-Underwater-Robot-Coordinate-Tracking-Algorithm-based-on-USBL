#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <limits>
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

    double parse_double(const std::string& s) {
        const double v = std::stod(s);
        if (!std::isfinite(v)) {
            throw std::runtime_error("Invalid double: " + s);
        }
        return v;
    }

    struct Args {
        std::string port = "/dev/ttyUSB0";
        uint8_t remote_beacon_id = 1;

        // Destination in NED relative coordinates (meters).
        double dest_n = 0.0;
        double dest_e = 0.0;
        double dest_d = 0.0;

        // Polling and timeouts.
        uint32_t poll_period_ms = 2000;
        uint32_t serial_read_timeout_ms = 50;
        uint32_t nav_response_timeout_ms = 6000;

        // Control
        double steering_kp = 1.0;
        double steering_max_deg = 30.0;
        double speed_cms = 50.0; // 0.5 m/s
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
            else if (key == "--remote_id") {
                a.remote_beacon_id = parse_u8(next());
            }
            else if (key == "--dest_n") {
                a.dest_n = parse_double(next());
            }
            else if (key == "--dest_e") {
                a.dest_e = parse_double(next());
            }
            else if (key == "--dest_d") {
                a.dest_d = parse_double(next());
            }
            else if (key == "--poll_ms") {
                a.poll_period_ms = static_cast<uint32_t>(std::stoul(next()));
            }
            else if (key == "--nav_timeout_ms") {
                a.nav_response_timeout_ms = static_cast<uint32_t>(std::stoul(next()));
            }
            else if (key == "--kp") {
                a.steering_kp = parse_double(next());
            }
            else if (key == "--max_steer_deg") {
                a.steering_max_deg = parse_double(next());
            }
            else if (key == "--speed_cms") {
                a.speed_cms = parse_double(next());
            }
            else {
                throw std::runtime_error("Unknown argument: " + key);
            }
        }
        return a;
    }

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

        seatrac::StreamParser parser([&](const seatrac::Frame& f) {
            rx_frames.push(f);
            });

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

        // Basic sanity ping to the beacon's command processor.
        port.write_all(seatrac::cmd_sys_alive());

        // Tracking and control objects.
        usbl::PathTracker::Params tp;
        tp.rel_error_threshold = 0.05;
        tp.max_usbl_fit_error = 3.0;
        tp.max_path_len = 200000;
        usbl::PathTracker tracker(tp);

        usbl::HeadingController::Params hp;
        hp.kp = args.steering_kp;
        hp.max_cmd = args.steering_max_deg;
        hp.deadband_deg = 2.0;
        usbl::HeadingController heading(hp);

        const usbl::Vec3 dest{ args.dest_n, args.dest_e, args.dest_d };

        uint8_t seq = 0;
        auto next_tick = std::chrono::steady_clock::now();

        while (!g_stop.load()) {
            if (!reader_ok.load()) {
                throw std::runtime_error("Reader thread failed");
            }

            next_tick += std::chrono::milliseconds(args.poll_period_ms);

            // Send NAV query to obtain:
            // - Enhanced USBL fix (ACOFIX)
            // - Remote depth (optional, used by system)
            // - Remote attitude (yaw/pitch/roll) for control
            const uint8_t query_flags = static_cast<uint8_t>(
                seatrac::nav_query::QRY_DEPTH | seatrac::nav_query::QRY_ATTITUDE);

            port.write_all(seatrac::cmd_nav_query_send(args.remote_beacon_id, query_flags, {}));

            // Await response or NAV_ERROR.
            std::optional<seatrac::NavQueryResp> last_nav;
            bool nav_error = false;

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(args.nav_response_timeout_ms);
            while (std::chrono::steady_clock::now() < deadline) {
                const auto f = rx_frames.pop_wait(100);
                if (!f) {
                    continue;
                }
                if (f->sync != '$') {
                    continue; // ignore echoes
                }

                try {
                    if (f->cid == seatrac::cid::NAV_QUERY_RESP) {
                        auto nav = seatrac::decode_nav_query_resp(*f);
                        if (nav.local_flag && nav.fix.src_id == args.remote_beacon_id) {
                            last_nav = std::move(nav);
                            break;
                        }
                    }
                    else if (f->cid == seatrac::cid::NAV_ERROR) {
                        auto err = seatrac::decode_nav_error(*f);
                        if (err.beacon_id == args.remote_beacon_id) {
                            nav_error = true;
                            break;
                        }
                    }
                    else {
                        // Drain queue: you can log additional messages here if desired.
                    }
                }
                catch (const std::exception&) {
                    // Ignore malformed frames.
                }
            }

            if (nav_error || !last_nav.has_value()) {
                std::cerr << "[warn] NAV query timeout or error for beacon " << int(args.remote_beacon_id) << "\n";
                std::this_thread::sleep_until(next_tick);
                continue;
            }

            // Compute corrected position (paper-inspired).
            const auto decision = tracker.update(last_nav->fix);

            // Control requires remote yaw.
            if (!last_nav->remote_yaw_deg.has_value()) {
                std::cerr << "[warn] remote_yaw unavailable; skipping control\n";
                std::this_thread::sleep_until(next_tick);
                continue;
            }

            const double steering_deg = heading.compute_steering_deg(
                decision.chosen,
                dest,
                *last_nav->remote_yaw_deg);

            // Build & send control packet.
            usbl::ControlPacket cmd;
            cmd.seq = seq++;
            cmd.steering_deci_deg = static_cast<int16_t>(std::lround(steering_deg * 10.0));
            cmd.speed_cms = static_cast<uint16_t>(std::clamp(args.speed_cms, 0.0, 65535.0));

            const auto payload = cmd.encode();

            // Use MSG_REQ so the remote beacon will ACK (DAT_RECEIVE with ACK_FLAG).
            port.write_all(seatrac::cmd_dat_send(args.remote_beacon_id, seatrac::MsgType::MSG_REQ, payload));

            std::cout << "[nav] NED=(" << decision.chosen.n << ", " << decision.chosen.e << ", " << decision.chosen.d
                << ") yaw=" << *last_nav->remote_yaw_deg
                << " steer=" << steering_deg
                << " used=" << (decision.used_measured ? "measured" : (decision.used_virtual ? "virtual" : "hold"))
                << " rel_err=" << decision.measured_vs_virtual_rel_error
                << "\n";

            std::this_thread::sleep_until(next_tick);
        }

        g_stop.store(true);
        if (reader.joinable()) {
            reader.join();
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
}
