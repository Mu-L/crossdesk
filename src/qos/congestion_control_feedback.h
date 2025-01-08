/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-18
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CONGESTION_CONTROL_FEEDBACK_H_
#define _CONGESTION_CONTROL_FEEDBACK_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "array_view.h"
#include "enc_mark.h"
#include "rtp_feedback.h"

// Congestion control feedback message as specified in
// https://www.rfc-editor.org/rfc/rfc8888.html
class CongestionControlFeedback : public RtpFeedback {
 public:
  struct PacketInfo {
    uint32_t ssrc = 0;
    uint16_t sequence_number = 0;
    //  Time offset from report timestamp. Minus infinity if the packet has not
    //  been received.
    int64_t arrival_time_offset = std::numeric_limits<int64_t>::min();
    EcnMarking ecn = EcnMarking::kNotEct;
  };

  static constexpr uint8_t kFeedbackMessageType = 11;

  // `Packets` MUST be sorted in sequence_number order per SSRC. There MUST not
  // be missing sequence numbers between `Packets`. `Packets` MUST not include
  // duplicate sequence numbers.
  CongestionControlFeedback(std::vector<PacketInfo> packets,
                            uint32_t report_timestamp_compact_ntp);
  CongestionControlFeedback() = default;

  bool Parse(const RtcpCommonHeader& packet);

  ArrayView<const PacketInfo> packets() const { return packets_; }

  uint32_t report_timestamp_compact_ntp() const {
    return report_timestamp_compact_ntp_;
  }

  // Serialize the packet.
  bool Create(uint8_t* packet, size_t* position, size_t max_length,
              PacketReadyCallback callback) const override;
  size_t BlockLength() const override;

 private:
  std::vector<PacketInfo> packets_;
  uint32_t report_timestamp_compact_ntp_ = 0;
};

#endif