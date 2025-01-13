#ifndef _CONGESTION_CONTROL_H_
#define _CONGESTION_CONTROL_H_

#include <deque>
#include <memory>

#include "congestion_window_pushback_controller.h"
#include "network_types.h"

class CongestionControl {
 public:
  CongestionControl();
  ~CongestionControl();

 public:
  NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback report);

 private:
  const std::unique_ptr<CongestionWindowPushbackController>
      congestion_window_pushback_controller_;

 private:
  std::deque<int64_t> feedback_max_rtts_;
  // std::unique_ptr<SendSideBandwidthEstimation> bandwidth_estimation_;
  int expected_packets_since_last_loss_update_ = 0;
  int lost_packets_since_last_loss_update_ = 0;
  int64_t next_loss_update_ = std::numeric_limits<int64_t>::min();
  const bool packet_feedback_only_ = false;
  bool previously_in_alr_ = false;
  // const bool limit_probes_lower_than_throughput_estimate_;
  std::optional<int64_t> current_data_window_;
};

#endif