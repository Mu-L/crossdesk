#include "sender_report.h"

SenderReport::SenderReport() : buffer_(nullptr), size_(0) {}

SenderReport::~SenderReport() {
  if (buffer_) {
    delete[] buffer_;
    buffer_ = nullptr;
  }

  size_ = 0;
}

void SenderReport::SetReportBlock(RtcpReportBlock &rtcp_report_block) {
  reports_.push_back(std::move(rtcp_report_block));
}

void SenderReport::SetReportBlocks(
    std::vector<RtcpReportBlock> &rtcp_report_blocks) {
  reports_ = std::move(rtcp_report_blocks);
}

const uint8_t *SenderReport::Create() {
  size_t buffer_size =
      DEFAULT_SR_SIZE + reports_.size() * RtcpReportBlock::kLength;
  if (!buffer_ || buffer_size != size_) {
    delete[] buffer_;
    buffer_ = nullptr;
  }

  buffer_ = new uint8_t[buffer_size];
  size_ = buffer_size;

  int pos =
      rtcp_common_header_.Create(DEFAULT_RTCP_VERSION, 0, DEFAULT_SR_BLOCK_NUM,
                                 RTCP_TYPE::SR, buffer_size, buffer_);

  buffer_[pos++] = sender_info_.sender_ssrc >> 24 & 0xFF;
  buffer_[pos++] = sender_info_.sender_ssrc >> 16 & 0xFF;
  buffer_[pos++] = sender_info_.sender_ssrc >> 8 & 0xFF;
  buffer_[pos++] = sender_info_.sender_ssrc & 0xFF;

  buffer_[pos++] = sender_info_.ntp_ts_msw >> 56 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_msw >> 48 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_msw >> 40 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_msw >> 32 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_lsw >> 24 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_lsw >> 16 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_lsw >> 8 & 0xFF;
  buffer_[pos++] = sender_info_.ntp_ts_lsw & 0xFF;

  buffer_[pos++] = sender_info_.rtp_ts >> 24 & 0xFF;
  buffer_[pos++] = sender_info_.rtp_ts >> 16 & 0xFF;
  buffer_[pos++] = sender_info_.rtp_ts >> 8 & 0xFF;
  buffer_[pos++] = sender_info_.rtp_ts & 0xFF;

  buffer_[pos++] = sender_info_.sender_packet_count >> 24 & 0xFF;
  buffer_[pos++] = sender_info_.sender_packet_count >> 16 & 0xFF;
  buffer_[pos++] = sender_info_.sender_packet_count >> 8 & 0xFF;
  buffer_[pos++] = sender_info_.sender_packet_count & 0xFF;

  buffer_[pos++] = sender_info_.sender_octet_count >> 24 & 0xFF;
  buffer_[pos++] = sender_info_.sender_octet_count >> 16 & 0xFF;
  buffer_[pos++] = sender_info_.sender_octet_count >> 8 & 0xFF;
  buffer_[pos++] = sender_info_.sender_octet_count & 0xFF;

  for (auto &report : reports_) {
    pos += report.Create(buffer_ + pos);
  }

  return buffer_;
}

size_t SenderReport::Parse() {
  reports_.clear();
  size_t pos = rtcp_common_header_.Parse(buffer_);

  sender_info_.sender_ssrc = (buffer_[pos] << 24) + (buffer_[pos + 1] << 16) +
                             (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;
  sender_info_.ntp_ts_msw = (buffer_[pos] << 24) + (buffer_[pos + 1] << 16) +
                            (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;
  sender_info_.ntp_ts_lsw = (buffer_[pos] << 24) + (buffer_[pos + 1] << 16) +
                            (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;
  sender_info_.rtp_ts = (buffer_[pos] << 24) + (buffer_[pos + 1] << 16) +
                        (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;
  sender_info_.sender_packet_count = (buffer_[pos] << 24) +
                                     (buffer_[pos + 1] << 16) +
                                     (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;
  sender_info_.sender_octet_count = (buffer_[pos] << 24) +
                                    (buffer_[pos + 1] << 16) +
                                    (buffer_[pos + 2] << 8) + buffer_[pos + 3];
  pos += 4;

  for (int i = 0; i < rtcp_common_header_.CountOrFormat(); i++) {
    RtcpReportBlock report;
    pos += report.Parse(buffer_ + pos);
    reports_.emplace_back(std::move(report));
  }

  return pos;
}