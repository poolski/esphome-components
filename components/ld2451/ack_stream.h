#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "ack_codec.h"

namespace esphome::ld2451 {

enum class AckScanResult {
  NEED_MORE_DATA,
  MATCHED,
};

namespace ack_stream_detail {

static constexpr uint8_t DATA_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
static constexpr uint8_t DATA_TAIL[] = {0xF8, 0xF7, 0xF6, 0xF5};
static constexpr uint8_t CMD_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
static constexpr uint8_t CMD_TAIL[] = {0x04, 0x03, 0x02, 0x01};

inline size_t find_header(const std::vector<uint8_t> &rx_bytes, const uint8_t *header) {
  if (rx_bytes.size() < 4) {
    return rx_bytes.size();
  }
  for (size_t i = 0; i + 4 <= rx_bytes.size(); i++) {
    if (std::memcmp(rx_bytes.data() + i, header, 4) == 0) {
      return i;
    }
  }
  return rx_bytes.size();
}

inline void retain_header_prefix(std::vector<uint8_t> &rx_bytes) {
  if (rx_bytes.size() > 3) {
    rx_bytes.erase(rx_bytes.begin(), rx_bytes.end() - 3);
  }
}

}  // namespace ack_stream_detail

inline AckScanResult scan_for_matching_ack(std::vector<uint8_t> &rx_bytes, uint16_t command,
                                           uint16_t max_ack_payload_len, std::vector<uint8_t> *ret) {
  while (true) {
    const size_t data_pos = ack_stream_detail::find_header(rx_bytes, ack_stream_detail::DATA_HEADER);
    const size_t cmd_pos = ack_stream_detail::find_header(rx_bytes, ack_stream_detail::CMD_HEADER);

    if (data_pos == rx_bytes.size() && cmd_pos == rx_bytes.size()) {
      ack_stream_detail::retain_header_prefix(rx_bytes);
      return AckScanResult::NEED_MORE_DATA;
    }

    const bool next_is_data = data_pos < cmd_pos;
    const size_t frame_pos = next_is_data ? data_pos : cmd_pos;

    if (frame_pos > 0) {
      rx_bytes.erase(rx_bytes.begin(), rx_bytes.begin() + static_cast<long>(frame_pos));
    }

    if (rx_bytes.size() < 6) {
      return AckScanResult::NEED_MORE_DATA;
    }

    const uint16_t payload_len = static_cast<uint16_t>(rx_bytes[4]) | (static_cast<uint16_t>(rx_bytes[5]) << 8);
    const size_t frame_len = static_cast<size_t>(payload_len) + 10;
    if (rx_bytes.size() < frame_len) {
      return AckScanResult::NEED_MORE_DATA;
    }

    const uint8_t *expected_tail = next_is_data ? ack_stream_detail::DATA_TAIL : ack_stream_detail::CMD_TAIL;
    const size_t tail_pos = frame_len - 4;
    if (std::memcmp(rx_bytes.data() + tail_pos, expected_tail, 4) != 0) {
      rx_bytes.erase(rx_bytes.begin());
      continue;
    }

    if (next_is_data) {
      rx_bytes.erase(rx_bytes.begin(), rx_bytes.begin() + static_cast<long>(frame_len));
      continue;
    }

    std::vector<uint8_t> ack_payload(rx_bytes.begin() + 6, rx_bytes.begin() + 6 + static_cast<long>(payload_len));
    rx_bytes.erase(rx_bytes.begin(), rx_bytes.begin() + static_cast<long>(frame_len));

    const AckDecodeResult decoded = decode_ack(command, ack_payload, max_ack_payload_len);
    if (!decoded.ok) {
      continue;
    }

    if (ret != nullptr) {
      ret->assign(decoded.value.begin(), decoded.value.end());
    }
    return AckScanResult::MATCHED;
  }
}

}  // namespace esphome::ld2451
