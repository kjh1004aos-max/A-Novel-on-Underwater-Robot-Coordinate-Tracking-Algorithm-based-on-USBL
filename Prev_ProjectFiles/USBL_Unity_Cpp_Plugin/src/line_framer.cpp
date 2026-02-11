
#include "line_framer.h"

namespace usbl {

void LineFramer::PushBytes(const uint8_t* data, int size) {
    if (!data || size <= 0) return;
    buffer_.append(reinterpret_cast<const char*>(data), static_cast<size_t>(size));
}

std::vector<std::string> LineFramer::PopLines() {
    std::vector<std::string> out;
    size_t start = 0;

    for (;;) {
        const size_t nl = buffer_.find('\n', start);
        if (nl == std::string::npos) break;

        size_t lineEnd = nl;
        if (lineEnd > 0 && buffer_[lineEnd - 1] == '\r') {
            lineEnd -= 1;
        }

        out.emplace_back(buffer_.substr(0, lineEnd));
        buffer_.erase(0, nl + 1);
        start = 0;
    }

    // Optional: prevent unbounded growth if the stream has no newlines.
    if (buffer_.size() > 1'000'000) {
        buffer_.clear();
    }

    return out;
}

void LineFramer::Clear() { buffer_.clear(); }

} // namespace usbl
