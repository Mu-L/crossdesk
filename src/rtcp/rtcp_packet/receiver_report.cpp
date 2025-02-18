#include "receiver_report.h"

ReceiverReport::ReceiverReport() : buffer_(nullptr), size_(0) {}

ReceiverReport::~ReceiverReport() {
  if (buffer_) {
    delete[] buffer_;
    buffer_ = nullptr;
  }
  size_ = 0;
}

void ReceiverReport::SetReportBlock(RtcpReportBlock &rtcp_report_block) {
  reports_.push_back(std::move(rtcp_report_block));
}

void ReceiverReport::SetReportBlocks(
    std::vector<RtcpReportBlock> &rtcp_report_blocks) {
  reports_ = std::move(rtcp_report_blocks);
}

const uint8_t *ReceiverReport::Create() {
  size_t buffer_size =
      DEFAULT_SR_SIZE + reports_.size() * RtcpReportBlock::kLength;
  if (!buffer_ || buffer_size != size_) {
    delete[] buffer_;
    buffer_ = nullptr;
  }

  buffer_ = new uint8_t[buffer_size];
  size_ = buffer_size;

  int pos =
      rtcp_common_header_.Create(DEFAULT_RTCP_VERSION, 0, DEFAULT_RR_BLOCK_NUM,
                                 RTCP_TYPE::RR, DEFAULT_RR_SIZE, buffer_);

  for (const auto &report : reports_) {
    pos += report.Create(buffer_ + pos);
  }

  return buffer_;
}

size_t ReceiverReport::Parse(const uint8_t *buffer) {
  reports_.clear();
  size_t pos = rtcp_common_header_.Parse(buffer);

  for (int i = 0; i < rtcp_common_header_.CountOrFormat(); i++) {
    RtcpReportBlock report;
    pos += report.Parse(buffer + pos);
    reports_.emplace_back(std::move(report));
  }

  return pos;
}