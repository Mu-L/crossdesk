#include "rtp_packet_history.h"

#include "sequence_number_compare.h"

RtpPacketHistory::RtpPacketHistory() {}

RtpPacketHistory::~RtpPacketHistory() {}

void RtpPacketHistory::AddPacket(std::shared_ptr<RtpPacketToSend> rtp_packet,
                                 Timestamp send_time) {
  rtp_packet_history_.push_back(
      {rtp_packet, send_time, GetPacketIndex(rtp_packet->SequenceNumber())});
}

int RtpPacketHistory::GetPacketIndex(uint16_t sequence_number) const {
  if (packet_history_.empty()) {
    return 0;
  }

  int first_seq = packet_history_.front().packet_->SequenceNumber();
  if (first_seq == sequence_number) {
    return 0;
  }

  int packet_index = sequence_number - first_seq;
  constexpr int kSeqNumSpan = std::numeric_limits<uint16_t>::max() + 1;

  if (IsNewerSequenceNumber(sequence_number, first_seq)) {
    if (sequence_number < first_seq) {
      // Forward wrap.
      packet_index += kSeqNumSpan;
    }
  } else if (sequence_number > first_seq) {
    // Backwards wrap.
    packet_index -= kSeqNumSpan;
  }

  return packet_index;
}