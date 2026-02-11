#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace seatrac {

        inline uint16_t crc16_ibm(const uint8_t* buf, size_t len) {    
        uint16_t poly = 0xA001;
        uint16_t crc = 0;
        for (size_t b = 0; b < len; ++b) {
            uint8_t v = buf[b];
            for (uint8_t i = 0; i < 8; ++i) {
                if (((v & 0x01u) != 0u) ^ ((crc & 0x01u) != 0u)) {
                    crc >>= 1;
                    crc ^= poly;
                }
                else {
                    crc >>= 1;
                }
                v >>= 1;
            }
        }
        return crc;
    }

    inline char nibble_to_hex(uint8_t n) {
        static const char* kHex = "0123456789ABCDEF";
        return kHex[n & 0x0Fu];
    }

    inline uint8_t hex_to_nibble(char c) {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(10 + (c - 'A'));
        throw std::runtime_error("Invalid hex character");
    }

    inline std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::string out;
        out.reserve(bytes.size() * 2);
        for (uint8_t b : bytes) {
            out.push_back(nibble_to_hex(static_cast<uint8_t>(b >> 4)));
            out.push_back(nibble_to_hex(static_cast<uint8_t>(b & 0x0F)));
        }
        return out;
    }

    inline std::vector<uint8_t> hex_to_bytes(std::string_view hex) {
        if (hex.size() % 2 != 0) {
            throw std::runtime_error("Hex string length must be even");
        }
        std::vector<uint8_t> out;
        out.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            const uint8_t hi = hex_to_nibble(hex[i]);
            const uint8_t lo = hex_to_nibble(hex[i + 1]);
            out.push_back(static_cast<uint8_t>((hi << 4) | lo));
        }
        return out;
    }

    // --- Frame builder ---
    struct Frame {
        // sync: '#' for host->beacon, '$' for beacon->host.
        char sync = '$';
        uint8_t cid = 0;
        std::vector<uint8_t> payload; // does NOT include crc
    };

    inline std::string build_command(uint8_t cid, const std::vector<uint8_t>& payload) {
        // Message format: # + CID(1B) + payload + CRC16(LE) encoded as ASCII-hex + CRLF
        std::vector<uint8_t> raw;
        raw.reserve(1 + payload.size());
        raw.push_back(cid);
        raw.insert(raw.end(), payload.begin(), payload.end());

        const uint16_t crc = crc16_ibm(raw.data(), raw.size());
        const uint8_t crc_lo = static_cast<uint8_t>(crc & 0xFFu);
        const uint8_t crc_hi = static_cast<uint8_t>((crc >> 8) & 0xFFu);

        std::vector<uint8_t> raw_plus;
        raw_plus.reserve(raw.size() + 2);
        raw_plus.insert(raw_plus.end(), raw.begin(), raw.end());
        raw_plus.push_back(crc_lo);
        raw_plus.push_back(crc_hi);

        std::string out;
        out.reserve(1 + raw_plus.size() * 2 + 2);
        out.push_back('#');
        out += bytes_to_hex(raw_plus);
        out.push_back('\r');
        out.push_back('\n');
        return out;
    }

    // --- Streaming parser ---
    class StreamParser {
    public:
        using Callback = std::function<void(const Frame&)>;

        explicit StreamParser(Callback cb)
            : cb_(std::move(cb)) {
            if (!cb_) {
                throw std::invalid_argument("StreamParser requires a callback");
            }
        }

        void reset() {
            state_ = State::WAIT_SYNC;
            sync_ = 0;
            buf_.clear();
            last_was_cr_ = false;
        }

        // Feed bytes from serial stream.
        void feed(const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                const char c = static_cast<char>(data[i]);

                if (state_ == State::WAIT_SYNC) {
                    if (c == '#' || c == '$') {
                        sync_ = c;
                        buf_.clear();
                        last_was_cr_ = false;
                        state_ = State::IN_MSG;
                    }
                    continue;
                }

                // IN_MSG
                if (last_was_cr_) {
                    if (c == '\n') {
                        process_message();
                        state_ = State::WAIT_SYNC;
                        last_was_cr_ = false;
                        continue;
                    }
                    else {
                        // stray CR, keep going but treat as data separator.
                        last_was_cr_ = false;
                    }
                }

                if (c == '\r') {
                    last_was_cr_ = true;
                    continue;
                }

                if (buf_.size() < max_chars_) {
                    buf_.push_back(c);
                }
                else {
                    // Oversized, drop.
                    state_ = State::WAIT_SYNC;
                    last_was_cr_ = false;
                    buf_.clear();
                }
            }
        }

    private:
        enum class State { WAIT_SYNC, IN_MSG };

        void process_message() {
            // buf_ contains ASCII hex (CID+payload+CRC) (no CRLF).
            // Reject if any non-hex char.
            for (char ch : buf_) {
                if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                    return; // ignore malformed messages silently
                }
            }

            // Minimum: CID(1B)=2 chars + CRC(2B)=4 chars => 6 chars hex.
            if (buf_.size() < 6 || (buf_.size() % 2) != 0) {
                return;
            }

            std::vector<uint8_t> bytes;
            try {
                bytes = hex_to_bytes(std::string_view(buf_.data(), buf_.size()));
            }
            catch (...) {
                return;
            }
            if (bytes.size() < 3) {
                return;
            }

            // Split CRC
            const uint8_t crc_lo = bytes[bytes.size() - 2];
            const uint8_t crc_hi = bytes[bytes.size() - 1];
            const uint16_t crc_recv = static_cast<uint16_t>(crc_lo | (static_cast<uint16_t>(crc_hi) << 8));

            const size_t raw_len = bytes.size() - 2;
            const uint16_t crc_calc = crc16_ibm(bytes.data(), raw_len);
            if (crc_calc != crc_recv) {
                return;
            }

            Frame f;
            f.sync = sync_;
            f.cid = bytes[0];
            f.payload.assign(bytes.begin() + 1, bytes.begin() + static_cast<long>(raw_len));
            cb_(f);
        }

        Callback cb_;
        State state_ = State::WAIT_SYNC;
        char sync_ = 0;
        std::vector<char> buf_;
        bool last_was_cr_ = false;
        size_t max_chars_ = 8192; // enough for CID_XCVR_USBL (~461 bytes => 922 hex chars + overhead)
    };

    // --- Command IDs (CID_E) ---
    // Core subset required for this project.
    namespace cid {
        static constexpr uint8_t SYS_ALIVE = 0x01;
        static constexpr uint8_t STATUS = 0x10;
        static constexpr uint8_t PING_SEND = 0x40;
        static constexpr uint8_t PING_RESP = 0x42;
        static constexpr uint8_t PING_ERROR = 0x43;
        static constexpr uint8_t NAV_QUERY_SEND = 0x50;
        static constexpr uint8_t NAV_QUERY_RESP = 0x52;
        static constexpr uint8_t NAV_ERROR = 0x53;
        static constexpr uint8_t DAT_SEND = 0x60;
        static constexpr uint8_t DAT_RECEIVE = 0x61;
        static constexpr uint8_t DAT_ERROR = 0x63;
        static constexpr uint8_t XCVR_USBL = 0x38;
    } // namespace cid

    // --- Acoustic message type (AMSGTYPE_E) ---
    enum class MsgType : uint8_t {
        MSG_OWAY = 0x00,
        MSG_OWAYU = 0x01,
        MSG_REQ = 0x02,
        MSG_RESP = 0x03,
        MSG_REQU = 0x04,
        MSG_RESPU = 0x05,
        MSG_REQX = 0x06,
        MSG_RESPX = 0x07,
        MSG_UNKNOWN = 0xFF,
    };

    // --- NAV query flags (NAV_QUERY_T) ---
    namespace nav_query {
        static constexpr uint8_t QRY_DEPTH = 1u << 0;
        static constexpr uint8_t QRY_SUPPLY = 1u << 1;
        static constexpr uint8_t QRY_TEMP = 1u << 2;
        static constexpr uint8_t QRY_ATTITUDE = 1u << 3;
        static constexpr uint8_t QRY_DATA = 1u << 7;
    }

    // --- Little-endian binary reader ---
    class ByteReader {
    public:
        explicit ByteReader(const std::vector<uint8_t>& b) : buf_(b) {}

        size_t remaining() const noexcept { return buf_.size() - off_; }
        size_t offset() const noexcept { return off_; }

        template <typename T>
        std::optional<T> read_le() {
            if (remaining() < sizeof(T)) {
                return std::nullopt;
            }
            T v{};
            std::memcpy(&v, buf_.data() + off_, sizeof(T));
            off_ += sizeof(T);
            // Fields are specified as little-endian for multi-byte types.
            // Assuming the host is little-endian (common on x86/x64/ARM64). If not, convert.
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            v = byteswap(v);
#endif
            return v;
        }

        std::optional<uint8_t> read_u8() { return read_le<uint8_t>(); }
        std::optional<int8_t> read_i8() { return read_le<int8_t>(); }
        std::optional<uint16_t> read_u16() { return read_le<uint16_t>(); }
        std::optional<int16_t> read_i16() { return read_le<int16_t>(); }
        std::optional<uint32_t> read_u32() { return read_le<uint32_t>(); }
        std::optional<int32_t> read_i32() { return read_le<int32_t>(); }
        std::optional<float> read_f32() { return read_le<float>(); }

        std::optional<std::vector<uint8_t>> read_bytes(size_t n) {
            if (remaining() < n) return std::nullopt;
            std::vector<uint8_t> out(buf_.begin() + static_cast<long>(off_), buf_.begin() + static_cast<long>(off_ + n));
            off_ += n;
            return out;
        }

    private:
        const std::vector<uint8_t>& buf_;
        size_t off_ = 0;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        template <typename T>
        static T byteswap(T v) {
            static_assert(std::is_trivially_copyable<T>::value, "byteswap requires trivially copyable type");
            std::array<uint8_t, sizeof(T)> a{};
            std::memcpy(a.data(), &v, sizeof(T));
            std::reverse(a.begin(), a.end());
            std::memcpy(&v, a.data(), sizeof(T));
            return v;
        }
#endif
    };

    // --- ACOFIX_T (subset + fields used here) ---
    struct AcousticFix {
        // Mandatory fields
        uint8_t dest_id = 0;
        uint8_t src_id = 0;
        uint8_t flags = 0;
        MsgType msg_type = MsgType::MSG_UNKNOWN;

        // Always-present local beacon fields (for fix computation)
        double local_yaw_deg = 0.0;
        double local_pitch_deg = 0.0;
        double local_roll_deg = 0.0;
        double local_depth_m = 0.0;
        double vos_mps = 0.0;
        double rssi_db = 0.0;

        // Optional fields depending on flags
        std::optional<uint32_t> range_count;
        std::optional<double> range_time_s;
        std::optional<double> range_dist_m;

        std::optional<uint8_t> usbl_channels;
        std::vector<double> usbl_rssi_db; // size == channels
        std::optional<double> usbl_azimuth_deg;    // 0..360
        std::optional<double> usbl_elevation_deg;  // -90..90
        std::optional<double> usbl_fit_error;

        // Position is NED (Northing, Easting, Depth) in meters, relative to local beacon.
        // Depth is from surface in developer guide; still treated as 'Depth' positive down.
        std::optional<double> pos_easting_m;
        std::optional<double> pos_northing_m;
        std::optional<double> pos_depth_m;

        bool has_range() const noexcept { return (flags & 0x01u) != 0u; }
        bool has_usbl() const noexcept { return (flags & 0x02u) != 0u; }
        bool has_position() const noexcept { return (flags & 0x04u) != 0u; }
        bool position_filter_error() const noexcept { return (flags & 0x10u) != 0u; }
        bool position_enhanced() const noexcept { return (flags & 0x08u) != 0u; }
    };

    inline AcousticFix decode_acofix(ByteReader& r) {
        AcousticFix fix;

        auto dest = r.read_u8();
        auto src = r.read_u8();
        auto flags = r.read_u8();
        auto msg_type = r.read_u8();

        if (!dest || !src || !flags || !msg_type) {
            throw std::runtime_error("ACOFIX: truncated mandatory fields");
        }

        fix.dest_id = *dest;
        fix.src_id = *src;
        fix.flags = *flags;
        fix.msg_type = static_cast<MsgType>(*msg_type);

        // Local attitude & environment
        auto yaw_dd = r.read_i16();
        auto pitch_dd = r.read_i16();
        auto roll_dd = r.read_i16();
        auto depth_dm = r.read_u16();
        auto vos_dms = r.read_u16();
        auto rssi_cb = r.read_i16();

        if (!yaw_dd || !pitch_dd || !roll_dd || !depth_dm || !vos_dms || !rssi_cb) {
            throw std::runtime_error("ACOFIX: truncated base fields");
        }

        fix.local_yaw_deg = static_cast<double>(*yaw_dd) / 10.0;
        fix.local_pitch_deg = static_cast<double>(*pitch_dd) / 10.0;
        fix.local_roll_deg = static_cast<double>(*roll_dd) / 10.0;
        fix.local_depth_m = static_cast<double>(*depth_dm) / 10.0;
        fix.vos_mps = static_cast<double>(*vos_dms) / 10.0;
        fix.rssi_db = static_cast<double>(*rssi_cb) / 10.0;

        // Range fields
        if (fix.has_range()) {
            auto rc = r.read_u32();
            auto rt = r.read_i32();
            auto rd_dm = r.read_u16();
            if (!rc || !rt || !rd_dm) {
                throw std::runtime_error("ACOFIX: truncated range fields");
            }
            fix.range_count = *rc;
            fix.range_time_s = static_cast<double>(*rt) / 10000000.0; // 100ns multiples
            fix.range_dist_m = static_cast<double>(*rd_dm) / 10.0;
        }

        // USBL fields
        if (fix.has_usbl()) {
            auto ch = r.read_u8();
            if (!ch) {
                throw std::runtime_error("ACOFIX: truncated USBL_CHANNELS");
            }
            fix.usbl_channels = *ch;
            fix.usbl_rssi_db.clear();
            fix.usbl_rssi_db.reserve(*ch);
            for (uint8_t i = 0; i < *ch; ++i) {
                auto v = r.read_i16();
                if (!v) {
                    throw std::runtime_error("ACOFIX: truncated USBL_RSSI[]");
                }
                fix.usbl_rssi_db.push_back(static_cast<double>(*v) / 10.0);
            }
            auto az_dd = r.read_i16();
            auto el_dd = r.read_i16();
            auto fit_err = r.read_i16();
            if (!az_dd || !el_dd || !fit_err) {
                throw std::runtime_error("ACOFIX: truncated USBL angle fields");
            }
            fix.usbl_azimuth_deg = static_cast<double>(*az_dd) / 10.0;
            fix.usbl_elevation_deg = static_cast<double>(*el_dd) / 10.0;
            fix.usbl_fit_error = static_cast<double>(*fit_err) / 100.0;
        }

        // Position fields (Easting, Northing, Depth)
        if (fix.has_position()) {
            auto e_dm = r.read_i16();
            auto n_dm = r.read_i16();
            auto d_dm = r.read_i16();
            if (!e_dm || !n_dm || !d_dm) {
                throw std::runtime_error("ACOFIX: truncated position fields");
            }
            fix.pos_easting_m = static_cast<double>(*e_dm) / 10.0;
            fix.pos_northing_m = static_cast<double>(*n_dm) / 10.0;
            fix.pos_depth_m = static_cast<double>(*d_dm) / 10.0;
        }

        return fix;
    }

    // --- High-level decoded message types used by this project ---

    struct PingResp {
        AcousticFix fix;
    };

    struct PingError {
        uint8_t status = 0;
        uint8_t beacon_id = 0;
    };

    struct NavQueryResp {
        AcousticFix fix;
        uint8_t query_flags = 0;

        std::optional<double> remote_depth_m;
        std::optional<double> remote_supply_v;
        std::optional<double> remote_temp_c;
        std::optional<double> remote_yaw_deg;
        std::optional<double> remote_pitch_deg;
        std::optional<double> remote_roll_deg;

        std::vector<uint8_t> data;
        bool local_flag = false;
    };

    struct NavError {
        uint8_t status = 0;
        uint8_t beacon_id = 0;
    };

    struct DatReceive {
        AcousticFix fix;
        bool ack_flag = false;
        std::vector<uint8_t> data;
        bool local_flag = false;
    };

    struct XcvrUsbl {
        float xcor_sig_peak = 0.0f;
        float xcor_threshold = 0.0f;
        uint16_t xcor_cross_point = 0;
        float xcor_cross_mag = 0.0f;
        uint16_t xcor_detect = 0;
        uint16_t xcor_length = 0;
        std::vector<float> xcor_data;

        uint8_t channels = 0;
        std::vector<double> channel_rssi_db;

        uint8_t baselines = 0;
        std::vector<float> phase_angle; // units not specified; as provided by beacon

        double signal_azimuth_deg = 0.0;
        double signal_elevation_deg = 0.0;
        float signal_fit_error = 0.0f;

        uint8_t beacon_dest_id = 0;
        uint8_t beacon_src_id = 0;
    };

    inline PingResp decode_ping_resp(const Frame& f) {
        ByteReader r(f.payload);
        PingResp out;
        out.fix = decode_acofix(r);
        return out;
    }

    inline PingError decode_ping_error(const Frame& f) {
        ByteReader r(f.payload);
        auto status = r.read_u8();
        auto bid = r.read_u8();
        if (!status || !bid) throw std::runtime_error("PING_ERROR truncated");
        return PingError{ *status, *bid };
    }

    inline NavError decode_nav_error(const Frame& f) {
        ByteReader r(f.payload);
        auto status = r.read_u8();
        auto bid = r.read_u8();
        if (!status || !bid) throw std::runtime_error("NAV_ERROR truncated");
        return NavError{ *status, *bid };
    }

    inline NavQueryResp decode_nav_query_resp(const Frame& f) {
        ByteReader r(f.payload);
        NavQueryResp out;
        out.fix = decode_acofix(r);
        auto qf = r.read_u8();
        if (!qf) throw std::runtime_error("NAV_QUERY_RESP truncated query_flags");
        out.query_flags = *qf;

        // Fields appended in order depending on flags.
        if (out.query_flags & nav_query::QRY_DEPTH) {
            auto depth_dm = r.read_i32();
            if (!depth_dm) throw std::runtime_error("NAV_QUERY_RESP truncated remote_depth");
            out.remote_depth_m = static_cast<double>(*depth_dm) / 10.0;
        }
        if (out.query_flags & nav_query::QRY_SUPPLY) {
            auto mv = r.read_u16();
            if (!mv) throw std::runtime_error("NAV_QUERY_RESP truncated remote_supply");
            out.remote_supply_v = static_cast<double>(*mv) / 1000.0;
        }
        if (out.query_flags & nav_query::QRY_TEMP) {
            auto dc = r.read_i16();
            if (!dc) throw std::runtime_error("NAV_QUERY_RESP truncated remote_temp");
            out.remote_temp_c = static_cast<double>(*dc) / 10.0;
        }
        if (out.query_flags & nav_query::QRY_ATTITUDE) {
            auto yaw_dd = r.read_i16();
            auto pitch_dd = r.read_i16();
            auto roll_dd = r.read_i16();
            if (!yaw_dd || !pitch_dd || !roll_dd) throw std::runtime_error("NAV_QUERY_RESP truncated remote_attitude");
            out.remote_yaw_deg = static_cast<double>(*yaw_dd) / 10.0;
            out.remote_pitch_deg = static_cast<double>(*pitch_dd) / 10.0;
            out.remote_roll_deg = static_cast<double>(*roll_dd) / 10.0;
        }
        if (out.query_flags & nav_query::QRY_DATA) {
            auto plen = r.read_u8();
            if (!plen) throw std::runtime_error("NAV_QUERY_RESP truncated packet_len");
            const auto data = r.read_bytes(*plen);
            if (!data) throw std::runtime_error("NAV_QUERY_RESP truncated packet_data");
            out.data = *data;
        }

        // Final field always present
        auto local = r.read_u8();
        if (!local) throw std::runtime_error("NAV_QUERY_RESP truncated local_flag");
        out.local_flag = (*local != 0);

        return out;
    }

    inline DatReceive decode_dat_receive(const Frame& f) {
        ByteReader r(f.payload);
        DatReceive out;
        out.fix = decode_acofix(r);
        auto ack = r.read_u8();
        auto plen = r.read_u8();
        if (!ack || !plen) throw std::runtime_error("DAT_RECEIVE truncated");
        out.ack_flag = (*ack != 0);
        if (*plen > 0) {
            auto data = r.read_bytes(*plen);
            if (!data) throw std::runtime_error("DAT_RECEIVE truncated packet_data");
            out.data = *data;
        }
        auto local = r.read_u8();
        if (!local) throw std::runtime_error("DAT_RECEIVE truncated local_flag");
        out.local_flag = (*local != 0);
        return out;
    }

    inline XcvrUsbl decode_xcvr_usbl(const Frame& f) {
        ByteReader r(f.payload);
        XcvrUsbl out;

        auto sig_peak = r.read_f32();
        auto thr = r.read_f32();
        auto cross_pt = r.read_u16();
        auto cross_mag = r.read_f32();
        auto detect = r.read_u16();
        auto xlen = r.read_u16();
        if (!sig_peak || !thr || !cross_pt || !cross_mag || !detect || !xlen) {
            throw std::runtime_error("XCVR_USBL truncated header fields");
        }

        out.xcor_sig_peak = *sig_peak;
        out.xcor_threshold = *thr;
        out.xcor_cross_point = *cross_pt;
        out.xcor_cross_mag = *cross_mag;
        out.xcor_detect = *detect;
        out.xcor_length = *xlen;

        out.xcor_data.clear();
        out.xcor_data.reserve(out.xcor_length);
        for (uint16_t i = 0; i < out.xcor_length; ++i) {
            auto v = r.read_f32();
            if (!v) throw std::runtime_error("XCVR_USBL truncated xcor_data");
            out.xcor_data.push_back(*v);
        }

        auto ch = r.read_u8();
        if (!ch) throw std::runtime_error("XCVR_USBL truncated channels");
        out.channels = *ch;

        out.channel_rssi_db.clear();
        out.channel_rssi_db.reserve(out.channels);
        for (uint8_t i = 0; i < out.channels; ++i) {
            auto cb = r.read_i16();
            if (!cb) throw std::runtime_error("XCVR_USBL truncated channel_rssi");
            out.channel_rssi_db.push_back(static_cast<double>(*cb) / 10.0);
        }

        auto bl = r.read_u8();
        if (!bl) throw std::runtime_error("XCVR_USBL truncated baselines");
        out.baselines = *bl;

        out.phase_angle.clear();
        out.phase_angle.reserve(out.baselines);
        for (uint8_t i = 0; i < out.baselines; ++i) {
            auto pa = r.read_f32();
            if (!pa) throw std::runtime_error("XCVR_USBL truncated phase_angle");
            out.phase_angle.push_back(*pa);
        }

        auto az_dd = r.read_i16();
        auto el_dd = r.read_i16();
        auto fit = r.read_f32();
        if (!az_dd || !el_dd || !fit) throw std::runtime_error("XCVR_USBL truncated signal angles");
        out.signal_azimuth_deg = static_cast<double>(*az_dd) / 10.0;
        out.signal_elevation_deg = static_cast<double>(*el_dd) / 10.0;
        out.signal_fit_error = *fit;

        auto did = r.read_u8();
        auto sid = r.read_u8();
        if (!did || !sid) throw std::runtime_error("XCVR_USBL truncated beacon ids");
        out.beacon_dest_id = *did;
        out.beacon_src_id = *sid;

        return out;
    }

    // --- Command helpers (host->beacon) ---
    inline std::string cmd_sys_alive() {
        return build_command(cid::SYS_ALIVE, {});
    }

    inline std::string cmd_ping_send(uint8_t dest_id, MsgType msg_type) {
        return build_command(cid::PING_SEND, { dest_id, static_cast<uint8_t>(msg_type) });
    }

    inline std::string cmd_nav_query_send(uint8_t dest_id, uint8_t query_flags, const std::vector<uint8_t>& packet) {
        if (packet.size() > 29) {
            throw std::runtime_error("NAV_QUERY packet too large (>29)");
        }
        std::vector<uint8_t> payload;
        payload.reserve(2 + 1 + packet.size());
        payload.push_back(dest_id);
        payload.push_back(query_flags);
        payload.push_back(static_cast<uint8_t>(packet.size()));
        payload.insert(payload.end(), packet.begin(), packet.end());
        return build_command(cid::NAV_QUERY_SEND, payload);
    }

    inline std::string cmd_dat_send(uint8_t dest_id, MsgType msg_type, const std::vector<uint8_t>& packet) {
        if (packet.size() > 30) {
            throw std::runtime_error("DAT packet too large (>30)");
        }
        std::vector<uint8_t> payload;
        payload.reserve(2 + 1 + packet.size());
        payload.push_back(dest_id);
        payload.push_back(static_cast<uint8_t>(msg_type));
        payload.push_back(static_cast<uint8_t>(packet.size()));
        payload.insert(payload.end(), packet.begin(), packet.end());
        return build_command(cid::DAT_SEND, payload);
    }

} // namespace seatrac
